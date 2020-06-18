#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(production_bootloader)

target_include_directories(app PRIVATE include)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

zephyr_code_relocate(src/uart_transport.c SRAM)
zephyr_code_relocate(src/slip.c SRAM)
zephyr_code_relocate(src/recovery_bootloader.c SRAM)


zephyr_linker_sources(SECTIONS custom-sections.ld)
