#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# secondary_slot([EXT_START <address>] [EXT_BASE <address>])
function(secondary_slot)
  cmake_parse_arguments(SEC_SLOT "" "NET_APP_SLOT_SIZE;EXT_START;EXT_BASE" "" ${ARGN})

  # Get the amount of updateable images from the MCUboot child image
  get_shared(no IMAGE mcuboot PROPERTY UPDATEABLE_IMAGE_NUMBER)

  # The address coming from other domains are not available in this scope
  # since it is imported by a different domain. Hence, it must be fetched
  # through the 'partition_manager' target.
  get_target_property(net_app_addr partition_manager CPUNET_PM_APP_ADDRESS)

  # This check whether multi image updates are enabled, in which case we need
  # to use the "_1" variant of the secondary partition for the network core.
  if(${no} GREATER 1)
    set(sec_slot_idx "_1")
    set(ext_start "${SEC_SLOT_EXT_START} +")
  endif()

  print(ext_start)
  print(SEC_SLOT_EXT_BASE)
  print(SEC_SLOT_EXT_START)

  # Calculate the offset fram the address which the net/app core app is linked
  # against to the secondary slot. We need these values to generate hex files
  # which targets the secondary slot.
  set(net_to_sec_sum
    "${ext_start} ${SEC_SLOT_EXT_BASE} + ${PM_MCUBOOT_SECONDARY${sec_slot_idx}_ADDRESS} - ${net_app_addr} + ${PM_MCUBOOT_PAD${sec_slot_idx}_SIZE}")
  print(net_to_sec_sum)
  math(EXPR net_app_to_secondary ${net_to_sec_sum})
  set_property(
    TARGET partition_manager
    PROPERTY net_app_TO_SECONDARY
    ${net_app_to_secondary}
    )

  set(app_to_sec_sum
    "${SEC_SLOT_EXT_START} + ${SEC_SLOT_EXT_BASE} + ${PM_MCUBOOT_SECONDARY_ADDRESS} - ${PM_MCUBOOT_PRIMARY_ADDRESS}")
  print(app_to_sec_sum)
  math(EXPR app_to_secondary ${app_to_sec_sum} )
  set_property(
    TARGET partition_manager
    PROPERTY app_TO_SECONDARY
    ${app_to_secondary}
    )

  # This value is needed by `imgtool.py` which is used to sign the images.
  set_property(
    TARGET partition_manager
    PROPERTY net_app_slot_size
    ${PM_MCUBOOT_SECONDARY${sec_slot_idx}_SIZE}
    )
endfunction()

macro(add_region)
  set(oneValueArgs NAME SIZE BASE PLACEMENT DEVICE DYNAMIC_PARTITION)
  cmake_parse_arguments(REGION "" "${oneValueArgs}" "" ${ARGN})
  list(APPEND regions ${REGION_NAME})
  list(APPEND region_arguments "--${REGION_NAME}-size;${REGION_SIZE}")
  list(APPEND region_arguments "--${REGION_NAME}-base-address;${REGION_BASE}")
  list(APPEND region_arguments
    "--${REGION_NAME}-placement-strategy;${REGION_PLACEMENT}")
  if (REGION_DEVICE)
    list(APPEND region_arguments "--${REGION_NAME}-device;${REGION_DEVICE}")
  endif()
  if (REGION_DYNAMIC_PARTITION)
    list(APPEND region_arguments
      "--${REGION_NAME}-dynamic-partition;${REGION_DYNAMIC_PARTITION}")
  endif()
endmacro()

# Load static configuration if found.
set(user_def_pm_static ${PM_STATIC_YML_FILE})
set(nodomain_pm_static ${APPLICATION_SOURCE_DIR}/pm_static.yml)
set(domain_pm_static ${APPLICATION_SOURCE_DIR}/pm_static_${DOMAIN}.yml)

if(EXISTS "${user_def_pm_static}" AND NOT IS_DIRECTORY "${user_def_pm_static}")
  set(static_configuration_file ${user_def_pm_static})
