#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_named(key_loader)

zephyr_library_sources(key_loader.c)

set(base_dir ${CONFIG_NRF_KEY_LOADER_BASE_PATH})
set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

set(required_keys
  APPLICATION_PUBKEY_GEN0
  APPLICATION_PUBKEY_GEN1
  APPLICATION_PUBKEY_GEN2
  APPLICATION_FWENC_GEN0
  APPLICATION_FWENC_GEN1
  RADIO_PUBKEY_GEN0
  RADIO_PUBKEY_GEN1
  RADIO_PUBKEY_GEN2
  RADIO_FWENC_GEN0
  RADIO_FWENC_GEN1
  NONE_OEMPUBKEY_GEN0
  NONE_OEMPUBKEY_GEN1
  NONE_OEMPUBKEY_GEN2
  )

foreach (key_name ${required_keys} )
  generate_inc_file_for_target(
    key_loader
    ${CONFIG_NRF_KEY_LOADER_BASE_PATH}/${key_name}
    ${gen_dir}/${key_name})
endforeach()

