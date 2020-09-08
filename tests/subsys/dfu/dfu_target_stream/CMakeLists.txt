#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(json)

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
