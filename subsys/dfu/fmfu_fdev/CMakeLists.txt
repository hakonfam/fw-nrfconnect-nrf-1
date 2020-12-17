#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_library()

zephyr_library_sources(src/fmfu_fdev.c)

zephyr_compile_definitions(fmfu_fdev CDDL_CBOR_CANONICAL)

if (CONFIG_FMFU_CDDL_DECODER_BUILD_STRATEGY_SOURCE)
  zephyr_library_sources(src/modem_update_decode.c)
  zephyr_include_directories(include)
elseif(CONFIG_FMFU_CDDL_DECODER_BUILD_STRATEGY_GENERATE)
  add_subdirectory(cddl)
endif()
