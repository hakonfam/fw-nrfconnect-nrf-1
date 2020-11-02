#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_dfu_target_stream)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

# Direct include since its not possible to get via Kconfig.
target_sources(app
  PRIVATE
  ${ZEPHYR_BASE}/../nrf/subsys/dfu/src/dfu_target_stream.c
  )

target_include_directories(app
  PRIVATE
  ${ZEPHYR_BASE}/../nrf/subsys/dfu/include
  ${ZEPHYR_BASE}/../nrf/include
  )

# Note: We enable CONFIG_DFU_TARGET_STREAM_SAVE_PROGRESS here since it has
# dependencies in KConfig which is not satisfiable for this test project.
target_compile_options(app
  PRIVATE
  -DCONFIG_DFU_TARGET_LOG_LEVEL=2
  -DCONFIG_DFU_TARGET_STREAM_SAVE_PROGRESS=1
  )
