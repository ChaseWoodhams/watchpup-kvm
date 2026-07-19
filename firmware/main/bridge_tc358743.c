#include "bridge_tc358743.h"

#include <stdlib.h>
#include <string.h>

#include "board_config.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "watchpup_bridge";

#define TC358743_CHIPID 0x0000
#define TC358743_SYSCTL 0x0002
#define TC358743_SYSCTL_SLEEP 0x0001
#define TC358743_CONFCTL 0x0004
#define TC358743_FIFOCTL 0x0006
#define TC358743_CLW_CNTRL 0x0140
#define TC358743_D0W_CNTRL 0x0144
#define TC358743_D1W_CNTRL 0x0148
#define TC358743_STARTCNTRL 0x0204
#define TC358743_LINEINITCNT 0x0210
#define TC358743_LPTXTIMECNT 0x0214
#define TC358743_TCLK_HEADERCNT 0x0218
#define TC358743_TCLK_TRAILCNT 0x021C
#define TC358743_THS_HEADERCNT 0x0220
#define TC358743_TWAKEUP 0x0224
#define TC358743_TCLK_POSTCNT 0x0228
#define TC358743_THS_TRAILCNT 0x022C
#define TC358743_HSTXVREGCNT 0x0230
#define TC358743_HSTXVREGEN 0x0234
#define TC358743_TXOPTIONCNTRL 0x0238
#define TC358743_CSI_CONTROL 0x040C
#define TC358743_SYS_STATUS 0x8520
#define TC358743_CSI_STATUS 0x0410
#define TC358743_CSI_INT_ENA 0x0418
#define TC358743_CSI_ERR_INTENA 0x0450
#define TC358743_CSI_ERR_HALT 0x0454
#define TC358743_CSI_CONFW 0x0500
#define TC358743_CSI_START 0x0518
#define TC358743_PHY_CTL0 0x8531
#define TC358743_PHY_CTL1 0x8532
#define TC358743_PHY_CTL2 0x8533
#define TC358743_PHY_EN 0x8534
#define TC358743_PHY_RST 0x8535
#define TC358743_PHY_BIAS 0x8536
#define TC358743_PHY_CSQ 0x853F
#define TC358743_HPD_CTL 0x8544
#define TC358743_DDC_CTL 0x8543
#define TC358743_ANA_CTL 0x8545
#define TC358743_AVM_CTL 0x8546
#define TC358743_HDMI_DET 0x8552
#define TC358743_HDCP_MODE 0x8560
#define TC358743_HV_RST 0x85AF
#define TC358743_EDID_MODE 0x85C7
#define TC358743_EDID_LEN1 0x85CA
#define TC358743_EDID_LEN2 0x85CB
#define TC358743_EDID_RAM 0x8C00
#define TC358743_SYS_FREQ0 0x8540
#define TC358743_SYS_FREQ1 0x8541
#define TC358743_PLLCTL0 0x0020
#define TC358743_PLLCTL1 0x0022
#define TC358743_HDMI_INT0 0x8500
#define TC358743_HDMI_INT1 0x8501
#define TC358743_SYS_INT 0x8502
#define TC358743_CLK_INT 0x8503
#define TC358743_INIT_END 0x854A
#define TC358743_VI_STATUS1 0x8522
#define TC358743_VI_STATUS3 0x8528
#define TC358743_FH_MIN0 0x85AA
#define TC358743_FH_MIN1 0x85AB
#define TC358743_FH_MAX0 0x85AC
#define TC358743_FH_MAX1 0x85AD
#define TC358743_LOCKDET_REF0 0x8630
#define TC358743_LOCKDET_REF1 0x8631
#define TC358743_LOCKDET_REF2 0x8632
#define TC358743_NCO_F0_MOD 0x8670
#define TC358743_CECHCLK 0x0028
#define TC358743_CECLCLK 0x002A
#define TC358743_VI_MODE 0x8570
#define TC358743_VOUT_SET2 0x8573
#define TC358743_VOUT_SET3 0x8574
#define TC358743_VI_MUTE 0x857F
#define TC358743_DE_WIDTH_H_LO 0x8582
#define TC358743_DE_WIDTH_H_HI 0x8583
#define TC358743_DE_WIDTH_V_LO 0x8588
#define TC358743_DE_WIDTH_V_HI 0x8589
#define TC358743_FV_CNT_LO 0x85A1
#define TC358743_FV_CNT_HI 0x85A2

#define SYS_STATUS_SYNC 0x80
#define SYS_STATUS_PHY_PLL 0x04
#define SYS_STATUS_TMDS 0x02
#define CSI_STATUS_TXACT 0x0200
#define SYS_INT_DVI 0x20
#define SYS_INT_HDMI 0x10

#define TC358743_PLLCTL0_972MHZ 0x308F
#define TC358743_PLLCTL1_FRS_MASK 0x0C00
#define TC358743_PLLCTL1_CKEN 0x0010
#define TC358743_PLLCTL1_RESETB 0x0002
#define TC358743_PLLCTL1_PLL_EN 0x0001
#define TC358743_CSI_CONFW_MODE_SET 0xA0000000
#define TC358743_CSI_CONFW_MODE_CLEAR 0xC0000000
#define TC358743_CSI_CONFW_ADDR_CONTROL 0x03000000
#define TC358743_CSI_CONFW_ADDR_INT_ENA 0x06000000
#define TC358743_CSI_CONFW_ADDR_ERR_INTENA 0x14000000
#define TC358743_CSI_CONFW_ADDR_ERR_HALT 0x15000000
#define TC358743_CSI_CONTROL_MODE 0x8000
#define TC358743_CSI_CONTROL_TXHSMD 0x0080
#define TC358743_CSI_CONTROL_LANES_2 0x0002
#define TC358743_CSI_ERR_INT_ENABLE 0x00000212
#define TC358743_CSI_ERR_HALT_MASK 0x00000012
#define TC358743_CSI_INT_ENABLE 0x00000004

