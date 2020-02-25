#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

define_property(GLOBAL PROPERTY PM_IMAGES
  BRIEF_DOCS "A list of all images that should be processed by the Partition Manager."
  FULL_DOCS "A list of all images that should be processed by the Partition Manager.
Each image's directory will be searched for a pm.yml, and will receive a pm_config.h header file with the result.
Also, the each image's hex file will be automatically associated with its partition.")

macro(add_region name size base placement_strategy)
  list(APPEND regions ${name})
  list(APPEND region_arguments "--${name}-size;${size}")
  list(APPEND region_arguments "--${name}-base-address;${base}")
  list(APPEND region_arguments "--${name}-placement-strategy;${placement_strategy}")
endmacro()

macro(add_region_with_dev name size base placement_strategy device)
  add_region(${name} ${size} ${base} ${placement_strategy})
  list(APPEND region_arguments "--${name}-device;${device}")
endmacro()

get_property(PM_IMAGES GLOBAL PROPERTY PM_IMAGES)
get_property(PM_SUBSYS_PREPROCESSED GLOBAL PROPERTY PM_SUBSYS_PREPROCESSED)
get_domain(${BOARD} domain)

if(EXISTS "${PM_STATIC_YML_FILE}" AND NOT IS_DIRECTORY "${PM_STATIC_YML_FILE}")
  set(static_configuration_file ${PM_STATIC_YML_FILE})
else()
  set(static_configuration_file ${APPLICATION_SOURCE_DIR}/pm_static.yml)
endif()

if("${IMAGE_NAME}" STREQUAL "${PM_${domain}_DYNAMIC_PARTITION}_")
  set(is_dynamic_partition_in_domain TRUE)
endif()

if ((IMAGE_NAME AND NOT is_dynamic_partition_in_domain) OR
    (NOT PM_DOMAINS AND (NOT (EXISTS ${static_configuration_file}))))
  # Don't run patition manager for non-dynamic sub-images or if no domains configured and
  # no static configuration is provided.
  return()
endif()

# Partition manager is enabled because we have populated PM_IMAGES,
# or because the application has specified a static configuration.
if (EXISTS ${static_configuration_file})
  set(static_configuration --static-config ${static_configuration_file})
endif()

set(generated_path zephyr/include/generated)

# Set the dynamic partition. This is the only partition which does not
# have a statically defined size. There is only one dynamic partition per
# domain. For the "root domain" (ie the domain of the root image) this is
# always "app".
if (NOT is_dynamic_partition_in_domain)
  set(dynamic_partition "app")
else()
  set(dynamic_partition ${PM_${domain}_DYNAMIC_PARTITION})
  set(dynamic_partition_argument "-d ${dynamic_partition}")
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
foreach (image ${PM_IMAGES})
  list(APPEND prefixed_images ${domain}:${image})
  list(APPEND images ${image})
  list(APPEND input_files ${CMAKE_BINARY_DIR}/${image}/${generated_path}/pm.yml)
  list(APPEND header_files ${CMAKE_BINARY_DIR}/${image}/${generated_path}/pm_config.h)
endforeach()

list(APPEND prefixed_images "${domain}:${dynamic_partition}")
list(APPEND images ${dynamic_partition})
list(APPEND input_files ${CMAKE_BINARY_DIR}/${generated_path}/pm.yml)
list(APPEND header_files ${CMAKE_BINARY_DIR}/${generated_path}/pm_config.h)

# Add subsys defined pm.yml to the input_files
list(APPEND input_files ${PM_SUBSYS_PREPROCESSED})

set(pm_out ${CMAKE_BINARY_DIR}/partitions_${domain}.yml)
  if (DEFINED CONFIG_SOC_NRF9160)
    # See nRF9160 Product Specification, chapter "UICR"
    set(otp_start_addr "0xff8108")
    set(otp_size 756) # 189 * 4
  elseif (DEFINED CONFIG_SOC_NRF5340_CPUAPP)
    # See nRF5340 Product Specification, chapter Application Core -> ... "UICR"
    set(otp_start_addr "0xff8100")
    set(otp_size 764)  # 191 * 4
  endif()

  math(EXPR flash_size "${CONFIG_FLASH_SIZE} * 1024" OUTPUT_FORMAT HEXADECIMAL)

  if (CONFIG_SOC_NRF9160 OR CONFIG_SOC_NRF5340_CPUAPP)
    add_region(
      otp
      ${otp_size}
      ${otp_start_addr}
      start_to_end
      )
  endif()
  add_region_with_dev(
    flash_primary
    ${flash_size}
    ${CONFIG_FLASH_BASE_ADDRESS}
    complex
    NRF_FLASH_DRV_NAME
    )

  if (CONFIG_PM_EXTERNAL_FLASH)
    add_region_with_dev(
      external_flash
      ${CONFIG_PM_EXTERNAL_FLASH_SIZE}
      ${CONFIG_PM_EXTERNAL_FLASH_BASE}
      start_to_end
      ${CONFIG_PM_EXTERNAL_FLASH_DEV_NAME}
      )
  endif()

set(pm_cmd
  ${PYTHON_EXECUTABLE}
  ${NRF_DIR}/scripts/partition_manager.py
  --input-files ${input_files}
    --regions ${regions}
    --output-partitions ${CMAKE_BINARY_DIR}/partitions.yml
    --output-regions ${CMAKE_BINARY_DIR}/regions.yml
  ${dynamic_partition_argument}
  ${static_configuration}
    ${region_arguments}
  )

set(pm_output_out ${CMAKE_BINARY_DIR}/pm_${domain}.config)
set(pm_output_cmd
  ${PYTHON_EXECUTABLE}
  ${NRF_DIR}/scripts/partition_manager_output.py
    --input-partitions ${CMAKE_BINARY_DIR}/partitions.yml
    --input-regions ${CMAKE_BINARY_DIR}/regions.yml
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
import_kconfig(PM_ ${pm_output_out} pm_var_names)

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
      set(${part}_PM_HEX_FILE ${CMAKE_BINARY_DIR}/${part}/zephyr/${${part}_KERNEL_HEX_NAME})
        set(${part}_PM_ELF_FILE ${CMAKE_BINARY_DIR}/${part}/zephyr/${${part}_KERNEL_ELF_NAME})
      set(${part}_PM_TARGET ${part}_subimage)
    elseif(${part} IN_LIST containers)
      set(${part}_PM_HEX_FILE ${PROJECT_BINARY_DIR}/${part}.hex)
      set(${part}_PM_TARGET ${part}_hex)
    endif()
    list(APPEND implicitly_assigned ${part})
  endif()
endforeach()

string(TOUPPER ${domain} DOMAIN)
set(PM_MERGED_${DOMAIN}_SPAN ${implicitly_assigned} ${explicitly_assigned})
set(merged_${domain}_overlap TRUE) # Enable overlapping for the merged hex file.

# Iterate over all container partitions, plus the "fake" merged paritition.
# The loop will create a hex file for each iteration.
foreach(container ${containers} merged_${domain})
  string(TOUPPER ${container} CONTAINER)

  # Prepare the list of hex files and list of dependencies for the merge command.
  message("Checking PM_${CONTAINER}_SPAN -> ${PM_${CONTAINER}_SPAN}")
  foreach(part ${PM_${CONTAINER}_SPAN})
    string(TOUPPER ${part} PART)
    list(APPEND ${container}hex_files ${${part}_PM_HEX_FILE})
      list(APPEND ${container}elf_files ${${part}_PM_ELF_FILE})
    list(APPEND ${container}targets ${${part}_PM_TARGET})
  endforeach()

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
      # SES will load symbols from all elf files listed as dependencies to ${PROJECT_BINARY_DIR}/merged.hex.
      # Therefore we add ${${container}elf_files} as dependency to ensure they are loaded by SES even though
      # it is unnecessary for building the application.
      ${${container}elf_files}
    )

  # Wrapper target for the merge command.
  add_custom_target(${container}_hex ALL DEPENDS ${PROJECT_BINARY_DIR}/${container}.hex)
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

if (is_dynamic_partition_in_domain)  # We are being built as sub image
  # Expose the generated pm_${domain}.config file to root image.
  set_property(
    TARGET         zephyr_property_target
    APPEND_STRING
    PROPERTY       shared_vars
    "set(PM_DOMAINS_${domain}_CONFIG ${pm_out})\n"
    )

  set_property(
    TARGET         zephyr_property_target
    APPEND_STRING
    PROPERTY       shared_vars
    "set(PM_DOMAINS_${domain}_HEADER_FILES ${header_files})\n"
    )

  set_property(
    TARGET         zephyr_property_target
    APPEND_STRING
    PROPERTY       shared_vars
    "set(PM_DOMAINS_${domain}_IMAGES ${prefixed_images})\n"
    )

  if (NOT ("${IMAGE_NAME}" STREQUAL "${PM_${domain}_DYNAMIC_PARTITION}_"))
    set_property(
      TARGET         zephyr_property_target
      APPEND_STRING
      PROPERTY       shared_vars
      "set(PM_DOMAINS_${domain}_HEX_FILE ${PROJECT_BINARY_DIR}/merged_${domain}.hex)\n"
      )
  endif()
else()
  # This is the root image, generate the global pm_config.h files
  list(REMOVE_DUPLICATES PM_DOMAINS)
  foreach (d ${PM_DOMAINS})
    # Don't include shared vars from own domain.
    if (NOT ${domain} STREQUAL ${d})
      set(shared_vars_file
        ${CMAKE_BINARY_DIR}/${PM_${d}_DYNAMIC_PARTITION}/shared_vars.cmake
        )
      if (NOT (EXISTS ${shared_vars_file}))
        message(FATAL_ERROR "Could not find shared vars file: ${shared_vars_file}")
      endif()
      include(${shared_vars_file})
      list(APPEND header_files ${PM_DOMAINS_${d}_HEADER_FILES})
      list(APPEND prefixed_images ${PM_DOMAINS_${d}_IMAGES})
      list(APPEND pm_out ${PM_DOMAINS_${d}_CONFIG})
      list(APPEND domain_hex_files ${PM_DOMAINS_${d}_HEX_FILE})
      message("APPENDED is now ${domain_hex_files}")
      list(APPEND domain_hex_depends ${PM_${d}_DYNAMIC_PARTITION}_subimage)
    endif()
  endforeach()

  # Add the root domains hex file to the list
  list(APPEND domain_hex_files ${PROJECT_BINARY_DIR}/merged_${domain}.hex)
  list(APPEND domain_hex_depends merged_${domain}_hex)

  print(prefixed_images)
  print(header_files)
  set(pm_global_output_cmd
    ${PYTHON_EXECUTABLE}
    ${NRF_DIR}/scripts/partition_manager_output.py
    --input ${pm_out}
    --header-files ${header_files}
    --images ${prefixed_images}
    )

  # Produce header files and config file.
  execute_process(
    COMMAND
    ${pm_global_output_cmd}
    RESULT_VARIABLE ret
    )

  if(NOT ${ret} EQUAL "0")
    message(FATAL_ERROR "Partition Manager GLOBAL output generation failed,
    aborting. Command: ${pm_global_output_cmd}")
  endif()

  set(final_merged ${PROJECT_BINARY_DIR}/merged.hex)
  print(domain_hex_files)

  # Add command to merge files.
  add_custom_command(
    OUTPUT ${final_merged}
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/scripts/mergehex.py
    -o ${final_merged}
    ${domain_hex_files}
    DEPENDS
    ${domain_hex_depends}
    )

  # Wrapper target for the merge command.
  add_custom_target(merged_hex ALL DEPENDS ${final_merged})
  # Add merged.hex as the representative hex file for flashing this app.
  if(TARGET flash)
    add_dependencies(flash merged_hex)
  endif()
  set(ZEPHYR_RUNNER_CONFIG_KERNEL_HEX "${final_merged}"
    CACHE STRING "Path to merged image in Intel Hex format" FORCE)

endif()
