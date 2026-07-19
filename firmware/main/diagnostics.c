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
static watchpup_capture_health_t *s_capture;

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

static const char *capture_status(void)
{
    if (s_capture == NULL || !s_capture->implemented) return "error";
    if (s_capture->last_error != NULL) return "degraded";
    if (s_capture->frame_count > 0) return "ok";
    return "degraded";
}

static const char *json_error(const char *error)
{
    static char json[2][64];
    static unsigned int slot;
    char *result = json[slot++ & 1U];
    if (error == NULL) return "null";
    snprintf(result, 64, "\"%s\"", error);
    return result;
}

static esp_err_t diag_get(httpd_req_t *request)
{
    if (s_bridge != NULL) (void)watchpup_tc358743_poll(s_bridge);
    if (s_capture != NULL) {
        const bool bridge_signal_locked = s_bridge != NULL && s_bridge->hdmi_signal_detected && s_bridge->hdmi_sync_locked;
        (void)watchpup_capture_poll(s_capture, bridge_signal_locked);
    }
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
        "\"bridge_tc358743\":{\"status\":\"%s\",\"implemented\":true,\"last_error\":%s,\"details\":{\"probe_ok\":%s,\"chip_id\":\"0x%04x\",\"driver_state\":\"%s\",\"reset_supported\":%s,\"hpd_asserted\":%s,\"edid_loaded\":%s,\"edid_readback_valid\":%s,\"edid_length_bytes\":%u,\"edid_revision\":\"%s\",\"hdmi_signal_detected\":%s,\"hdmi_sync_locked\":%s,\"negotiated_width\":%u,\"negotiated_height\":%u,\"negotiated_frame_rate\":%u,\"pll_locked\":%s,\"csi_output_enabled\":%s,\"i2c_port\":%d,\"i2c_sda_gpio\":%d,\"i2c_scl_gpio\":%d,\"i2c_address\":%d,\"configured_csi_lane_count\":%d,\"configured_csi_lane_rate_mbps\":%d,\"sys_status\":%u,\"csi_status\":%u,\"sysctl\":%u,\"pllctl0\":%u,\"pllctl1\":%u,\"edid_mode\":%u,\"phy_ctl0\":%u,\"phy_ctl1\":%u,\"phy_ctl2\":%u,\"phy_en\":%u,\"phy_rst\":%u,\"hdmi_det\":%u,\"hv_rst\":%u,\"ddc_ctl\":%u,\"hpd_ctl\":%u,\"ana_ctl\":%u,\"sys_int\":%u,\"clk_int\":%u,\"hdmi_int0\":%u,\"hdmi_int1\":%u,\"vi_status1\":%u,\"vi_status3\":%u,\"init_end\":%u,\"refclk_readback_valid\":%s,\"sys_freq0\":%u,\"sys_freq1\":%u,\"fh_min0\":%u,\"fh_min1\":%u,\"fh_max0\":%u,\"fh_max1\":%u,\"lockdet_ref0\":%u,\"lockdet_ref1\":%u,\"lockdet_ref2\":%u,\"nco_f0_mod\":%u,\"last_init_success_ms\":%lld,\"last_signal_lock_ms\":%lld,\"recovery_count\":%u,\"last_step\":\"%s\"}},"
        "\"capture\":{\"status\":\"%s\",\"implemented\":true,\"last_error\":%s,\"details\":{\"receiver_configured\":%s,\"receiver_started\":%s,\"frame_width\":%u,\"frame_height\":%u,\"frame_buffer_bytes\":%u,\"frame_count\":%u,\"last_received_bytes\":%u,\"frame_fingerprint\":%u,\"last_frame_ms\":%lld,\"driver_state\":\"%s\"}},"
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
        s_bridge != NULL && s_bridge->edid_readback_valid ? "true" : "false",
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
        s_bridge == NULL ? 0U : (unsigned)s_bridge->sysctl,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->pllctl0,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->pllctl1,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->edid_mode,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->phy_ctl0,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->phy_ctl1,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->phy_ctl2,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->phy_en,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->phy_rst,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->hdmi_det,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->hv_rst,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->ddc_ctl,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->hpd_ctl,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->ana_ctl,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->sys_int,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->clk_int,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->hdmi_int0,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->hdmi_int1,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->vi_status1,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->vi_status3,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->init_end,
        s_bridge != NULL && s_bridge->refclk_readback_valid ? "true" : "false",
        s_bridge == NULL ? 0U : (unsigned)s_bridge->sys_freq0,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->sys_freq1,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->fh_min0,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->fh_min1,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->fh_max0,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->fh_max1,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->lockdet_ref0,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->lockdet_ref1,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->lockdet_ref2,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->nco_f0_mod,
        s_bridge == NULL ? 0LL : (long long)s_bridge->last_init_success_ms,
        s_bridge == NULL ? 0LL : (long long)s_bridge->last_signal_lock_ms,
        s_bridge == NULL ? 0U : (unsigned)s_bridge->recovery_count,
        s_bridge == NULL ? "unavailable" : s_bridge->last_step,
        capture_status(), json_error(s_capture == NULL ? "capture_not_initialized" : s_capture->last_error),
        s_capture != NULL && s_capture->receiver_configured ? "true" : "false",
        s_capture != NULL && s_capture->receiver_started ? "true" : "false",
        s_capture == NULL ? 0U : (unsigned)s_capture->frame_width,
        s_capture == NULL ? 0U : (unsigned)s_capture->frame_height,
        s_capture == NULL ? 0U : (unsigned)s_capture->frame_buffer_bytes,
        s_capture == NULL ? 0U : (unsigned)s_capture->frame_count,
        s_capture == NULL ? 0U : (unsigned)s_capture->last_received_bytes,
        s_capture == NULL ? 0U : (unsigned)s_capture->frame_fingerprint,
        s_capture == NULL ? 0LL : (long long)s_capture->last_frame_ms,
        s_capture == NULL ? "unavailable" : s_capture->driver_state);
    if (written < 0 || (size_t)written >= response_size) { free(response); return ESP_FAIL; }
    httpd_resp_set_type(request, "application/json");
    const esp_err_t result = httpd_resp_send(request, response, HTTPD_RESP_USE_STRLEN);
    free(response);
    return result;
}