#define EDID_MODE_E_DDC 0x02
#define HDCP_MANUAL_AUTHENTICATION 0x02
#define HPD_OUT 0x01

/* Waveshare's official 720p60 bring-up EDID from its driver package. */
static const uint8_t s_edid_720p60[256] = {
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x52, 0x62, 0x88, 0x88, 0x00, 0x88, 0x88, 0x88,
    0x1C, 0x15, 0x01, 0x03, 0x80, 0xA0, 0x5A, 0x78, 0x0A, 0x0D, 0xC9, 0xA0, 0x57, 0x47, 0x98, 0x27,
    0x12, 0x48, 0x4C, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x1D, 0x00, 0x72, 0x51, 0xD0, 0x1E, 0x20, 0x6E, 0x28,
    0x55, 0x00, 0x80, 0x38, 0x74, 0x00, 0x00, 0x1E, 0x01, 0x1D, 0x80, 0x18, 0x71, 0x38, 0x2D, 0x40,
    0x58, 0x2C, 0x45, 0x00, 0x80, 0x38, 0x74, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x44,
    0x43, 0x44, 0x5A, 0x2D, 0x48, 0x32, 0x43, 0x20, 0x4D, 0x4F, 0x44, 0x0A, 0x00, 0x00, 0x00, 0xFD,
    0x00, 0x14, 0x78, 0x01, 0xFF, 0x10, 0x00, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0x74,
    0x02, 0x03, 0x1A, 0x71, 0x47, 0x84, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x23, 0x09, 0x07, 0x01,
    0x83, 0x01, 0x00, 0x00, 0x65, 0x03, 0x0C, 0x00, 0x10, 0x00, 0x01, 0x1D, 0x80, 0x18, 0x71, 0x38,
    0x2D, 0x40, 0x58, 0x2C, 0x45, 0x00, 0x80, 0x38, 0x74, 0x00, 0x00, 0x1E, 0x01, 0x1D, 0x80, 0x18,
    0x71, 0x38, 0x2D, 0x40, 0x58, 0x2C, 0x45, 0x00, 0x80, 0x38, 0x74, 0x00, 0x00, 0x1E, 0x01, 0x1D,
    0x80, 0x18, 0x71, 0x38, 0x2D, 0x40, 0x58, 0x2C, 0x45, 0x00, 0x80, 0x38, 0x74, 0x00, 0x00, 0x1E,
    0x01, 0x1D, 0x80, 0x18, 0x71, 0x38, 0x2D, 0x40, 0x58, 0x2C, 0x45, 0x00, 0x80, 0x38, 0x74, 0x00,
    0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21,
};

static i2c_master_bus_handle_t s_bus;
static i2c_master_dev_handle_t s_device;
static bool s_csi_configured;

static void bridge_fail(watchpup_tc358743_health_t *health, const char *step, const char *error)
{
    health->driver_state = "error";
    health->last_step = step;
    health->last_error = error;
    ESP_LOGE(TAG, "[diag] subsystem.health subsystem=bridge_tc358743 status=error step=%s error=%s",
             step, error);
}

static esp_err_t bridge_write(uint16_t reg, const uint8_t *data, size_t length)
{
    uint8_t *buffer = malloc(length + 2);
    if (buffer == NULL) return ESP_ERR_NO_MEM;
    buffer[0] = (uint8_t)(reg >> 8);
    buffer[1] = (uint8_t)reg;
    memcpy(&buffer[2], data, length);
    const esp_err_t err = i2c_master_transmit(s_device, buffer, length + 2, 100);
    free(buffer);
    return err;
}

static esp_err_t bridge_read(uint16_t reg, uint8_t *data, size_t length)
{
    const uint8_t address[2] = {(uint8_t)(reg >> 8), (uint8_t)reg};
    return i2c_master_transmit_receive(s_device, address, sizeof(address), data, length, 100);
}

static esp_err_t bridge_read_with_stop(uint16_t reg, uint8_t *data, size_t length)
{
    const uint8_t address[2] = {(uint8_t)(reg >> 8), (uint8_t)reg};
    esp_err_t err = i2c_master_transmit(s_device, address, sizeof(address), 100);
    if (err == ESP_OK) err = i2c_master_receive(s_device, data, length, 100);
    return err;
}

static esp_err_t bridge_write8(uint16_t reg, uint8_t value)
{
    return bridge_write(reg, &value, sizeof(value));
}

static esp_err_t bridge_write16(uint16_t reg, uint16_t value)
{
    const uint8_t data[2] = {(uint8_t)value, (uint8_t)(value >> 8)};
    return bridge_write(reg, data, sizeof(data));
}

static esp_err_t bridge_write32(uint16_t reg, uint32_t value)
{
    const uint8_t data[4] = {
        (uint8_t)value,
        (uint8_t)(value >> 8),
        (uint8_t)(value >> 16),
        (uint8_t)(value >> 24),
    };
    return bridge_write(reg, data, sizeof(data));
}

static esp_err_t bridge_read8(uint16_t reg, uint8_t *value)
{
    return bridge_read(reg, value, sizeof(*value));
}

static esp_err_t bridge_read16(uint16_t reg, uint16_t *value)
{
    uint8_t data[2];
    const esp_err_t err = bridge_read(reg, data, sizeof(data));
    if (err == ESP_OK) *value = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
    return err;
}

static esp_err_t bridge_update8(uint16_t reg, uint8_t clear_mask, uint8_t set_bits)
{
    uint8_t value = 0;
    esp_err_t err = bridge_read8(reg, &value);
    if (err == ESP_OK) err = bridge_write8(reg, (uint8_t)((value & (uint8_t)~clear_mask) | set_bits));
    return err;
}

