#include "capture_csi.h"

#include <string.h>

#include "esp_cam_ctlr.h"
#include "esp_cam_ctlr_csi.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_attr.h"
#include "hal/cam_ctlr_types.h"

static const char *TAG = "watchpup_capture";
static esp_cam_ctlr_handle_t s_csi;
static const uint8_t *s_frame_buffer;

static bool IRAM_ATTR capture_on_frame(esp_cam_ctlr_handle_t handle, esp_cam_ctlr_trans_t *trans, void *user_data)
{
    (void)handle;
    watchpup_capture_health_t *health = (watchpup_capture_health_t *)user_data;
    health->frame_count++;
    health->last_received_bytes = (uint32_t)trans->received_size;
    return false;
}

static uint32_t capture_fingerprint(const uint8_t *buffer, size_t length)
{
    if (buffer == NULL || length == 0) return 0;
    const size_t sample_count = length < 256 ? length : 256;
    uint32_t hash = 2166136261U;
    for (size_t i = 0; i < sample_count; i++) {
        hash ^= buffer[i];
        hash *= 16777619U;
    }
    return hash;
}

esp_err_t watchpup_capture_start(watchpup_capture_health_t *health)
{
    if (health == NULL) return ESP_ERR_INVALID_ARG;
    memset(health, 0, sizeof(*health));
    health->implemented = true;
    health->frame_width = 1280;
    health->frame_height = 720;
    health->driver_state = "waiting_for_bridge";
    ESP_LOGI(TAG, "[diag] subsystem.init subsystem=capture status=ok implemented=true state=waiting_for_bridge");
    return ESP_OK;
}

esp_err_t watchpup_capture_poll(watchpup_capture_health_t *health, bool bridge_signal_locked)
{
    if (health == NULL || !health->implemented) return ESP_ERR_INVALID_STATE;

    if (!bridge_signal_locked) {
        health->driver_state = s_csi == NULL ? "waiting_for_bridge" : "receiver_idle";
        return ESP_OK;
    }

    if (s_csi == NULL) {
        const esp_cam_ctlr_csi_config_t config = {
            .ctlr_id = 0,
            .h_res = health->frame_width,
            .v_res = health->frame_height,
            .data_lane_num = 2,
            .lane_bit_rate_mbps = 972,
            .input_data_color_type = CAM_CTLR_COLOR_RGB888,
            .output_data_color_type = CAM_CTLR_COLOR_RGB888,
            .queue_items = 1,
            .byte_swap_en = false,
            .bk_buffer_dis = false,
        };
        esp_err_t err = esp_cam_new_csi_ctlr(&config, &s_csi);
        if (err != ESP_OK) {
            health->driver_state = "error";
            health->last_error = "csi_controller_create_failed";
            return err;
        }
        const void *frame_buffer = NULL;
        err = esp_cam_ctlr_get_frame_buffer(s_csi, 1, &frame_buffer);
        if (err != ESP_OK) {
            health->driver_state = "error";
            health->last_error = "csi_frame_buffer_unavailable";
            return err;
        }
        s_frame_buffer = (const uint8_t *)frame_buffer;
        size_t frame_buffer_bytes = 0;
        err = esp_cam_ctlr_get_frame_buffer_len(s_csi, &frame_buffer_bytes);
        if (err != ESP_OK) {
            health->driver_state = "error";
            health->last_error = "csi_frame_buffer_length_unavailable";
            return err;
        }
        health->frame_buffer_bytes = (uint32_t)frame_buffer_bytes;
        const esp_cam_ctlr_evt_cbs_t callbacks = {
            .on_get_new_trans = NULL,
            .on_trans_finished = capture_on_frame,
        };
        err = esp_cam_ctlr_register_event_callbacks(s_csi, &callbacks, health);
        if (err != ESP_OK) {
            health->driver_state = "error";
            health->last_error = "csi_callback_registration_failed";
            return err;
        }
        health->receiver_configured = true;
    }

    if (!health->receiver_started) {
        esp_err_t err = esp_cam_ctlr_enable(s_csi);
        if (err == ESP_OK) err = esp_cam_ctlr_start(s_csi);
        if (err != ESP_OK) {
            health->driver_state = "error";
            health->last_error = "csi_start_failed";
            return err;
        }
        health->receiver_started = true;
        health->driver_state = "receiving";
        ESP_LOGI(TAG, "[diag] capture.receiver status=ok width=%u height=%u lanes=2 lane_rate_mbps=972",
                 (unsigned)health->frame_width, (unsigned)health->frame_height);
    }

    if (health->frame_count > 0) {
        health->last_frame_ms = esp_timer_get_time() / 1000;
        health->frame_fingerprint = capture_fingerprint(s_frame_buffer, health->frame_buffer_bytes);
        health->last_error = NULL;
    }
    return ESP_OK;
}
