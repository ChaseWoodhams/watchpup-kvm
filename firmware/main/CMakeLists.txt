idf_component_register(
    SRCS "app_main.c" "board_config.c" "board_health.c" "bridge_tc358743.c" "capture_csi.c" "diagnostics.c"
    INCLUDE_DIRS "."
    REQUIRES
        nvs_flash
        esp_event
        esp_netif
        esp_timer
        esp_http_server
        esp_psram
        esp_eth
        esp_driver_gpio
        esp_driver_i2c
        esp_driver_cam
)
