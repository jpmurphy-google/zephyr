/*
 * Copyright 2026 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_DRIVERS_INPUT_NPCX_KBD_PINCTRL_SOC_H_
#define ZEPHYR_TESTS_DRIVERS_INPUT_NPCX_KBD_PINCTRL_SOC_H_

#include <zephyr/types.h>

typedef uint32_t pinctrl_soc_pin_t;

/* One dummy pin per state; pinctrl_configure_pins() is faked by the test. */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop) { 0 }

#endif /* ZEPHYR_TESTS_DRIVERS_INPUT_NPCX_KBD_PINCTRL_SOC_H_ */
