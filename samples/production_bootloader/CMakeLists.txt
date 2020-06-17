#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.8.2)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(uart_recovery_bootloader)

zephyr_library_include_directories(include)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/recovery_bootloader.c)
target_sources(app PRIVATE src/slip.c)
target_sources(app PRIVATE src/uart_transport.c)