static esp_err_t bridge_reset_hdmi_phy(void)
{
    /* The upstream driver uses a low-to-high transition on PHY_RST when the
     * HDMI source changes state or remains out of sync. Keep this recovery
     * one-shot so a bad source cannot turn the poll endpoint into a reset
     * loop. */
    esp_err_t err = bridge_update8(TC358743_PHY_RST, 0x01, 0x00);
    if (err == ESP_OK) err = bridge_update8(TC358743_PHY_RST, 0x01, 0x01);
    if (err == ESP_OK) err = bridge_write8(TC358743_ANA_CTL, 0x31);
    return err;
}

static bool bridge_refclk_values_match(const watchpup_board_config_t *board,
                                       const watchpup_tc358743_health_t *health)
{
    const uint32_t sys_freq = (uint32_t)board->bridge_refclk_hz / 10000;
    const uint32_t lockdet_ref = (uint32_t)board->bridge_refclk_hz / 100;
    const uint16_t fh_min = (uint16_t)((uint32_t)board->bridge_refclk_hz / 100000);
    const uint16_t fh_max = (uint16_t)(((uint32_t)fh_min * 66) / 10);
    return health->sys_freq0 == (uint8_t)sys_freq &&
           health->sys_freq1 == (uint8_t)(sys_freq >> 8) &&
           health->fh_min0 == (uint8_t)fh_min &&
           health->fh_min1 == (uint8_t)(fh_min >> 8) &&
           health->fh_max0 == (uint8_t)fh_max &&
           health->fh_max1 == (uint8_t)(fh_max >> 8) &&
           health->lockdet_ref0 == (uint8_t)lockdet_ref &&
           health->lockdet_ref1 == (uint8_t)(lockdet_ref >> 8) &&
           health->lockdet_ref2 == (uint8_t)(lockdet_ref >> 16) &&
           health->nco_f0_mod == (board->bridge_refclk_hz == 27000000 ? 0x01 : 0x00);
}

static esp_err_t bridge_refresh_refclk_health(const watchpup_board_config_t *board,
                                              watchpup_tc358743_health_t *health,
                                              const char *stage)
{
    esp_err_t err = bridge_read8(TC358743_SYS_FREQ0, &health->sys_freq0);
    if (err == ESP_OK) err = bridge_read8(TC358743_SYS_FREQ1, &health->sys_freq1);
    if (err == ESP_OK) err = bridge_read8(TC358743_FH_MIN0, &health->fh_min0);
    if (err == ESP_OK) err = bridge_read8(TC358743_FH_MIN1, &health->fh_min1);
    if (err == ESP_OK) err = bridge_read8(TC358743_FH_MAX0, &health->fh_max0);
    if (err == ESP_OK) err = bridge_read8(TC358743_FH_MAX1, &health->fh_max1);
    if (err == ESP_OK) err = bridge_read8(TC358743_LOCKDET_REF0, &health->lockdet_ref0);
    if (err == ESP_OK) err = bridge_read8(TC358743_LOCKDET_REF1, &health->lockdet_ref1);
    if (err == ESP_OK) err = bridge_read8(TC358743_LOCKDET_REF2, &health->lockdet_ref2);
    if (err == ESP_OK) err = bridge_read8(TC358743_NCO_F0_MOD, &health->nco_f0_mod);
    health->refclk_readback_valid = err == ESP_OK && bridge_refclk_values_match(board, health);
    ESP_LOGI(TAG, "[diag] bridge.refclk stage=%s status=%s sys_freq=0x%02x%02x fh_min=0x%02x%02x fh_max=0x%02x%02x lockdet_ref=0x%02x%02x%02x nco_f0_mod=0x%02x",
             stage, err == ESP_OK && health->refclk_readback_valid ? "ok" : "mismatch",
             health->sys_freq1, health->sys_freq0, health->fh_min1, health->fh_min0,
             health->fh_max1, health->fh_max0, health->lockdet_ref2, health->lockdet_ref1,
             health->lockdet_ref0, health->nco_f0_mod);
    return err == ESP_OK && health->refclk_readback_valid ? ESP_OK : (err == ESP_OK ? ESP_ERR_INVALID_RESPONSE : err);
}

