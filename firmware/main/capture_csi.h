#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

typedef struct {
    bool implemented;
    bool receiver_configured;
    bool receiver_started;
    uint32_t frame_count;
    uint32_t frame_width;
    uint32_t frame_height;
    uint32_t frame_buffer_bytes;
    uint32_t last_received_bytes;
    uint32_t frame_fingerprint;
    int64_t last_frame_ms;
    const char *driver_state;
    const char *last_error;
} watchpup_capture_health_t;

esp_err_t watchpup_capture_start(watchpup_capture_health_t *health);
esp_err_t watchpup_capture_poll(watchpup_capture_health_t *health, bool bridge_signal_locked);
