#include "diagnostics.h"

#include <stdio.h>
#include <stdlib.h>

#include "esp_check.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"

static const char *TAG = "watchpup_diag";
static const watchpup_board_health_t *s_health;

static const char *reset_reason_name(void)
{
    switch (esp_reset_reason()) {
    case ESP_RST_POWERON: return "power_on";
    case ESP_RST_SW: return "software";
    case ESP_RST_PANIC: return "panic";
    case ESP_RST_INT_WDT: return "interrupt_watchdog";
    case ESP_RST_TASK_WDT: return "task_watchdog";
    case ESP_RST_WDT: return "watchdog";
    case ESP_RST_BROWNOUT: return "brownout";
    default: return "other";
    }
}

static bool overall_healthy(void)
{
    return s_health->psram_test_passed && s_health->ethernet_link_up && s_health->ethernet_has_ipv4;
}

static esp_err_t diag_get(httpd_req_t *request)
{
    const int64_t uptime_ms = esp_timer_get_time() / 1000;
    const size_t response_size = 4096;
    char *response = malloc(response_size);
    if (response == NULL) return ESP_ERR_NO_MEM;
    const int written = snprintf(response, response_size,
        "{\"schema_version\":1,\"generated_at_ms\":%lld,\"uptime_ms\":%lld,"
        "\"overall_status\":\"%s\",\"firmware\":{\"project_name\":\"watchpup_kvm\","
        "\"firmware_version\":\"phase1-bench\",\"git_revision\":null,\"idf_version\":\"%s\","
        "\"target\":\"esp32p4\"},\"boot\":{\"boot_count\":1,\"reset_reason\":\"%s\",\"safe_mode\":false},"
        "\"subsystems\":{\"diagnostics\":{\"status\":\"ok\",\"implemented\":true,\"last_error\":null,"
        "\"details\":{\"endpoint_ready\":true,\"serial_logging_ready\":true}},"
        "\"board_config\":{\"status\":\"unavailable\",\"implemented\":false,\"last_error\":null,\"details\":{}},"
        "\"psram\":{\"status\":\"%s\",\"implemented\":true,\"last_error\":%s,\"details\":{\"psram_present\":%s,\"psram_size_bytes\":%u,\"psram_test_passed\":%s}},"
        "\"ethernet\":{\"status\":\"%s\",\"implemented\":true,\"last_error\":%s,\"details\":{\"initialized\":%s,\"link_up\":%s,\"dhcp_enabled\":true,\"ipv4_address\":%s%s%s}},"
        "\"bridge_tc358743\":{\"status\":\"unavailable\",\"implemented\":false,\"last_error\":null,\"details\":{}},"
        "\"capture\":{\"status\":\"unavailable\",\"implemented\":false,\"last_error\":null,\"details\":{}},"
        "\"encoder_mjpeg\":{\"status\":\"unavailable\",\"implemented\":false,\"last_error\":null,\"details\":{}},"
        "\"frame_store\":{\"status\":\"unavailable\",\"implemented\":false,\"last_error\":null,\"details\":{}},"
        "\"stream_server\":{\"status\":\"unavailable\",\"implemented\":false,\"last_error\":null,\"details\":{}},"
        "\"hid_device\":{\"status\":\"unavailable\",\"implemented\":false,\"last_error\":null,\"details\":{}},"
        "\"control_protocol\":{\"status\":\"unavailable\",\"implemented\":false,\"last_error\":null,\"details\":{}},"
        "\"auth_security\":{\"status\":\"unavailable\",\"implemented\":false,\"last_error\":null,\"details\":{}}},"
        "\"recovery\":{\"total_recoveries\":0,\"last_recovery_reason\":null}}",
        (long long)uptime_ms, (long long)uptime_ms, overall_healthy() ? "ok" : "degraded",
        esp_get_idf_version(), reset_reason_name(),
        s_health->psram_test_passed ? "ok" : "error", s_health->psram_test_passed ? "null" : "\"memory_test_failed\"",
        s_health->psram_size_bytes > 0 ? "true" : "false", (unsigned)s_health->psram_size_bytes,
        s_health->psram_test_passed ? "true" : "false",
        s_health->ethernet_init_failed ? "error" : (s_health->ethernet_has_ipv4 ? "ok" : "degraded"),
        s_health->ethernet_init_failed ? "\"initialization_failed\"" : "null",
        s_health->ethernet_initialized ? "true" : "false", s_health->ethernet_link_up ? "true" : "false",
        s_health->ethernet_has_ipv4 ? "\"" : "", s_health->ethernet_has_ipv4 ? s_health->ipv4_address : "null",
        s_health->ethernet_has_ipv4 ? "\"" : "");
    if (written < 0 || (size_t)written >= response_size) { free(response); return ESP_FAIL; }
    httpd_resp_set_type(request, "application/json");
    const esp_err_t result = httpd_resp_send(request, response, HTTPD_RESP_USE_STRLEN);
    free(response);
    return result;
}

void watchpup_diag_log_boot_start(void)
{
    ESP_LOGI(TAG, "[diag] boot.start project=watchpup_kvm version=phase1-bench idf=%s target=esp32p4 reset_reason=%s",
             esp_get_idf_version(), reset_reason_name());
}

esp_err_t watchpup_diag_start(const watchpup_board_health_t *health)
{
    s_health = health;
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    ESP_RETURN_ON_ERROR(httpd_start(&server, &config), TAG, "start diagnostics HTTP server");
    const httpd_uri_t route = {.uri = "/diag", .method = HTTP_GET, .handler = diag_get};
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &route), TAG, "register /diag");
    ESP_LOGI(TAG, "[diag] subsystem.init subsystem=diagnostics status=ok endpoint_ready=true serial_logging_ready=true");
    ESP_LOGI(TAG, "[diag] boot.ready diag_route=/diag overall_status=degraded");
    ESP_LOGI(TAG, "[diag] health.summary overall_status=degraded uptime_ms=%lld", (long long)(esp_timer_get_time() / 1000));
    return ESP_OK;
}
