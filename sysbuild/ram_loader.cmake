#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_NRF_RAM_LOADER)
  ExternalZephyrProject_Add(
    APPLICATION ram_loader
    SOURCE_DIR "${ZEPHYR_NRF_MODULE_DIR}/applications/ram_loader"
  )
endif()
