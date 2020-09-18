#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

zephyr_compile_options(-DPCD_CMD_ADDRESS=0x1000000)

project(pcd_test)

FILE(GLOB app_sources src/*.c)

target_sources(app PRIVATE ${app_sources})
