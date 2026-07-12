#include "board_config.h"

const watchpup_board_config_t WATCHPUP_BOARD_CONFIG = {
    .board_id = "waveshare_esp32_p4_eth",
    .phy_address = 1,
    .phy_power_gpio = 51,
    .mdc_gpio = 31,
    .mdio_gpio = 52,
    .rmii_clock_gpio = 50,
    .rmii_tx_en_gpio = 49,
    .rmii_txd0_gpio = 34,
    .rmii_txd1_gpio = 35,
    .rmii_crs_dv_gpio = 28,
    .rmii_rxd0_gpio = 29,
    .rmii_rxd1_gpio = 30,
};