elseif (EXISTS ${domain_pm_static})
  set(static_configuration_file ${domain_pm_static})
elseif (EXISTS ${nodomain_pm_static})
  set(static_configuration_file ${nodomain_pm_static})
endif()

if (EXISTS ${static_configuration_file})
  set(static_configuration --static-config ${static_configuration_file})
endif()

if (NOT static_configuration AND CONFIG_PM_IMAGE_NOT_BUILT_FROM_SOURCE)
  message(WARNING
    "One or more child image is not configured to be built from source. \
    However, there is no static configuration provided to the \
    partition manager. Please provide a static configuration as described in \
    the 'Scripts -> Partition Manager -> Static configuration' chapter in the \
    documentation. Without this information, the build system is not able to \
    place the image correctly in flash.")
endif()


# Check if current image is the dynamic partition in its domain.
# I.E. it is the only partition without a statically configured size in this
# domain. This is equivalent to the 'app' partition in the root domain.
#
# The dynamic partition is specified by the parent domain (i.e. the domain
# which creates the current domain through 'create_domain_image()'.
if(DEFINED ${DOMAIN}_PM_DOMAIN_DYNAMIC_PARTITION
   AND "${IMAGE_NAME}" STREQUAL "${${DOMAIN}_PM_DOMAIN_DYNAMIC_PARTITION}"
)
  set(is_dynamic_partition_in_domain TRUE)
endif()

get_property(PM_IMAGES GLOBAL PROPERTY PM_IMAGES)
get_property(PM_SUBSYS_PREPROCESSED GLOBAL PROPERTY PM_SUBSYS_PREPROCESSED)
get_property(PM_DOMAINS GLOBAL PROPERTY PM_DOMAINS)

# This file is executed once per domain.
#
# It will be executed if one of the following criteria is true for the
# current image:
# - It's a child image, and is the dynamic partition in the domain
# - It's the root image, and a static configuration has been provided
# - It's the root image, and PM_IMAGES is populated.
# - It's the root image, and other domains exist.
# - A subsys has defined a partition and CONFIG_PM_SINGLE_IMAGE is set.
# Otherwise, return here
if (NOT (
  (IMAGE_NAME AND is_dynamic_partition_in_domain) OR
  (NOT IMAGE_NAME AND static_configuration) OR
  (NOT IMAGE_NAME AND PM_IMAGES) OR
  (NOT IMAGE_NAME AND PM_DOMAINS) OR
  (PM_SUBSYS_PREPROCESSED AND CONFIG_PM_SINGLE_IMAGE)
  ))
  return()
endif()

# Set the dynamic partition. This is the only partition which does not
# have a statically defined size. There is only one dynamic partition per
# domain. For the "root domain" (ie the domain of the root image) this is
# always "app".
if (NOT is_dynamic_partition_in_domain)
  set(dynamic_partition "app")
else()
  set(dynamic_partition ${${DOMAIN}_PM_DOMAIN_DYNAMIC_PARTITION})
  set(
    dynamic_partition_argument
    "--flash_primary-dynamic-partition;${dynamic_partition}"
    )
endif()

# Add the dynamic partition as an image partition.
set_property(GLOBAL PROPERTY
  ${dynamic_partition}_PM_HEX_FILE
  ${PROJECT_BINARY_DIR}/${KERNEL_HEX_NAME}
  )

set_property(GLOBAL PROPERTY
  ${dynamic_partition}_PM_TARGET
  ${logical_target_for_zephyr_elf}
  )

