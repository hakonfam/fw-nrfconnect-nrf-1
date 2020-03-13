#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

if (NOT CONFIG_NETCORE_BOOTLOADER)
  include(${CMAKE_CURRENT_LIST_DIR}/../cmake/set_image_conf.cmake)
  include(${CMAKE_CURRENT_LIST_DIR}/../cmake/debug_keys.cmake)
  include(${CMAKE_CURRENT_LIST_DIR}/../cmake/sign.cmake)
endif ()

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/provision.cmake)
