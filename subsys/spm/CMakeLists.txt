#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#
if (CONFIG_BOOTLOADER_MCUBOOT)
    add_custom_command(
      OUTPUT merged_to_sign.hex
      COMMAND
      ${PYTHON_EXECUTABLE}
      ${ZEPHYR_BASE}/scripts/mergehex.py
      -o merged_to_sign.hex
      ${ZEPHYR_BINARY_DIR}/${KERNEL_HEX_NAME}
      ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_HEX_NAME}
      ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL_HEX_NAME}
      DEPENDS
      kernel_elf
      ${IMAGE}kernel_elf
      )

    add_custom_target(merged_to_sign DEPENDS merged_to_sign.hex)

    set_property(
      GLOBAL PROPERTY
      MCUBOOT_SIGN_MERGED
      "merged_to_sign.hex"
      )
else ()
  set_property(
    GLOBAL PROPERTY
    MERGE_ROOT_APP
    1
    )

  set_property(
    GLOBAL APPEND PROPERTY
    HEX_FILES_TO_MERGE_TARGET
    ${IMAGE}kernel_elf
    )

  set_property(
    GLOBAL APPEND PROPERTY
    HEX_FILES_TO_MERGE
    ${ZEPHYR_BINARY_DIR}/${KERNEL_HEX_NAME}
    )
endif()

zephyr_sources(spm.c)