# Prepare the input_files, header_files, and images lists
set(generated_path include/generated)
foreach (image ${PM_IMAGES})
  list(APPEND prefixed_images ${DOMAIN}:${image})
  list(APPEND images ${image})

  get_shared(${image}_input_files IMAGE ${image} PROPERTY PM_YML_FILES)
  get_shared(${image}_binary_dir  IMAGE ${image} PROPERTY ZEPHYR_BINARY_DIR)

  list(APPEND input_files  ${${image}_input_files})
  list(APPEND header_files ${${image}_binary_dir}/${generated_path}/pm_config.h)

  # Re-configure (Re-execute all CMakeLists.txt code) when original
  # (not preprocessed) configuration file changes.
  get_shared(dependencies IMAGE ${image} PROPERTY PM_YML_DEP_FILES)
  set_property(
    DIRECTORY APPEND PROPERTY
    CMAKE_CONFIGURE_DEPENDS
    ${dependencies}
    )
endforeach()

# Explicitly add the dynamic partition image
list(APPEND prefixed_images "${DOMAIN}:${dynamic_partition}")
list(APPEND images ${dynamic_partition})
list(APPEND input_files ${ZEPHYR_BINARY_DIR}/${generated_path}/pm.yml)
list(APPEND header_files ${ZEPHYR_BINARY_DIR}/${generated_path}/pm_config.h)

# Add subsys defined pm.yml to the input_files
list(APPEND input_files ${PM_SUBSYS_PREPROCESSED})

if (DEFINED CONFIG_SOC_NRF9160)
  # See nRF9160 Product Specification, chapter "UICR"
  set(otp_start_addr "0xff8108")
  set(otp_size 756) # 189 * 4
elseif (DEFINED CONFIG_SOC_NRF5340_CPUAPP)
  # See nRF5340 Product Specification, chapter Application Core -> ... "UICR"
  set(otp_start_addr "0xff8100")
  set(otp_size 764)  # 191 * 4
endif()

add_region(
  NAME sram_primary
  SIZE ${CONFIG_PM_SRAM_SIZE}
  BASE ${CONFIG_PM_SRAM_BASE}
  PLACEMENT complex
  DYNAMIC_PARTITION sram_primary
  )

math(EXPR flash_size "${CONFIG_FLASH_SIZE} * 1024" OUTPUT_FORMAT HEXADECIMAL)

if (CONFIG_SOC_NRF9160 OR CONFIG_SOC_NRF5340_CPUAPP)
  add_region(
    NAME otp
    SIZE ${otp_size}
    BASE ${otp_start_addr}
    PLACEMENT start_to_end
    )
endif()
add_region(
  NAME flash_primary
  SIZE ${flash_size}
  BASE ${CONFIG_FLASH_BASE_ADDRESS}
  PLACEMENT complex
  DEVICE NRF_FLASH_DRV_NAME
  )

if (CONFIG_PM_EXTERNAL_FLASH)
  add_region(
    NAME external_flash
    SIZE ${CONFIG_PM_EXTERNAL_FLASH_SIZE}
    BASE ${CONFIG_PM_EXTERNAL_FLASH_BASE}
    PLACEMENT start_to_end
    DEVICE ${CONFIG_PM_EXTERNAL_FLASH_DEV_NAME}
    )
endif()

# Check if we are performing simultaneous update of the app core and network
# core on the nRF53. To do this check, we need to fetch the value of the
# UPDATEABLE_IMAGE_NUMBER value set in the mcuboot child image.
get_shared(
  mcuboot_UPDATEABLE_IMAGE_NUMBER
  IMAGE mcuboot
  PROPERTY UPDATEABLE_IMAGE_NUMBER
  )

# If simultaneous updates of the network core and application core is supported
# we add a region which is used to emulate flash. In reality this data is being
# placed in RAM. This is used to bank the network core update in RAM while
# the application core update is banked in flash. This works since the nRF53
# application core has 512kB of RAM and the network core only has 256kB of flash
if (CONFIG_NRF53_UPGRADE_NETWORK_CORE
  AND CONFIG_HCI_RPMSG_BUILD_STRATEGY_FROM_SOURCE
  AND (mcuboot_UPDATEABLE_IMAGE_NUMBER GREATER 1))

  # This region will contain the 'mcuboot_secondary' partition, and the banked
  # updates for the network core will be stored here.
  add_region(
    NAME ram_flash
    SIZE 0x40000    # 256kB - The maximum size of a netcore image
    BASE 0x00000000 # As this is an emulated area the base does not matter.
    PLACEMENT start_to_end
    DEVICE "flash_ctrl"
    )
