#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_NRF_RAM_LOADER)
  # Change to radio target as this is the only local domain with sufficient TCM for the loader
  # string(REGEX REPLACE "cpuapp" "cpurad" radio_target ${CACHED_BOARD})
  ExternalZephyrProject_Add(
    APPLICATION ram_loader
    SOURCE_DIR "${ZEPHYR_NRF_MODULE_DIR}/applications/ram_loader"
    #    BOARD ${radio_target}
  )
endif()
