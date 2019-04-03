#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_sources(spm.c)

if (IMAGE)
  # Only perform merge operation when being part of multi image build.
  if (CONFIG_BOOTLOADER_MCUBOOT)
    # If MCUBOOT is enabled, ensure that spm and app is merged before
    # the merged result is signed.
    add_custom_command(
      OUTPUT merged_to_sign.hex
      COMMAND
      ${PYTHON_EXECUTABLE}
      ${ZEPHYR_BASE}/scripts/mergehex.py
      -o merged_to_sign.hex
      ${ZEPHYR_BINARY_DIR}/${KERNEL_HEX_NAME}
      ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_HEX_NAME}
      WORKING_DIRECTORY
      ${CMAKE_BINARY_DIR}
      DEPENDS
      kernel_elf
      ${IMAGE}kernel_elf
      )

    add_custom_target(MERGED_TO_SIGN_TARGET DEPENDS merged_to_sign.hex)

    set_property(
      GLOBAL PROPERTY
      MCUBOOT_SIGN_MERGED
      "merged_to_sign.hex"
      )
  else ()
    set_property(
      GLOBAL APPEND PROPERTY
      HEX_FILES_TO_MERGE
      ${PROJECT_BINARY_DIR}/${KERNEL_HEX_NAME}

      )
    set_property(
      GLOBAL APPEND PROPERTY
      HEX_FILES_TO_MERGE_TARGET
      ${IMAGE}kernel_elf
      )

    set_property(
      GLOBAL APPEND PROPERTY
      HEX_FILES_TO_MERGE
      ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_HEX_NAME}

      )
    set_property(
      GLOBAL APPEND PROPERTY
      HEX_FILES_TO_MERGE_TARGET
      kernel_elf
      )
  endif()
endif()
