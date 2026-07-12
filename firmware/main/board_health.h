#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"

typedef struct {
    size_t psram_size_bytes;
    bool psram_test_passed;
    bool ethernet_initialized;
    bool ethernet_init_failed;
    bool ethernet_link_up;
    bool ethernet_has_ipv4;
    char ipv4_address[16];
} watchpup_board_health_t;

void watchpup_board_health_test_psram(watchpup_board_health_t *health);
esp_err_t watchpup_board_health_start_ethernet(watchpup_board_health_t *health);