endif()

if (DOMAIN)
  set(UNDERSCORE_DOMAIN _${DOMAIN})
endif()

set(pm_out_partition_file ${APPLICATION_BINARY_DIR}/partitions${UNDERSCORE_DOMAIN}.yml)
set(pm_out_region_file ${APPLICATION_BINARY_DIR}/regions${UNDERSCORE_DOMAIN}.yml)
set(pm_out_dotconf_file ${APPLICATION_BINARY_DIR}/pm${UNDERSCORE_DOMAIN}.config)

set(pm_cmd
  ${PYTHON_EXECUTABLE}
  ${NRF_DIR}/scripts/partition_manager.py
  --input-files ${input_files}
  --regions ${regions}
  --output-partitions ${pm_out_partition_file}
  --output-regions ${pm_out_region_file}
  ${dynamic_partition_argument}
  ${static_configuration}
  ${region_arguments}
  )

set(pm_output_cmd
  ${PYTHON_EXECUTABLE}
  ${NRF_DIR}/scripts/partition_manager_output.py
  --input-partitions ${pm_out_partition_file}
  --input-regions ${pm_out_region_file}
  --config-file ${pm_out_dotconf_file}
  )

# Run the partition manager algorithm.
execute_process(
  COMMAND
  ${pm_cmd}
  RESULT_VARIABLE ret
  )

if(NOT ${ret} EQUAL "0")
  message(FATAL_ERROR "Partition Manager failed, aborting. Command: ${pm_cmd}")
endif()

# Produce header files and config file.
execute_process(
  COMMAND
  ${pm_output_cmd}
  RESULT_VARIABLE ret
  )

if(NOT ${ret} EQUAL "0")
  message(FATAL_ERROR "Partition Manager output generation failed, aborting. Command: ${pm_output_cmd}")
endif()

# Create a dummy target that we can add properties to for
# extraction in generator expressions.
add_custom_target(partition_manager)

# Make Partition Manager configuration available in CMake
import_kconfig(PM_ ${pm_out_dotconf_file} pm_var_names)

foreach(name ${pm_var_names})
  set_property(
    TARGET partition_manager
    PROPERTY ${name}
    ${${name}}
    )
endforeach()

# Turn the space-separated list into a Cmake list.
string(REPLACE " " ";" PM_ALL_BY_SIZE ${PM_ALL_BY_SIZE})

# Iterate over every partition, from smallest to largest.
foreach(part ${PM_ALL_BY_SIZE})
  string(TOUPPER ${part} PART)
  get_property(${part}_PM_HEX_FILE GLOBAL PROPERTY ${part}_PM_HEX_FILE)
  get_property(${part}_PM_ELF_FILE GLOBAL PROPERTY ${part}_PM_ELF_FILE)

  # Process container partitions (if it has a SPAN list it is a container partition).
  if(DEFINED PM_${PART}_SPAN)
    string(REPLACE " " ";" PM_${PART}_SPAN ${PM_${PART}_SPAN})
    list(APPEND containers ${part})
  endif()

  # Include the partition in the merge operation if it has a hex file.
  if(DEFINED ${part}_PM_HEX_FILE)
    get_property(${part}_PM_TARGET GLOBAL PROPERTY ${part}_PM_TARGET)
    list(APPEND explicitly_assigned ${part})
  else()
    if(${part} IN_LIST images)
      get_shared(${part}_bin_dir  IMAGE ${part} PROPERTY ZEPHYR_BINARY_DIR)
      get_shared(${part}_hex_file IMAGE ${part} PROPERTY KERNEL_HEX_NAME)
      get_shared(${part}_elf_file IMAGE ${part} PROPERTY KERNEL_ELF_NAME)
      set(${part}_PM_HEX_FILE ${${part}_bin_dir}/${${part}_hex_file})
      set(${part}_PM_ELF_FILE ${${part}_bin_dir}/${${part}_elf_file})
      set(${part}_PM_TARGET ${part}_subimage)
    elseif(${part} IN_LIST containers)
      set(${part}_PM_HEX_FILE ${PROJECT_BINARY_DIR}/${part}.hex)
      set(${part}_PM_TARGET ${part}_hex)
    endif()
    list(APPEND implicitly_assigned ${part})
  endif()
