# Copyright 2026 Google LLC
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(input_npcx_kbd)

# pinctrl_soc.h is needed by every user of <zephyr/drivers/pinctrl.h>.
zephyr_include_directories(include/pinctrl)

# The NPCX driver is built here, against a minimal NPCX soc.h, instead of
# through CONFIG_INPUT_NPCX_KBD, so that native_sim's own soc.h is left alone.
target_include_directories(app PRIVATE
  include/npcx
  "${ZEPHYR_BASE}/soc/nuvoton/npcx/common"
  "${ZEPHYR_BASE}/soc/nuvoton/npcx/common/npcxn/include"
)
target_sources(app PRIVATE
  src/main.c
  "${ZEPHYR_BASE}/drivers/input/input_npcx_kbd.c"
)

# Host-side helper that maps the KBS register block at its devicetree address.
target_sources(native_simulator INTERFACE
  src/kbs_regs_bottom.c
)