static esp_err_t bridge_retrain_get(httpd_req_t *request)
{
    if (s_bridge == NULL) {
        httpd_resp_set_status(request, "503 Service Unavailable");
        return httpd_resp_sendstr(request, "{\"status\":\"unavailable\"}");
    }

    const esp_err_t err = watchpup_tc358743_retrain(s_bridge);
    char response[128];
    snprintf(response, sizeof(response), "{\"status\":\"%s\",\"error_code\":%d}",
             err == ESP_OK ? "completed" : "failed", (int)err);
    httpd_resp_set_type(request, "application/json");
    if (err != ESP_OK) httpd_resp_set_status(request, "500 Internal Server Error");
    return httpd_resp_sendstr(request, response);
}

void watchpup_diag_log_boot_start(void)
{
    ESP_LOGI(TAG, "[diag] boot.start project=watchpup_kvm version=phase2-bridge idf=%s target=esp32p4 reset_reason=%s",
             esp_get_idf_version(), reset_reason_name());
}

esp_err_t watchpup_diag_start(const watchpup_board_health_t *health,
                              watchpup_tc358743_health_t *bridge,
                              watchpup_capture_health_t *capture)
{
    s_health = health;
    s_bridge = bridge;
    s_capture = capture;
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    ESP_RETURN_ON_ERROR(httpd_start(&server, &config), TAG, "start diagnostics HTTP server");
    const httpd_uri_t route = {.uri = "/diag", .method = HTTP_GET, .handler = diag_get};
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &route), TAG, "register /diag");
    const httpd_uri_t retrain_route = {.uri = "/bridge/retrain", .method = HTTP_GET, .handler = bridge_retrain_get};
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &retrain_route), TAG, "register /bridge/retrain");
    ESP_LOGI(TAG, "[diag] subsystem.init subsystem=diagnostics status=ok endpoint_ready=true serial_logging_ready=true");
    ESP_LOGI(TAG, "[diag] boot.ready diag_route=/diag overall_status=%s", overall_healthy() ? "ok" : "degraded");
    ESP_LOGI(TAG, "[diag] health.summary overall_status=%s uptime_ms=%lld", overall_healthy() ? "ok" : "degraded", (long long)(esp_timer_get_time() / 1000));
    return ESP_OK;
}