endforeach()

if (${is_dynamic_partition_in_domain})
  set(merged_suffix _${DOMAIN})
  string(TOUPPER ${merged_suffix} MERGED_SUFFIX)
endif()
set(merged merged${merged_suffix})
set(MERGED MERGED${MERGED_SUFFIX})

set(PM_${MERGED}_SPAN ${implicitly_assigned} ${explicitly_assigned})
set(${merged}_overlap TRUE) # Enable overlapping for the merged hex file.

# Iterate over all container partitions, plus the "fake" merged paritition.
# The loop will create a hex file for each iteration.
foreach(container ${containers} ${merged})
  string(TOUPPER ${container} CONTAINER)

  # Prepare the list of hex files and list of dependencies for the merge command.
  foreach(part ${PM_${CONTAINER}_SPAN})
    string(TOUPPER ${part} PART)
    list(APPEND ${container}hex_files ${${part}_PM_HEX_FILE})
    list(APPEND ${container}elf_files ${${part}_PM_ELF_FILE})
    list(APPEND ${container}targets ${${part}_PM_TARGET})
  endforeach()

  # Do not merge hex files for empty partitions
  if(NOT ${container}hex_files)
    list(REMOVE_ITEM PM_${MERGED}_SPAN ${container})
    continue()
  endif()

  # If overlapping is enabled, add the appropriate argument.
  if(${${container}_overlap})
    set(${container}overlap_arg --overlap=replace)
  endif()

  # Add command to merge files.
  add_custom_command(
    OUTPUT ${PROJECT_BINARY_DIR}/${container}.hex
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/scripts/mergehex.py
    -o ${PROJECT_BINARY_DIR}/${container}.hex
    ${${container}overlap_arg}
    ${${container}hex_files}
    DEPENDS
    ${${container}targets}
    ${${container}hex_files}
    # SES will load symbols from all elf files listed as dependencies to
    # ${PROJECT_BINARY_DIR}/merged.hex. Therefore we add
    # ${${container}elf_files} as dependency to ensure they are loaded by SES
    # even though it is unnecessary for building the application.
    ${${container}elf_files}
    )

  # Wrapper target for the merge command.
  add_custom_target(
    ${container}_hex
    ALL DEPENDS
    ${PROJECT_BINARY_DIR}/${container}.hex
    )

endforeach()

if (CONFIG_SECURE_BOOT AND CONFIG_BOOTLOADER_MCUBOOT)
  # Create symbols for the offsets required for moving test update hex files
  # to MCUBoots secondary slot. This is needed because objcopy does not
  # support arithmetic expressions as argument (e.g. '0x100+0x200'), and all
  # of the symbols used to generate the offset is only available as a
  # generator expression when MCUBoots cmake code exectues. This because
  # partition manager is performed as the last step in the configuration stage.
  math(EXPR s0_offset "${PM_MCUBOOT_SECONDARY_ADDRESS} - ${PM_S0_ADDRESS}")
  math(EXPR s1_offset "${PM_MCUBOOT_SECONDARY_ADDRESS} - ${PM_S1_ADDRESS}")

  set_property(
    TARGET partition_manager
    PROPERTY s0_TO_SECONDARY
    ${s0_offset}
    )
  set_property(
    TARGET partition_manager
    PROPERTY s1_TO_SECONDARY
    ${s1_offset}
    )
endif()

