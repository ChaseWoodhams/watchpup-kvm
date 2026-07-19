#include "board_health.h"
#include "bridge_tc358743.h"
#include "capture_csi.h"
#include "diagnostics.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"

static const char *TAG = "watchpup_boot";
static watchpup_board_health_t s_health;
static watchpup_tc358743_health_t s_bridge;
static watchpup_capture_health_t s_capture;

static esp_err_t watchpup_init_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

void app_main(void)
{
    watchpup_diag_log_boot_start();

    ESP_ERROR_CHECK(watchpup_init_nvs());
    ESP_LOGI(TAG, "[diag] subsystem.init subsystem=nvs status=ok");
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_LOGI(TAG, "[diag] subsystem.init subsystem=network_stack status=ok");
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_LOGI(TAG, "[diag] subsystem.init subsystem=event_loop status=ok");

    watchpup_board_health_test_psram(&s_health);
    const esp_err_t ethernet_result = watchpup_board_health_start_ethernet(&s_health);
    if (ethernet_result != ESP_OK) {
        s_health.ethernet_init_failed = true;
        ESP_LOGE(TAG, "[diag] subsystem.init subsystem=ethernet status=error error=initialization_failed code=%s",
                 esp_err_to_name(ethernet_result));
    }
    const esp_err_t bridge_result = watchpup_tc358743_start(&s_bridge);
    if (bridge_result != ESP_OK) {
        ESP_LOGE(TAG, "[diag] subsystem.init subsystem=bridge_tc358743 status=error error=%s",
                 esp_err_to_name(bridge_result));
    } else {
        ESP_LOGI(TAG, "[diag] subsystem.init subsystem=bridge_tc358743 status=%s implemented=true",
                 s_bridge.last_error == NULL ? "ok" : "degraded");
    }
    ESP_ERROR_CHECK(watchpup_capture_start(&s_capture));
    ESP_ERROR_CHECK(watchpup_diag_start(&s_health, &s_bridge, &s_capture));
}
