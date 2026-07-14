#pragma once

typedef struct {
    const char *board_id;
    const char *bridge_adapter_id;
    int phy_address;
    int phy_power_gpio;
    int mdc_gpio;
    int mdio_gpio;
    int rmii_clock_gpio;
    int rmii_tx_en_gpio;
    int rmii_txd0_gpio;
    int rmii_txd1_gpio;
    int rmii_crs_dv_gpio;
    int rmii_rxd0_gpio;
    int rmii_rxd1_gpio;
    int bridge_i2c_port;
    int bridge_i2c_sda_gpio;
    int bridge_i2c_scl_gpio;
    int bridge_i2c_address;
    int configured_csi_lane_count;
    int configured_csi_lane_rate_mbps;
    int bridge_refclk_hz;
    int bridge_reset_gpio;
} watchpup_board_config_t;

extern const watchpup_board_config_t WATCHPUP_BOARD_CONFIG;