if (is_dynamic_partition_in_domain)
  # We are being built as sub image.
  # Expose the generated partition manager configuration files to parent image.
  # This is used by the root image to create the global configuration in
  # pm_config.h.
  set_shared(IMAGE ${DOMAIN} PROPERTY PM_DOMAIN_PARTITIONS ${pm_out_partition_file})
  set_shared(IMAGE ${DOMAIN} PROPERTY PM_DOMAIN_REGIONS ${pm_out_region_file})
  set_shared(IMAGE ${DOMAIN} PROPERTY PM_DOMAIN_HEADER_FILES ${header_files})
  set_shared(IMAGE ${DOMAIN} PROPERTY PM_DOMAIN_IMAGES ${prefixed_images})
  set_shared(IMAGE ${DOMAIN} PROPERTY PM_HEX_FILE ${PROJECT_BINARY_DIR}/${merged}.hex)
  set_shared(IMAGE ${DOMAIN} PROPERTY PM_DOTCONF_FILES ${pm_out_dotconf_file})
  set_shared(IMAGE ${DOMAIN} PROPERTY PM_APP_HEX ${PROJECT_BINARY_DIR}/app.hex)
  set_shared(IMAGE ${IMAGE_NAME} APPEND PROPERTY BUILD_BYPRODUCTS ${PROJECT_BINARY_DIR}/${merged}.hex)
  if(CONFIG_SECURE_BOOT)
    # Only when secure boot is enabled the app will be signed.
    set_shared(IMAGE ${DOMAIN} PROPERTY PM_SIGNED_APP_HEX ${PROJECT_BINARY_DIR}/signed_by_b0_app.hex)
  endif()
