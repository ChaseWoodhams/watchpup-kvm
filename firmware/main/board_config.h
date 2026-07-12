#pragma once

typedef struct {
    const char *board_id;
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
} watchpup_board_config_t;

extern const watchpup_board_config_t WATCHPUP_BOARD_CONFIG;