static esp_err_t bridge_configure_source_path(const watchpup_board_config_t *board)
{
    esp_err_t err;
    uint16_t sysctl = 0;
    uint32_t sys_freq = (uint32_t)board->bridge_refclk_hz / 10000;
    uint32_t lockdet_ref = (uint32_t)board->bridge_refclk_hz / 100;
    uint16_t fh_min = (uint16_t)((uint32_t)board->bridge_refclk_hz / 100000);
    uint16_t fh_max = (uint16_t)(((uint32_t)fh_min * 66) / 10);
    uint16_t cec_freq = (uint16_t)((656U * sys_freq) / 4200U);

    err = bridge_read16(TC358743_SYSCTL, &sysctl);
    if (err != ESP_OK) return err;
    /* Match the Linux driver: keep IR/CEC in reset and pulse only the
     * context/HDMI reset bits before leaving sleep mode. */
    const uint16_t base_sysctl = (uint16_t)((sysctl & (uint16_t)~TC358743_SYSCTL_SLEEP) | 0x0c00);
    err = bridge_write16(TC358743_SYSCTL, base_sysctl);
    if (err != ESP_OK) return err;
    err = bridge_write16(TC358743_SYSCTL, (uint16_t)(base_sysctl | 0x0300));
    if (err != ESP_OK) return err;
    err = bridge_write16(TC358743_SYSCTL, base_sysctl);
    if (err != ESP_OK) return err;
    err = bridge_write16(TC358743_FIFOCTL, 374);
    if (err != ESP_OK) return err;

    err = bridge_write8(TC358743_SYS_FREQ0, (uint8_t)sys_freq);
    if (err == ESP_OK) err = bridge_write8(TC358743_SYS_FREQ1, (uint8_t)(sys_freq >> 8));
    if (err == ESP_OK) err = bridge_write8(TC358743_FH_MIN0, (uint8_t)fh_min);
    if (err == ESP_OK) err = bridge_write8(TC358743_FH_MIN1, (uint8_t)(fh_min >> 8));
    if (err == ESP_OK) err = bridge_write8(TC358743_FH_MAX0, (uint8_t)fh_max);
    if (err == ESP_OK) err = bridge_write8(TC358743_FH_MAX1, (uint8_t)(fh_max >> 8));
    if (err == ESP_OK) err = bridge_write8(TC358743_LOCKDET_REF0, (uint8_t)lockdet_ref);
    if (err == ESP_OK) err = bridge_write8(TC358743_LOCKDET_REF1, (uint8_t)(lockdet_ref >> 8));
    if (err == ESP_OK) err = bridge_write8(TC358743_LOCKDET_REF2, (uint8_t)(lockdet_ref >> 16));
    if (err == ESP_OK) err = bridge_write8(TC358743_NCO_F0_MOD, board->bridge_refclk_hz == 27000000 ? 0x01 : 0x00);
    if (err == ESP_OK) err = bridge_write16(TC358743_CECHCLK, cec_freq);
    if (err == ESP_OK) err = bridge_write16(TC358743_CECLCLK, cec_freq);
    if (err == ESP_OK) err = bridge_update8(TC358743_DDC_CTL, 0x03, 0x02);
    if (err == ESP_OK) err = bridge_update8(TC358743_EDID_MODE, 0x03, EDID_MODE_E_DDC);
    if (err != ESP_OK) return err;

    err = bridge_write8(TC358743_PHY_EN, 0x00);
    if (err == ESP_OK) err = bridge_update8(TC358743_PHY_CTL0, 0x02, board->bridge_refclk_hz == 27000000 ? 0x00 : 0x02);
    if (err == ESP_OK) err = bridge_write8(TC358743_PHY_CTL1, 0x80);
    /* Keep the optional automatic PHY resets disabled, matching the Linux
     * reference defaults; recovery is handled explicitly from latched events. */
    if (err == ESP_OK) err = bridge_write8(TC358743_PHY_CTL2, 0x00);
    if (err == ESP_OK) err = bridge_write8(TC358743_PHY_BIAS, 0x40);
    if (err == ESP_OK) err = bridge_write8(TC358743_PHY_CSQ, 0x0a);
    if (err == ESP_OK) err = bridge_write8(TC358743_AVM_CTL, 45);
    if (err == ESP_OK) err = bridge_update8(TC358743_HDMI_DET, 0x30, 0x00);
    if (err == ESP_OK) err = bridge_write8(TC358743_ANA_CTL, 0x31);
    if (err == ESP_OK) err = bridge_update8(TC358743_HV_RST, 0x30, 0x00);
    /* The adapter does not support HDCP; match the upstream driver's
     * enable_hdcp=false default so a source cannot start authentication. */
    if (err == ESP_OK) err = bridge_update8(TC358743_HDCP_MODE, HDCP_MANUAL_AUTHENTICATION,
                                             HDCP_MANUAL_AUTHENTICATION);
    if (err == ESP_OK) err = bridge_write8(TC358743_PHY_EN, 0x01);
    if (err != ESP_OK) return err;

    err = bridge_update8(TC358743_VI_MODE, 0x08, 0x00);
    if (err == ESP_OK) err = bridge_update8(TC358743_VOUT_SET2, 0x03, 0x01);
    if (err == ESP_OK) err = bridge_write8(TC358743_VOUT_SET3, 0x08);
    return err;
}

static esp_err_t bridge_load_edid(watchpup_tc358743_health_t *health)
{
    health->last_step = "load_edid";
    const uint16_t edid_length = (uint16_t)sizeof(s_edid_720p60);
    /* HPD is already low, which disables DDC access while changing the RAM. */
    esp_err_t err = bridge_write8(TC358743_EDID_LEN1, (uint8_t)edid_length);
    if (err == ESP_OK) err = bridge_write8(TC358743_EDID_LEN2, (uint8_t)(edid_length >> 8));
    if (err != ESP_OK) return err;

    /* Diagnostic fallback: test whether this bridge accepts EDID RAM only as
     * single-byte register writes on the ESP32 I2C controller. */
    for (size_t offset = 0; offset < sizeof(s_edid_720p60); offset++) {
        err = bridge_write((uint16_t)(TC358743_EDID_RAM + offset), &s_edid_720p60[offset], 1);
        if (err != ESP_OK) return err;
    }
    uint8_t length_lo = 0;
    uint8_t length_hi = 0;
    if (bridge_read8(TC358743_EDID_LEN1, &length_lo) != ESP_OK ||
        bridge_read8(TC358743_EDID_LEN2, &length_hi) != ESP_OK ||
        (((uint16_t)length_hi << 8) | length_lo) != edid_length) {
        return ESP_FAIL;
    }
    uint8_t mode = 0;
    if (bridge_read8(TC358743_EDID_MODE, &mode) != ESP_OK || (mode & 0x03) != EDID_MODE_E_DDC) {
        return ESP_FAIL;
    }
    health->edid_loaded = true;
    health->edid_length_bytes = edid_length;
    health->edid_readback_valid = true;
    for (size_t offset = 0; offset < sizeof(s_edid_720p60); offset += 128) {
        uint8_t readback[128];
        const esp_err_t read_err = bridge_read_with_stop((uint16_t)(TC358743_EDID_RAM + offset), readback, sizeof(readback));
        if (read_err != ESP_OK || memcmp(readback, &s_edid_720p60[offset], sizeof(readback)) != 0) {
            health->edid_readback_valid = false;
            ESP_LOGW(TAG, "[diag] bridge.edid readback_mismatch offset=%u err=%s actual=%02x %02x %02x %02x %02x %02x %02x %02x expected=%02x %02x %02x %02x %02x %02x %02x %02x",
                     (unsigned)offset, esp_err_to_name(read_err), readback[0], readback[1], readback[2], readback[3], readback[4], readback[5], readback[6], readback[7],
                     s_edid_720p60[offset], s_edid_720p60[offset + 1], s_edid_720p60[offset + 2], s_edid_720p60[offset + 3],
                     s_edid_720p60[offset + 4], s_edid_720p60[offset + 5], s_edid_720p60[offset + 6], s_edid_720p60[offset + 7]);
            if (read_err == ESP_OK && offset == 0) {
                uint8_t probes[8] = {0};
                const uint16_t probe_offsets[8] = {0, 1, 2, 3, 4, 7, 127, 128};
                for (size_t probe = 0; probe < 8; probe++) {
                    (void)bridge_read8((uint16_t)(TC358743_EDID_RAM + probe_offsets[probe]), &probes[probe]);
                }
                ESP_LOGW(TAG, "[diag] bridge.edid probes=%02x %02x %02x %02x %02x %02x %02x %02x",
                         probes[0], probes[1], probes[2], probes[3], probes[4], probes[5], probes[6], probes[7]);
            }
            break;
        }
    }
    return ESP_OK;
}