else()
  # This is the root image, generate the global pm_config.h
  # First, include the shared properties for all child images.
  if (PM_DOMAINS)
    # We ensure the existence of PM_DOMAINS to support older cmake versions.
    # When version >= 3.17 is required this check can be removed.
    list(REMOVE_DUPLICATES PM_DOMAINS)
  endif()
  foreach (d ${PM_DOMAINS})
    # Don't include shared vars from own domain.
    if (NOT ("${DOMAIN}" STREQUAL "${d}"))
      get_shared(shared_header_files          IMAGE ${d} PROPERTY PM_DOMAIN_HEADER_FILES)
      get_shared(shared_prefixed_images       IMAGE ${d} PROPERTY PM_DOMAIN_IMAGES)
      get_shared(shared_pm_out_partition_file IMAGE ${d} PROPERTY PM_DOMAIN_PARTITIONS)
      get_shared(shared_pm_out_region_file    IMAGE ${d} PROPERTY PM_DOMAIN_REGIONS)
      get_shared(shared_domain_hex_files      IMAGE ${d} PROPERTY PM_HEX_FILE)

      list(APPEND header_files          ${shared_header_files})
      list(APPEND prefixed_images       ${shared_prefixed_images})
      list(APPEND pm_out_partition_file ${shared_pm_out_partition_file})
      list(APPEND pm_out_region_file    ${shared_pm_out_region_file})
      list(APPEND domain_hex_files      ${shared_domain_hex_files})
      list(APPEND global_hex_depends    ${${d}_PM_DOMAIN_DYNAMIC_PARTITION}_subimage)

      # Add domain prefix cmake variables for all partitions
      # Here, we actually overwrite the already imported kconfig values
      # for our own domain. This is not an issue since all of these variables
      # are accessed through the 'partition_manager' target, and most likely
      # through generator expression, as this file is one of the last
      # cmake files executed in the configure stage.
      get_shared(conf_file IMAGE ${d} PROPERTY PM_DOTCONF_FILES)
      import_kconfig(PM_ ${conf_file} ${d}_pm_var_names)

      foreach(name ${${d}_pm_var_names})
        set_property(
          TARGET partition_manager
          PROPERTY ${d}_${name}
          ${${name}}
          )
      endforeach()

      if (CONFIG_NRF53_UPGRADE_NETWORK_CORE
          AND CONFIG_HCI_RPMSG_BUILD_STRATEGY_FROM_SOURCE)
          # The memory mapped address of the external flash is different
          # for the net core and the app core
          if(CONFIG_SOC_NRF5340_CPUAPP)
            set(EXT_ADDR 0x10000000)
          else()
            set(EXT_ADDR 0x12000000) # TODO Remove this and rework the logics
          endif()

          # Create symbols for the offset reqired for moving the signed network
          # core application to MCUBoots secondary slot. This is needed
          # because  objcopy does not support arithmetic expressions as argument
          # (e.g. '0x100+0x200'), and all of the symbols used to generate the
          # offset are only available as a generator expression when MCUBoots
          # cmake code exectues.
          if(CONFIG_PM_EXTERNAL_FLASH)
            secondary_slot(EXT_START ${EXT_ADDR} EXT_BASE ${CONFIG_PM_EXTERNAL_FLASH_BASE})
          else()
            secondary_slot()
          endif()
        endif()

    endif()
  endforeach()

  # Explicitly add the root image domain hex file to the list
  list(APPEND domain_hex_files ${PROJECT_BINARY_DIR}/${merged}.hex)
  list(APPEND global_hex_depends ${merged}_hex)

  # Now all partition manager configuration from all images and domains are
  # available. Generate the global pm_config.h, and provide it to all images.
  set(pm_global_output_cmd
    ${PYTHON_EXECUTABLE}
    ${NRF_DIR}/scripts/partition_manager_output.py
    --input-partitions ${pm_out_partition_file}
    --input-regions ${pm_out_region_file}
    --header-files ${header_files}
    --images ${prefixed_images}
    )

  execute_process(
    COMMAND
    ${pm_global_output_cmd}
    RESULT_VARIABLE ret
    )

  if(NOT ${ret} EQUAL "0")
    message(FATAL_ERROR "Partition Manager GLOBAL output generation failed,
    aborting. Command: ${pm_global_output_cmd}")
  endif()

  set_property(
    TARGET partition_manager
    PROPERTY PM_CONFIG_FILES
    ${pm_out_partition_file}
    )

  set_property(
    TARGET partition_manager
    PROPERTY PM_DEPENDS
    ${global_hex_depends}
    )

  if (PM_DOMAINS)
    # For convenience, generate global hex file containing all domains' hex
    # files.
    set(final_merged ${ZEPHYR_BINARY_DIR}/merged_domains.hex)

    # Add command to merge files.
    add_custom_command(
      OUTPUT ${final_merged}
      COMMAND
      ${PYTHON_EXECUTABLE}
      ${ZEPHYR_BASE}/scripts/mergehex.py
      -o ${final_merged}
      ${domain_hex_files}
      DEPENDS
      ${global_hex_depends}
      )

    # Wrapper target for the merge command.
    add_custom_target(merged_domains_hex ALL DEPENDS ${final_merged})
  endif()

  set(ZEPHYR_RUNNER_CONFIG_KERNEL_HEX "${final_merged}"
    CACHE STRING "Path to merged image in Intel Hex format" FORCE)

endif()

# We need to tell the flash runner use the merged hex file instead of
# 'zephyr.hex'This is typically done by setting the 'hex_file' property of the
# 'runners_yaml_props_target' target. However, since the CMakeLists.txt file
# reading those properties has already run, and the 'hex_file' property
# is not evaluated in a generator expression, it is too late at this point to
# set that variable. Hence we must operate on the 'yaml_contents' property,
# which is evaluated in a generator expression.

if (final_merged)
  # Multiple domains are included in the build, point to the result of
  # merging the merged hex file for all domains.
  set(merged_hex_to_flash ${final_merged})
else()
  set(merged_hex_to_flash ${PROJECT_BINARY_DIR}/${merged}.hex)
endif()

get_target_property(runners_content runners_yaml_props_target yaml_contents)

string(REGEX REPLACE "hex_file:[^\n]*"
  "hex_file: ${merged_hex_to_flash}" new  ${runners_content})

set_property(
  TARGET         runners_yaml_props_target
  PROPERTY       yaml_contents
  ${new}
  )
