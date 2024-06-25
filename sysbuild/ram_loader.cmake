#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_NRF_RAM_LOADER)
  message("from main app, including ram loader")
  ExternalZephyrProject_Add(
    APPLICATION ram_loader
    SOURCE_DIR "${ZEPHYR_NRF_MODULE_DIR}/applications/ram_loader"
    BUILD_ONLY true
  )
endif()
