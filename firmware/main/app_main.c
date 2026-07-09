#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"

static const char *TAG = "watchpup_boot";

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
    ESP_LOGI(TAG, "WatchPup KVM firmware skeleton booting");
    ESP_LOGI(TAG, "Target baseline: ESP32-P4 on ESP-IDF v6.0.2");

    ESP_ERROR_CHECK(watchpup_init_nvs());
    ESP_LOGI(TAG, "NVS initialized");

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_LOGI(TAG, "Network interface stack initialized");

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_LOGI(TAG, "Default event loop initialized");

    ESP_LOGI(TAG, "Diagnostics scaffolding placeholder ready");
    ESP_LOGI(TAG, "mDNS initialization placeholder ready");
    ESP_LOGI(TAG, "TinyUSB initialization placeholder ready");
    ESP_LOGI(TAG, "No capture, streaming, HID, or ATX behavior is enabled in this skeleton");
}
