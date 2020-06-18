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

zephyr_code_relocate(${CMAKE_CURRENT_SOURCE_DIR}/src/uart_transport.c SRAM)
zephyr_code_relocate(${CMAKE_CURRENT_SOURCE_DIR}/src/slip.c SRAM)
zephyr_code_relocate(${CMAKE_CURRENT_SOURCE_DIR}/src/recovery_bootloader.c SRAM)
zephyr_code_relocate(${ZEPHYR_BASE}/arch/arm/core/aarch32/isr_wrapper.S SRAM)

get_property(libs GLOBAL PROPERTY ZEPHYR_LIBS)
print(libs)

foreach(l ${libs} kernel)
  get_target_property(sources ${l} SOURCES)
  #message("${l} sources: ${sources}")
  foreach(f ${sources})
    set(prefix "")
    if (NOT IS_ABSOLUTE ${f})
      message("${l}: ${f} is not absolute")
      find_library(out NAMES ${l} HINTS ${ZEPHYR_BASE})
      print(out)
    endif()
    zephyr_code_relocate(${prefx}${f} SRAM)
  endforeach()
endforeach()

zephyr_linker_sources(SECTIONS custom-sections.ld)