static esp_err_t bridge_configure_csi_output(void)
{
    esp_err_t err;
    uint16_t sysctl = 0;
    uint16_t pllctl1 = 0;
    uint16_t confctl = 0;

    err = bridge_read16(TC358743_SYSCTL, &sysctl);
    if (err == ESP_OK) err = bridge_write16(TC358743_SYSCTL, (uint16_t)(sysctl | TC358743_SYSCTL_SLEEP));
    if (err == ESP_OK) err = bridge_write16(TC358743_PLLCTL0, TC358743_PLLCTL0_972MHZ);
    if (err == ESP_OK) err = bridge_read16(TC358743_PLLCTL1, &pllctl1);
    if (err == ESP_OK) {
        pllctl1 = (uint16_t)((pllctl1 & (uint16_t)~(TC358743_PLLCTL1_FRS_MASK |
                                                      TC358743_PLLCTL1_RESETB |
                                                      TC358743_PLLCTL1_PLL_EN)) |
                             TC358743_PLLCTL1_RESETB | TC358743_PLLCTL1_PLL_EN);
        err = bridge_write16(TC358743_PLLCTL1, pllctl1);
    }
    if (err == ESP_OK) esp_rom_delay_us(10);
    if (err == ESP_OK) err = bridge_write16(TC358743_PLLCTL1, (uint16_t)(pllctl1 | TC358743_PLLCTL1_CKEN));
    if (err == ESP_OK) err = bridge_write16(TC358743_SYSCTL, (uint16_t)(sysctl & (uint16_t)~TC358743_SYSCTL_SLEEP));

    /* Linux's TC358743 driver uses these values for a 972 Mbps/lane CSI clock. */
    if (err == ESP_OK) err = bridge_write32(TC358743_CLW_CNTRL, 0);
    if (err == ESP_OK) err = bridge_write32(TC358743_D0W_CNTRL, 0);
    if (err == ESP_OK) err = bridge_write32(TC358743_D1W_CNTRL, 0);
    if (err == ESP_OK) err = bridge_write32(TC358743_LINEINITCNT, 0x1B58);
    if (err == ESP_OK) err = bridge_write32(TC358743_LPTXTIMECNT, 0x0007);
    if (err == ESP_OK) err = bridge_write32(TC358743_TCLK_HEADERCNT, 0x2806);
    if (err == ESP_OK) err = bridge_write32(TC358743_TCLK_TRAILCNT, 0x0000);
    if (err == ESP_OK) err = bridge_write32(TC358743_THS_HEADERCNT, 0x0806);
    if (err == ESP_OK) err = bridge_write32(TC358743_TWAKEUP, 0x4268);
    if (err == ESP_OK) err = bridge_write32(TC358743_TCLK_POSTCNT, 0x0008);
    if (err == ESP_OK) err = bridge_write32(TC358743_THS_TRAILCNT, 0x0005);
    if (err == ESP_OK) err = bridge_write32(TC358743_HSTXVREGCNT, 0);
    if (err == ESP_OK) err = bridge_write32(TC358743_HSTXVREGEN, 0x0007);
    if (err == ESP_OK) err = bridge_write32(TC358743_TXOPTIONCNTRL, 0x0000);
    if (err == ESP_OK) err = bridge_write32(TC358743_TXOPTIONCNTRL, 0x0001);
    if (err == ESP_OK) err = bridge_write32(TC358743_STARTCNTRL, 0x0001);
    if (err == ESP_OK) err = bridge_write32(TC358743_CSI_START, 0x0001);
    if (err == ESP_OK) err = bridge_write32(TC358743_CSI_CONFW,
                                             TC358743_CSI_CONFW_MODE_SET |
                                             TC358743_CSI_CONFW_ADDR_CONTROL |
                                             TC358743_CSI_CONTROL_MODE |
                                             TC358743_CSI_CONTROL_TXHSMD |
                                             TC358743_CSI_CONTROL_LANES_2);
    if (err == ESP_OK) err = bridge_write32(TC358743_CSI_CONFW,
                                             TC358743_CSI_CONFW_MODE_SET |
                                             TC358743_CSI_CONFW_ADDR_ERR_INTENA |
                                             TC358743_CSI_ERR_INT_ENABLE);
    if (err == ESP_OK) err = bridge_write32(TC358743_CSI_CONFW,
                                             TC358743_CSI_CONFW_MODE_CLEAR |
                                             TC358743_CSI_CONFW_ADDR_ERR_HALT |
                                             TC358743_CSI_ERR_HALT_MASK);
    if (err == ESP_OK) err = bridge_write32(TC358743_CSI_CONFW,
                                             TC358743_CSI_CONFW_MODE_SET |
                                             TC358743_CSI_CONFW_ADDR_INT_ENA |
                                             TC358743_CSI_INT_ENABLE);
    if (err == ESP_OK) err = bridge_write8(TC358743_VI_MUTE, 0xC0);
    if (err == ESP_OK) err = bridge_read16(TC358743_CONFCTL, &confctl);
    if (err == ESP_OK) err = bridge_write16(TC358743_CONFCTL, (uint16_t)(confctl | 0x0003));
    if (err == ESP_OK) s_csi_configured = true;
    return err;
}

