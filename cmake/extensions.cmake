#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

function(get_board_without_ns_suffix board_in board_out)
  string(REGEX REPLACE "(_?ns)$" "" board_in_without_suffix ${board_in})
  if(NOT ${board_in} STREQUAL ${board_in_without_suffix})
    if (NOT CONFIG_ARM_NONSECURE_FIRMWARE)
      message(FATAL_ERROR "${board_in} is not a valid name for a board without "
      "'CONFIG_ARM_NONSECURE_FIRMWARE' set. This because the 'ns'/'_ns' ending "
      "indicates that the board is the non-secure variant in a TrustZone "
      "enabled system.")
    endif()
    set(${board_out} ${board_in_without_suffix} PARENT_SCOPE)
    message("Changed board to secure ${board_in_without_suffix} (NOT NS)")
  else()
    set(${board_out} ${board_in} PARENT_SCOPE)
  endif()
endfunction()

function(add_overlay_config image overlay_file)
  set(old_conf ${${image}OVERLAY_CONFIG})
  string(FIND "${old_conf}" "${overlay_file}" found)
  if (${found} EQUAL -1)
    set(${image}OVERLAY_CONFIG
      "${old_conf} ${overlay_file}"
      CACHE STRING "" FORCE)
  endif()
endfunction()

