#include "board_health.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "board_config.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_eth.h"
#include "esp_eth_mac_esp.h"
#include "esp_eth_netif_glue.h"
#include "esp_eth_phy.h"
#include "esp_event.h"
#include "esp_heap_caps.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_psram.h"
#include "esp_timer.h"

static const char *TAG = "watchpup_health";
static watchpup_board_health_t *s_health;

static void ethernet_event(void *arg, esp_event_base_t base, int32_t event_id, void *event_data)
{
    (void)arg; (void)base; (void)event_data;
    if (event_id == ETHERNET_EVENT_CONNECTED) {
        s_health->ethernet_link_up = true;
        ESP_LOGI(TAG, "[diag] subsystem.health subsystem=ethernet status=degraded link_up=true ipv4_address=null");
    } else if (event_id == ETHERNET_EVENT_DISCONNECTED) {
        s_health->ethernet_link_up = false;
        s_health->ethernet_has_ipv4 = false;
        s_health->ipv4_address[0] = '\0';
        ESP_LOGW(TAG, "[diag] subsystem.health subsystem=ethernet status=degraded link_up=false ipv4_address=null");
    }
}

static void got_ip(void *arg, esp_event_base_t base, int32_t event_id, void *event_data)
{
    (void)arg; (void)base;
    if (event_id == IP_EVENT_ETH_LOST_IP) {
        s_health->ethernet_has_ipv4 = false;
        s_health->ipv4_address[0] = '\0';
        ESP_LOGW(TAG, "[diag] subsystem.health subsystem=ethernet status=degraded link_up=%s ipv4_address=null",
                 s_health->ethernet_link_up ? "true" : "false");
        return;
    }
    const ip_event_got_ip_t *event = event_data;
    snprintf(s_health->ipv4_address, sizeof(s_health->ipv4_address), IPSTR, IP2STR(&event->ip_info.ip));
    s_health->ethernet_has_ipv4 = true;
    ESP_LOGI(TAG, "[diag] subsystem.health subsystem=ethernet status=ok link_up=true ipv4_address=%s", s_health->ipv4_address);
    ESP_LOGI(TAG, "[diag] health.summary overall_status=%s uptime_ms=%lld",
             s_health->psram_test_passed ? "ok" : "degraded", (long long)(esp_timer_get_time() / 1000));
}

void watchpup_board_health_test_psram(watchpup_board_health_t *health)
{
    health->psram_size_bytes = esp_psram_get_size();
    const size_t test_size_bytes = 64 * 1024;
    uint8_t *buffer = heap_caps_malloc(test_size_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    health->psram_test_passed = buffer != NULL;
    if (buffer != NULL) {
        for (size_t i = 0; i < test_size_bytes; ++i) buffer[i] = (uint8_t)(i ^ 0xA5);
        for (size_t i = 0; i < test_size_bytes; ++i) {
            if (buffer[i] != (uint8_t)(i ^ 0xA5)) { health->psram_test_passed = false; break; }
        }
        heap_caps_free(buffer);
    }
    ESP_LOGI(TAG, "[diag] subsystem.init subsystem=psram status=%s size_bytes=%u test_passed=%s",
             health->psram_test_passed ? "ok" : "error", (unsigned)health->psram_size_bytes,
             health->psram_test_passed ? "true" : "false");
}

esp_err_t watchpup_board_health_start_ethernet(watchpup_board_health_t *health)
{
    s_health = health;
    const watchpup_board_config_t *board = &WATCHPUP_BOARD_CONFIG;
    gpio_config_t power = {.pin_bit_mask = 1ULL << board->phy_power_gpio, .mode = GPIO_MODE_OUTPUT};
    ESP_RETURN_ON_ERROR(gpio_config(&power), TAG, "configure PHY power");
    ESP_RETURN_ON_ERROR(gpio_set_level(board->phy_power_gpio, 1), TAG, "enable PHY power");

    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.phy_addr = board->phy_address;
    phy_config.reset_gpio_num = -1;
    eth_esp32_emac_config_t emac = ETH_ESP32_EMAC_DEFAULT_CONFIG();
    emac.smi_gpio.mdc_num = board->mdc_gpio;
    emac.smi_gpio.mdio_num = board->mdio_gpio;
    emac.clock_config.rmii.clock_mode = EMAC_CLK_EXT_IN;
    emac.clock_config.rmii.clock_gpio = board->rmii_clock_gpio;
    emac.emac_dataif_gpio.rmii.tx_en_num = board->rmii_tx_en_gpio;
    emac.emac_dataif_gpio.rmii.txd0_num = board->rmii_txd0_gpio;
    emac.emac_dataif_gpio.rmii.txd1_num = board->rmii_txd1_gpio;
    emac.emac_dataif_gpio.rmii.crs_dv_num = board->rmii_crs_dv_gpio;
    emac.emac_dataif_gpio.rmii.rxd0_num = board->rmii_rxd0_gpio;
    emac.emac_dataif_gpio.rmii.rxd1_num = board->rmii_rxd1_gpio;

    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&emac, &mac_config);
#if ESP_IDF_VERSION_MAJOR >= 6
    esp_eth_phy_t *phy = esp_eth_phy_new_generic(&phy_config);
#else
    esp_eth_phy_t *phy = esp_eth_phy_new_ip101(&phy_config);
#endif
    if (mac == NULL || phy == NULL) return ESP_ERR_NO_MEM;
    esp_eth_handle_t handle = NULL;
    esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
    ESP_RETURN_ON_ERROR(esp_eth_driver_install(&config, &handle), TAG, "install Ethernet driver");
    esp_netif_config_t netif_config = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *netif = esp_netif_new(&netif_config);
    if (netif == NULL) return ESP_ERR_NO_MEM;
    esp_eth_netif_glue_handle_t glue = esp_eth_new_netif_glue(handle);
    if (glue == NULL) return ESP_ERR_NO_MEM;
    ESP_RETURN_ON_ERROR(esp_netif_attach(netif, glue), TAG, "attach Ethernet netif");
    ESP_RETURN_ON_ERROR(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, ethernet_event, NULL), TAG, "register Ethernet events");
    ESP_RETURN_ON_ERROR(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, got_ip, NULL), TAG, "register IP events");
    ESP_RETURN_ON_ERROR(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_LOST_IP, got_ip, NULL), TAG, "register lost-IP event");
    ESP_RETURN_ON_ERROR(esp_eth_start(handle), TAG, "start Ethernet");
    health->ethernet_initialized = true;
    ESP_LOGI(TAG, "[diag] subsystem.init subsystem=ethernet status=degraded initialized=true dhcp_enabled=true");
    return ESP_OK;
}
