#pragma once

#include "board_health.h"
#include "bridge_tc358743.h"
#include "capture_csi.h"
#include "esp_err.h"

void watchpup_diag_log_boot_start(void);
esp_err_t watchpup_diag_start(const watchpup_board_health_t *health,
                              watchpup_tc358743_health_t *bridge,
                              watchpup_capture_health_t *capture);
