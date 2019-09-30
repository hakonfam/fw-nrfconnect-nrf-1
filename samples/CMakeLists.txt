#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

if (CONFIG_SPM)
  zephyr_add_executable(spm require_build)
  if (${require_build})
    add_subdirectory(nrf9160/spm ${CMAKE_BINARY_DIR}/spm)
  endif ()
endif()

if (CONFIG_NSB_BOOTLOADER)
  zephyr_add_executable(nsb require_build)
  if(${require_build})
    add_subdirectory(nsb ${CMAKE_BINARY_DIR}/nsb)
  endif ()
endif ()