esp_err_t watchpup_tc358743_poll(watchpup_tc358743_health_t *health)
{
    if (health == NULL || !health->probe_ok || s_device == NULL) return ESP_ERR_INVALID_STATE;

    uint8_t sys_status = 0;
    uint16_t csi_status = 0;
    uint16_t sysctl = 0, pllctl0 = 0, pllctl1 = 0;
    uint8_t edid_mode = 0, phy_ctl0 = 0, phy_ctl1 = 0, phy_ctl2 = 0;
    uint8_t phy_en = 0, phy_rst = 0, hdmi_det = 0, hv_rst = 0, ddc_ctl = 0, hpd_ctl = 0, ana_ctl = 0;
    uint8_t sys_int = 0, clk_int = 0, hdmi_int0 = 0, hdmi_int1 = 0, vi_status1 = 0, vi_status3 = 0, init_end = 0;
    esp_err_t err = bridge_read8(TC358743_SYS_STATUS, &sys_status);
    if (err == ESP_OK) err = bridge_read16(TC358743_CSI_STATUS, &csi_status);
    if (err == ESP_OK) err = bridge_read16(TC358743_SYSCTL, &sysctl);
    if (err == ESP_OK) err = bridge_read16(TC358743_PLLCTL0, &pllctl0);
    if (err == ESP_OK) err = bridge_read16(TC358743_PLLCTL1, &pllctl1);
    if (err == ESP_OK) err = bridge_read8(TC358743_EDID_MODE, &edid_mode);
    if (err == ESP_OK) err = bridge_read8(TC358743_PHY_CTL0, &phy_ctl0);
    if (err == ESP_OK) err = bridge_read8(TC358743_PHY_CTL1, &phy_ctl1);
    if (err == ESP_OK) err = bridge_read8(TC358743_PHY_CTL2, &phy_ctl2);
    if (err == ESP_OK) err = bridge_read8(TC358743_PHY_EN, &phy_en);
    if (err == ESP_OK) err = bridge_read8(TC358743_PHY_RST, &phy_rst);
    if (err == ESP_OK) err = bridge_read8(TC358743_HDMI_DET, &hdmi_det);
    if (err == ESP_OK) err = bridge_read8(TC358743_HV_RST, &hv_rst);
    if (err == ESP_OK) err = bridge_read8(TC358743_DDC_CTL, &ddc_ctl);
    if (err == ESP_OK) err = bridge_read8(TC358743_HPD_CTL, &hpd_ctl);
    if (err == ESP_OK) err = bridge_read8(TC358743_ANA_CTL, &ana_ctl);
    if (err == ESP_OK) err = bridge_read8(TC358743_SYS_INT, &sys_int);
    if (err == ESP_OK) err = bridge_read8(TC358743_CLK_INT, &clk_int);
    if (err == ESP_OK) err = bridge_read8(TC358743_HDMI_INT0, &hdmi_int0);
    if (err == ESP_OK) err = bridge_read8(TC358743_HDMI_INT1, &hdmi_int1);
    if (err == ESP_OK) err = bridge_read8(TC358743_VI_STATUS1, &vi_status1);
    if (err == ESP_OK) err = bridge_read8(TC358743_VI_STATUS3, &vi_status3);
    if (err == ESP_OK) err = bridge_read8(TC358743_INIT_END, &init_end);
    if (err != ESP_OK) {
        health->last_step = "poll_status";
        health->last_error = "i2c_read_failed";
        health->driver_state = "degraded";
        return err;
    }

    /* SYS_INT and CLK_INT are latched, write-one-to-clear status registers.
     * The Linux driver services these events even when it is polling rather
     * than using the bridge IRQ. A DVI transition without TMDS/sync gets one
     * additional PHY reset; the bound prevents a persistent bad source from
     * causing an endless reset loop. */
    if ((sys_int & SYS_INT_DVI) != 0 &&
        (sys_status & (SYS_STATUS_TMDS | SYS_STATUS_SYNC)) != (SYS_STATUS_TMDS | SYS_STATUS_SYNC) &&
        health->recovery_count < 2) {
        if (bridge_reset_hdmi_phy() == ESP_OK) {
            health->recovery_count++;
            ESP_LOGI(TAG, "[diag] bridge.recovery reason=dvi_event_without_lock count=%u",
                     (unsigned)health->recovery_count);
        }
    } else if ((sys_int & SYS_INT_HDMI) != 0) {
        (void)bridge_write8(TC358743_ANA_CTL, 0x31);
    }
    if (sys_int != 0) (void)bridge_write8(TC358743_SYS_INT, sys_int);
    if (clk_int != 0) (void)bridge_write8(TC358743_CLK_INT, clk_int);

    health->sys_status = sys_status;
    health->csi_status = csi_status;
    health->sysctl = sysctl;
    health->pllctl0 = pllctl0;
    health->pllctl1 = pllctl1;
    health->edid_mode = edid_mode;
    health->phy_ctl0 = phy_ctl0;
    health->phy_ctl1 = phy_ctl1;
    health->phy_ctl2 = phy_ctl2;
    health->phy_en = phy_en;
    health->phy_rst = phy_rst;
    health->hdmi_det = hdmi_det;
    health->hv_rst = hv_rst;
    health->ddc_ctl = ddc_ctl;
    health->hpd_ctl = hpd_ctl;
    health->hpd_asserted = (hpd_ctl & HPD_OUT) != 0;
    health->ana_ctl = ana_ctl;
    health->sys_int = sys_int;
    health->clk_int = clk_int;
    health->hdmi_int0 = hdmi_int0;
    health->hdmi_int1 = hdmi_int1;
    health->vi_status1 = vi_status1;
    health->vi_status3 = vi_status3;
    health->init_end = init_end;
    health->hdmi_signal_detected = (sys_status & SYS_STATUS_TMDS) != 0;
    health->hdmi_sync_locked = (sys_status & SYS_STATUS_SYNC) != 0;
    health->pll_locked = (sys_status & SYS_STATUS_PHY_PLL) != 0;
    health->csi_output_enabled = (csi_status & CSI_STATUS_TXACT) != 0;
    (void)bridge_refresh_refclk_health(&WATCHPUP_BOARD_CONFIG, health, "poll");

    if (health->hdmi_signal_detected && health->hdmi_sync_locked) {
        uint8_t width_lo = 0, width_hi = 0, height_lo = 0, height_hi = 0;
        uint8_t fps_lo = 0, fps_hi = 0;
        if (bridge_read8(TC358743_DE_WIDTH_H_LO, &width_lo) == ESP_OK &&
            bridge_read8(TC358743_DE_WIDTH_H_HI, &width_hi) == ESP_OK &&
            bridge_read8(TC358743_DE_WIDTH_V_LO, &height_lo) == ESP_OK &&
            bridge_read8(TC358743_DE_WIDTH_V_HI, &height_hi) == ESP_OK &&
            bridge_read8(TC358743_FV_CNT_LO, &fps_lo) == ESP_OK &&
            bridge_read8(TC358743_FV_CNT_HI, &fps_hi) == ESP_OK) {
            const uint16_t frame_interval = (uint16_t)(((fps_hi & 0x03) << 8) | fps_lo);
            health->negotiated_width = (uint16_t)(((width_hi & 0x1f) << 8) | width_lo);
            health->negotiated_height = (uint16_t)(((height_hi & 0x1f) << 8) | height_lo);
            health->negotiated_frame_rate = frame_interval == 0 ? 0 : (uint16_t)((10000 + frame_interval / 2) / frame_interval);
        }
        if (health->pll_locked) {
            if (!s_csi_configured) {
                if (bridge_configure_csi_output() != ESP_OK) {
                    health->last_error = "csi_output_config_failed";
                    health->driver_state = "signal_locked_csi_error";
                } else {
                    health->last_error = NULL;
                }
            } else {
                health->last_error = NULL;
            }
            health->last_signal_lock_ms = esp_timer_get_time() / 1000;
            if (health->last_error == NULL) health->driver_state = "signal_locked";
        } else {
            health->driver_state = "signal_detected";
            health->last_error = "pll_unlocked";
        }
    } else if (health->hdmi_signal_detected) {
        health->driver_state = "signal_detected";
        health->last_error = "no_sync";
    } else {
        health->driver_state = "waiting_for_signal";
        health->last_error = "no_signal";
        health->negotiated_width = 0;
        health->negotiated_height = 0;
        health->negotiated_frame_rate = 0;
    }

    ESP_LOGI(TAG, "[diag] subsystem.health subsystem=bridge_tc358743 status=%s driver_state=%s hpd_asserted=%s edid_loaded=%s hdmi_signal_detected=%s negotiated_width=%u negotiated_height=%u negotiated_frame_rate=%u pll_locked=%s csi_output_enabled=%s",
             health->last_error == NULL ? "ok" : "degraded", health->driver_state,
             health->hpd_asserted ? "true" : "false", health->edid_loaded ? "true" : "false",
             health->hdmi_signal_detected ? "true" : "false", (unsigned)health->negotiated_width,
             (unsigned)health->negotiated_height, (unsigned)health->negotiated_frame_rate,
             health->pll_locked ? "true" : "false", health->csi_output_enabled ? "true" : "false");
    ESP_LOGI(TAG, "[diag] bridge.registers sys_status=0x%02x csi_status=0x%04x sysctl=0x%04x pllctl0=0x%04x pllctl1=0x%04x edid_mode=0x%02x phy_ctl0=0x%02x phy_ctl1=0x%02x phy_ctl2=0x%02x phy_en=0x%02x phy_rst=0x%02x hdmi_det=0x%02x hv_rst=0x%02x ddc_ctl=0x%02x hpd_ctl=0x%02x ana_ctl=0x%02x sys_int=0x%02x clk_int=0x%02x hdmi_int0=0x%02x hdmi_int1=0x%02x vi_status1=0x%02x vi_status3=0x%02x init_end=0x%02x refclk_readback_valid=%s",
             sys_status, csi_status, sysctl, pllctl0, pllctl1, edid_mode, phy_ctl0, phy_ctl1, phy_ctl2, phy_en, phy_rst, hdmi_det, hv_rst, ddc_ctl, hpd_ctl, ana_ctl,
             sys_int, clk_int, hdmi_int0, hdmi_int1, vi_status1, vi_status3, init_end,
             health->refclk_readback_valid ? "true" : "false");
    return ESP_OK;
}

