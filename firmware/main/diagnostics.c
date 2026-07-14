#include "diagnostics.h"

#include <stdio.h>
#include <stdlib.h>

#include "esp_check.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "board_config.h"

static const char *TAG = "watchpup_diag";
static const watchpup_board_health_t *s_health;
static watchpup_tc358743_health_t *s_bridge;

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
    return s_health->psram_test_passed && s_health->ethernet_link_up && s_health->ethernet_has_ipv4 &&
        s_bridge != NULL && s_bridge->probe_ok && s_bridge->hdmi_signal_detected &&
        s_bridge->hdmi_sync_locked && s_bridge->pll_locked;
}

static const char *bridge_status(void)
{
    if (s_bridge == NULL || !s_bridge->probe_ok) return "error";
    if (s_bridge->last_error != NULL) return "degraded";
    return "ok";
}

static const char *json_error(const char *error)
{
    static char json[64];
    if (error == NULL) return "null";
    snprintf(json, sizeof(json), "\"%s\"", error);
    return json;
}

static esp_err_t diag_get(httpd_req_t *request)
{
    if (s_bridge != NULL) (void)watchpup_tc358743_poll(s_bridge);
    const int64_t uptime_ms = esp_timer_get_time() / 1000;
    const size_t response_size = 8192;
    char *response = malloc(response_size);
    if (response == NULL) return ESP_ERR_NO_MEM;
    const watchpup_board_config_t *board = &WATCHPUP_BOARD_CONFIG;
    const int written = snprintf(response, response_size,
        "{\"schema_version\":1,\"generated_at_ms\":%lld,\"uptime_ms\":%lld,"
        "\"overall_status\":\"%s\",\"firmware\":{\"project_name\":\"watchpup_kvm\","
        "\"firmware_version\":\"phase2-bridge\",\"git_revision\":null,\"idf_version\":\"%s\","
        "\"target\":\"esp32p4\"},\"boot\":{\"boot_count\":1,\"reset_reason\":\"%s\",\"safe_mode\":false},"
        "\"subsystems\":{\"diagnostics\":{\"status\":\"ok\",\"implemented\":true,\"last_error\":null,"
        "\"details\":{\"endpoint_ready\":true,\"serial_logging_ready\":true}},"
        "\"board_config\":{\"status\":\"ok\",\"implemented\":true,\"last_error\":null,\"details\":{\"board_id\":\"%s\",\"bridge_adapter_id\":\"%s\",\"bridge_i2c_port\":%d,\"bridge_i2c_sda_gpio\":%d,\"bridge_i2c_scl_gpio\":%d,\"bridge_i2c_address\":%d,\"configured_csi_lane_count\":%d,\"configured_csi_lane_rate_mbps\":%d,\"bridge_refclk_hz\":%d,\"bridge_reset_gpio\":%d}},"
        "\"psram\":{\"status\":\"%s\",\"implemented\":true,\"last_error\":%s,\"details\":{\"psram_present\":%s,\"psram_size_bytes\":%u,\"psram_test_passed\":%s}},"
        "\"ethernet\":{\"status\":\"%s\",\"implemented\":true,\"last_error\":%s,\"details\":{\"initialized\":%s,\"link_up\":%s,\"dhcp_enabled\":true,\"ipv4_address\":%s%s%s}},"
        "\"bridge_tc358743\":{\"status\":\"%s\",\"implemented\":true,\"last_error\":%s,\"details\":{\"probe_ok\":%s,\"chip_id\":\"0x%04x\",\"driver_state\":\"%s\",\"reset_supported\":%s,\"hpd_asserted\":%s,\"edid_loaded\":%s,\"edid_length_bytes\":%u,\"edid_revision\":\"%s\",\"hdmi_signal_detected\":%s,\"hdmi_sync_locked\":%s,\"negotiated_width\":%u,\"negotiated_height\":%u,\"negotiated_frame_rate\":%u,\"pll_locked\":%s,\"csi_output_enabled\":%s,\"i2c_port\":%d,\"i2c_sda_gpio\":%d,\"i2c_scl_gpio\":%d,\"i2c_address\":%d,\"configured_csi_lane_count\":%d,\"configured_csi_lane_rate_mbps\":%d,\"sys_status\":%u,\"csi_status\":%u,\"phy_en\":%u,\"phy_rst\":%u,\"hdmi_det\":%u,\"hv_rst\":%u,\"ddc_ctl\":%u,\"hpd_ctl\":%u,\"ana_ctl\":%u,\"last_init_success_ms\":%lld,\"last_signal_lock_ms\":%lld,\"recovery_count\":%u,\"last_step\":\"%s\"}},"
        "\"capture\":{\"status\":\"unavailable\",\"implemented\":false,\"last_error\":null,\"details\":{}},"
        "\"encoder_mjpeg\":{\"status\":\"unavailable\",\"implemented\":false,\"last_error\":null,\"details\":{}},"
        "\"frame_store\":{\"status\":\"unavailable\",\"implemented\":false,\"last_error\":null,\"details\":{}},"
        "\"stream_server\":{\"status\":\"unavailable\",\"implemented\":false,\"last_error\":null,\"details\":{}},"
        "\"hid_device\":{\"status\":\"unavailable\",\"implemented\":false,\"last_error\":null,\"details\":{}},"
        "\"control_protocol\":{\"status\":\"unavailable\",\"implemented\":false,\"last_error\":null,\"details\":{}},"
        "\"auth_security\":{\"status\":\"unavailable\",\"implemented\":false,\"last_error\":null,\"details\":{}}},"
        "\"recovery\":{\"total_recoveries\":0,\"last_recovery_reason\":null}}",
        (long long)uptime_ms, (long long)uptime_ms, overall_healthy() ? "ok" : "degraded",
        esp_get_idf_version(), reset_reason_name(), board->board_id, board->bridge_adapter_id,
        board->bridge_i2c_port, board->bridge_i2c_sda_gpio, board->bridge_i2c_scl_gpio,
        board->bridge_i2c_address, board->configured_csi_lane_count, board->configured_csi_lane_rate_mbps,
        board->bridge_refclk_hz, board->bridge_reset_gpio,
        s_health->psram_test_passed ? "ok" : "error", s_health->psram_test_passed ? "null" : "\"memory_test_failed\"",
        s_health->psram_size_bytes > 0 ? "true" : "false", (unsigned)s_health->psram_size_bytes,
        s_health->psram_test_passed ? "true" : "false",
        s_health->ethernet_init_failed ? "error" : (s_health->ethernet_has_ipv4 ? "ok" : "degraded"),
        s_health->ethernet_init_failed ? "\"initialization_failed\"" : "null",
        s_health->ethernet_initialized ? "true" : "false", s_health->ethernet_link_up ? "true" : "false",
        s_health->ethernet_has_ipv4 ? "\"" : "", s_health->ethernet_has_ipv4 ? s_health->ipv4_address : "null",
        s_health->ethernet_has_ipv4 ? "\"" : "",
        bridge_status(), json_error(s_bridge == NULL ? "bridge_not_initialized" : s_bridge->last_error),
        s_bridge != NULL && s_bridge->probe_ok ? "true" : "false", s_bridge == NULL ? 0 : s_bridge->chip_id,
        s_bridge == NULL ? "unavailable" : s_bridge->driver_state,
        s_bridge != NULL && s_bridge->reset_supported ? "true" : "false",
        s_bridge != NULL && s_bridge->hpd_asserted ? "true" : "false",
        s_bridge != NULL && s_bridge->edid_loaded ? "true" : "false",
        s_bridge == NULL ? 0U : (unsigned)s_bridge->edid_length_bytes,
        s_bridge == NULL ? "unavailable" : s_bridge->edid_revision,
        s_bridge != NULL && s_bridge->hdmi_signal_detected ? "true" : "false",
        s_bridge != NULL && s_bridge->hdmi_sync_locked ? "true" : "false",
        s_bridge == NULL ? 0 : s_bridge->negotiated_width, s_bridge == NULL ? 0 : s_bridge->negotiated_height,
        s_bridge == NULL ? 0 : s_bridge->negotiated_frame_rate,
        s_bridge != NULL && s_bridge->pll_locked ? "true" : "false",
        s_bridge != NULL && s_bridge->csi_output_enabled ? "true" : "false",
        s_bridge == NULL ? 0 : s_bridge->i2c_port, s_bridge == NULL ? 0 : s_bridge->i2c_sda_gpio,
        s_bridge == NULL ? 0 : s_bridge->i2c_scl_gpio, s_bridge == NULL ? 0 : s_bridge->i2c_address,
        s_bridge == NULL ? 0 : s_bridge->configured_csi_lane_count,
        s_bridge == NULL ? 0 : s_bridge->configured_csi_lane_rate_mbps,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->sys_status,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->csi_status,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->phy_en,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->phy_rst,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->hdmi_det,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->hv_rst,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->ddc_ctl,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->hpd_ctl,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->ana_ctl,
        s_bridge == NULL ? 0LL : (long long)s_bridge->last_init_success_ms,
        s_bridge == NULL ? 0LL : (long long)s_bridge->last_signal_lock_ms,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->recovery_count,
        s_bridge == NULL ? "unavailable" : s_bridge->last_step);
    if (written < 0 || (size_t)written >= response_size) { free(response); return ESP_FAIL; }
    httpd_resp_set_type(request, "application/json");
    const esp_err_t result = httpd_resp_send(request, response, HTTPD_RESP_USE_STRLEN);
    free(response);
    return result;
}

void watchpup_diag_log_boot_start(void)
{
    ESP_LOGI(TAG, "[diag] boot.start project=watchpup_kvm version=phase2-bridge idf=%s target=esp32p4 reset_reason=%s",
             esp_get_idf_version(), reset_reason_name());
}

esp_err_t watchpup_diag_start(const watchpup_board_health_t *health,
                              watchpup_tc358743_health_t *bridge)
{
    s_health = health;
    s_bridge = bridge;
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    ESP_RETURN_ON_ERROR(httpd_start(&server, &config), TAG, "start diagnostics HTTP server");
    const httpd_uri_t route = {.uri = "/diag", .method = HTTP_GET, .handler = diag_get};
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &route), TAG, "register /diag");
    ESP_LOGI(TAG, "[diag] subsystem.init subsystem=diagnostics status=ok endpoint_ready=true serial_logging_ready=true");
    ESP_LOGI(TAG, "[diag] boot.ready diag_route=/diag overall_status=%s", overall_healthy() ? "ok" : "degraded");
    ESP_LOGI(TAG, "[diag] health.summary overall_status=%s uptime_ms=%lld", overall_healthy() ? "ok" : "degraded", (long long)(esp_timer_get_time() / 1000));
    return ESP_OK;
}
