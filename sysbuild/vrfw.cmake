# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

# Include NRF VRFW image if the path is specified
if(DEFINED SB_CONFIG_NRF_VRFW_IMAGE AND NOT "${SB_CONFIG_NRF_VRFW_IMAGE}" STREQUAL "")
  ExternalZephyrProject_Add(
    APPLICATION nrf_vrfw
    SOURCE_DIR ${SB_CONFIG_NRF_VRFW_IMAGE}
    BOARD ${BOARD}${BOARD_QUALIFIERS}
    BOARD_REVISION ${BOARD_REVISION}
  )
endif()