esp_err_t watchpup_tc358743_start(watchpup_tc358743_health_t *health)
{
    if (health == NULL) return ESP_ERR_INVALID_ARG;
    memset(health, 0, sizeof(*health));
    s_csi_configured = false;
    const watchpup_board_config_t *board = &WATCHPUP_BOARD_CONFIG;
    health->implemented = true;
    health->i2c_port = board->bridge_i2c_port;
    health->i2c_sda_gpio = board->bridge_i2c_sda_gpio;
    health->i2c_scl_gpio = board->bridge_i2c_scl_gpio;
    health->i2c_address = board->bridge_i2c_address;
    health->configured_csi_lane_count = board->configured_csi_lane_count;
    health->configured_csi_lane_rate_mbps = board->configured_csi_lane_rate_mbps;
    health->reset_supported = board->bridge_reset_gpio >= 0;
    health->edid_revision = "waveshare-720p60-vendor-edid";
    health->driver_state = "starting";
    health->last_step = "i2c_bus_init";

    const i2c_master_bus_config_t bus_config = {
        .i2c_port = board->bridge_i2c_port,
        .sda_io_num = board->bridge_i2c_sda_gpio,
        .scl_io_num = board->bridge_i2c_scl_gpio,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,
    };
    esp_err_t err = i2c_new_master_bus(&bus_config, &s_bus);
    if (err != ESP_OK) {
        bridge_fail(health, "i2c_bus_init", "i2c_bus_init_failed");
        return err;
    }
    const i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = board->bridge_i2c_address,
        .scl_speed_hz = 100000,
    };
    err = i2c_master_bus_add_device(s_bus, &device_config, &s_device);
    if (err != ESP_OK) {
        bridge_fail(health, "i2c_device_init", "i2c_device_init_failed");
        return err;
    }
    health->last_step = "probe";
    err = i2c_master_probe(s_bus, board->bridge_i2c_address, 100);
    if (err != ESP_OK) {
        bridge_fail(health, "probe", "tc358743_not_acknowledged");
        return err;
    }
    err = bridge_read16(TC358743_CHIPID, &health->chip_id);
    if (err != ESP_OK || (health->chip_id & 0xff00) != 0) {
        bridge_fail(health, "probe", "chip_id_invalid");
        return err == ESP_OK ? ESP_ERR_NOT_FOUND : err;
    }
    health->probe_ok = true;
    ESP_LOGI(TAG, "[diag] subsystem.init subsystem=bridge_tc358743 status=ok probe_ok=true chip_id=0x%04x i2c_port=%d sda_gpio=%d scl_gpio=%d i2c_address=0x%02x reset_supported=%s",
             health->chip_id, health->i2c_port, health->i2c_sda_gpio, health->i2c_scl_gpio,
             health->i2c_address, health->reset_supported ? "true" : "false");

    health->last_step = "hpd_low";
    err = bridge_write8(TC358743_HPD_CTL, 0);
    if (err != ESP_OK) { bridge_fail(health, "hpd_low", "hpd_write_failed"); return err; }
    health->hpd_asserted = false;
    vTaskDelay(pdMS_TO_TICKS(50));

    health->last_step = "configure_source_path";
    err = bridge_configure_source_path(board);
    if (err != ESP_OK) { bridge_fail(health, "configure_source_path", "source_path_config_failed"); return err; }
    (void)bridge_refresh_refclk_health(board, health, "post_configure");

    err = bridge_load_edid(health);
    if (err != ESP_OK) { bridge_fail(health, "load_edid", "edid_load_failed"); return err; }
    ESP_LOGI(TAG, "[diag] bridge.edid status=ok revision=%s blocks=2", health->edid_revision);

    /* Match the TC358743 reference driver: allow EDID/DDC setup to settle
     * before the HPD edge causes the HDMI source to begin its transaction. */
    vTaskDelay(pdMS_TO_TICKS(500));
    health->last_step = "hpd_high";
    err = bridge_write8(TC358743_HPD_CTL, HPD_OUT);
    if (err != ESP_OK) { bridge_fail(health, "hpd_high", "hpd_write_failed"); return err; }
    health->hpd_asserted = true;
    /* Give the source a full second to read EDID and begin TMDS training
     * before applying the explicit PHY recovery edge. */
    vTaskDelay(pdMS_TO_TICKS(1000));
    health->last_step = "hdmi_phy_recovery";
    err = bridge_reset_hdmi_phy();
    if (err != ESP_OK) {
        bridge_fail(health, "hdmi_phy_recovery", "phy_reset_failed");
        return err;
    }
    health->recovery_count++;
    ESP_LOGI(TAG, "[diag] bridge.recovery reason=post_hpd_phy_reset count=%u", (unsigned)health->recovery_count);
    (void)bridge_refresh_refclk_health(board, health, "post_phy_reset");
    vTaskDelay(pdMS_TO_TICKS(1000));
    health->last_init_success_ms = esp_timer_get_time() / 1000;
    health->last_step = "poll_status";
    (void)watchpup_tc358743_poll(health);
    return ESP_OK;
}

