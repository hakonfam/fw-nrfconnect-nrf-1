#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

# Build and include hex file with provision data about the image to be booted.
set(NRF_SCRIPTS            ${NRF_DIR}/scripts)
set(NRF_BOOTLOADER_SCRIPTS ${NRF_SCRIPTS}/bootloader)

set(PROVISION_HEX_NAME     provision.hex)
set(PROVISION_HEX          ${PROJECT_BINARY_DIR}/${PROVISION_HEX_NAME})
if (NOT CONFIG_NETCORE_BOOTLOADER)
  if (${CONFIG_SB_DEBUG_SIGNATURE_PUBLIC_KEY_LAST})
    set(ALL_PUBLIC_KEY_FILES "${PUBLIC_KEY_FILES},${SIGNATURE_PUBLIC_KEY_FILE}")
    message(WARNING
      "
      -----------------------------------------------------------------
      --- WARNING: SB_DEBUG_SIGNATURE_PUBLIC_KEY_LAST is enabled.   ---
      --- This config should only be enabled for testing/debugging. ---
      -----------------------------------------------------------------")
    else()
      set(ALL_PUBLIC_KEY_FILES "${SIGNATURE_PUBLIC_KEY_FILE},${PUBLIC_KEY_FILES}")
    endif()

endif() # CONFIG_NETCORE_BOOTLOADER

if (DEFINED CONFIG_SOC_NRF9160)
  # Override PM_PROVISION_ADDRESS since Partition Manager cannot yet place
  # partitions in OTP.
  set(otp_start_addr "0xff8108")
  set(provision_addr ${otp_start_addr})
elseif (DEFINED CONFIG_SOC_NRF5340_CPUAPP)
  # Override PM_PROVISION_ADDRESS since Partition Manager cannot yet place
  # partitions in OTP.
  set(otp_start_addr "0xff8100")
  set(provision_addr ${otp_start_addr})
else ()
  set(provision_addr "$<TARGET_PROPERTY:partition_manager,PM_PROVISION_ADDRESS>")
endif()

if (ALL_PUBLIC_KEY_FILES)
  set(public_key_arg "--public-key-files ${ALL_PUBLIC_KEY_FILES}")
endif()

if (CONFIG_NETCORE_BOOTLOADER)
  set(s0_arg --s0-addr $<TARGET_PROPERTY:partition_manager,PM_APP_ADDRESS>)
else()
  set(s0_arg --s0-addr $<TARGET_PROPERTY:partition_manager,PM_S0_ADDRESS>)
  set(s1_arg --s1-addr $<TARGET_PROPERTY:partition_manager,PM_S1_ADDRESS>)
endif()

add_custom_command(
  OUTPUT
  ${PROVISION_HEX}
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${NRF_BOOTLOADER_SCRIPTS}/provision.py
  ${s0_arg}
  ${s1_arg}
  --provision-addr ${provision_addr}
  ${public_key_arg}
  --output ${PROVISION_HEX}
  DEPENDS
  ${PROVISION_KEY_DEPENDS}
  WORKING_DIRECTORY
  ${PROJECT_BINARY_DIR}
  COMMENT
  "Creating provision data for Bootloader, storing to ${PROVISION_HEX_NAME}"
  USES_TERMINAL
  )

if (NOT CONFIG_NETCORE_BOOTLOADER)
  set(provision_depends signature_public_key_file_target)
endif()

add_custom_target(
  provision_target
  DEPENDS
  ${PROVISION_HEX}
  ${provision_depends}
  )

get_property(
  provision_set
  GLOBAL PROPERTY provision_PM_HEX_FILE SET
  )

if(NOT provision_set)
  # Set hex file and target for the 'provision' placeholder partition.
  # This includes the hex file (and its corresponding target) to the build.
  set_property(
    GLOBAL PROPERTY
    provision_PM_HEX_FILE
    ${PROVISION_HEX}
    )

  set_property(
    GLOBAL PROPERTY
    provision_PM_TARGET
    provision_target
    )
endif()
