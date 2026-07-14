#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

typedef struct {
    bool implemented;
    bool probe_ok;
    uint16_t chip_id;
    int i2c_port;
    int i2c_sda_gpio;
    int i2c_scl_gpio;
    int i2c_address;
    int configured_csi_lane_count;
    int configured_csi_lane_rate_mbps;
    bool reset_supported;
    bool hpd_asserted;
    bool edid_loaded;
    uint16_t edid_length_bytes;
    const char *edid_revision;
    bool hdmi_signal_detected;
    bool hdmi_sync_locked;
    uint16_t negotiated_width;
    uint16_t negotiated_height;
    uint16_t negotiated_frame_rate;
    bool pll_locked;
    bool csi_output_enabled;
    uint32_t sys_status;
    uint32_t csi_status;
    uint8_t phy_en;
    uint8_t phy_rst;
    uint8_t hdmi_det;
    uint8_t hv_rst;
    uint8_t ddc_ctl;
    uint8_t hpd_ctl;
    uint8_t ana_ctl;
    int64_t last_init_success_ms;
    int64_t last_signal_lock_ms;
    uint32_t recovery_count;
    const char *driver_state;
    const char *last_error;
    const char *last_step;
} watchpup_tc358743_health_t;

esp_err_t watchpup_tc358743_start(watchpup_tc358743_health_t *health);
esp_err_t watchpup_tc358743_poll(watchpup_tc358743_health_t *health);