esp_err_t watchpup_tc358743_retrain(watchpup_tc358743_health_t *health)
{
    if (health == NULL || !health->probe_ok || s_device == NULL) return ESP_ERR_INVALID_STATE;

    const watchpup_board_config_t *board = &WATCHPUP_BOARD_CONFIG;
    health->last_step = "hpd_retrain_low";
    esp_err_t err = bridge_write8(TC358743_HPD_CTL, 0);
    if (err != ESP_OK) {
        bridge_fail(health, "hpd_retrain_low", "hpd_write_failed");
        return err;
    }
    health->hpd_asserted = false;
    vTaskDelay(pdMS_TO_TICKS(500));

    health->last_step = "hpd_retrain_high";
    err = bridge_write8(TC358743_HPD_CTL, HPD_OUT);
    if (err != ESP_OK) {
        bridge_fail(health, "hpd_retrain_high", "hpd_write_failed");
        return err;
    }
    health->hpd_asserted = true;
    vTaskDelay(pdMS_TO_TICKS(1000));

    health->last_step = "hpd_retrain_phy_reset";
    err = bridge_reset_hdmi_phy();
    if (err != ESP_OK) {
        bridge_fail(health, "hpd_retrain_phy_reset", "phy_reset_failed");
        return err;
    }
    health->recovery_count++;
    (void)bridge_refresh_refclk_health(board, health, "post_retrain_phy_reset");
    vTaskDelay(pdMS_TO_TICKS(1000));
    health->last_step = "poll_status";
    return watchpup_tc358743_poll(health);
}
