#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_named(key_loader)

zephyr_library_sources(key_loader.c)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
generate_inc_file_for_target(
  key_loader
  ${CONFIG_NRF_KEY_LOADER_MANIFEST_PUBKEY_APPLICATION_GEN0_PATH}
  ${gen_dir}/${CONFIG_NRF_KEY_LOADER_MANIFEST_PUBKEY_APPLICATION_GEN0_NAME})
