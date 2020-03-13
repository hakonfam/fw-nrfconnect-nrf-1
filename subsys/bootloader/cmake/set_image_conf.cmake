#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

function(add_overlay_config image overlay_file)
  set(old_conf ${${image}OVERLAY_CONFIG})
  string(FIND "${old_conf}" "${overlay_file}" found)
  if (${found} EQUAL -1)
    set(${image}OVERLAY_CONFIG
      "${old_conf} ${overlay_file}"
      CACHE STRING "" FORCE)
  endif()
endfunction()

# This CMakeLists.txt is executed by the root application only when
# B0 (SECURE_BOOT) is enabled. First, figure out what image will be
# booted by B0, and set the required properties for that image to be
# bootable by B0.

if (CONFIG_BOOTLOADER_MCUBOOT)
  set(image_to_boot mcuboot_)

  add_overlay_config(
    mcuboot_
    ${CMAKE_CURRENT_SOURCE_DIR}/multi_image_mcuboot.conf
    )

else()

  if (CONFIG_SPM)
    set(image_to_boot spm_)
  endif()
endif()

if (image_to_boot)
  # Include a kconfig file which enables CONFIG_FW_INFO in the image
  # which is booted by B0.
  add_overlay_config(
    ${image_to_boot}
    ${CMAKE_CURRENT_SOURCE_DIR}/fw_info.conf
    )
else()
  assert(CONFIG_FW_INFO "CONFIG_FW_INFO must be set")
endif()

