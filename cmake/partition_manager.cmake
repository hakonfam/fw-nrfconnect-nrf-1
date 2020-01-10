#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

set(static_configuration_file ${APPLICATION_SOURCE_DIR}/pm_static.yml)

if (IMAGE_NAME OR
    (NOT PM_DOMAINS AND (NOT (EXISTS ${static_configuration_file}))))
  # Don't run patition manager for sub-images or if no domains configured and
  # no static configuration is provided.
  return()
endif()

# TODO make it so that this is not required
list(REMOVE_DUPLICATES PM_DOMAINS)

# Create a variable exposing the correct logical target for the current image.
get_domain(${BOARD} domain)

function(get_image_name image out_var)
  string(LENGTH ${image} len)
  MATH(EXPR len "${len}-1")
  string(SUBSTRING ${image} 0 ${len} ${out_var})
  set(${out_var} ${${out_var}} PARENT_SCOPE)
endfunction()

# Get a local copy of the list of domains

# Merge x and x_ns domains.
# Add all partitions from x_ns list to x, and delete x_ns from domain list
foreach (non_secure ${PM_DOMAINS})
  string(REGEX MATCH "(.*)ns$" unused_out_var ${non_secure})
  if (CMAKE_MATCH_1)
    # We found an "x_ns" board, find the corresponding "x" board
    foreach (secure in ${PM_DOMAINS})
      if (${secure} STREQUAL ${CMAKE_MATCH_1})
        # Merge the lists and delete the _ns domain
        foreach (part in PM_IMAGES_${non_secure})
          list(APPEND PM_IMAGES PM_IMAGES_${secure} ${part})
        endforeach()
        list(APPEND ${to_remove} ${non_secure})
      endif()
    endforeach()
  endif()
endforeach()
foreach(non_secure ${to_remove})
  list(REMOVE ${PM_DOMAINS} non_secure)
endforeach()

# Create a dummy target that we can add properties to for
# extraction in generator expressions.
add_custom_target(partition_manager)

set(generated_path include/generated)

foreach (d ${PM_DOMAINS})

  get_property(
    PM_IMAGES_${d}
    GLOBAL PROPERTY
    PM_IMAGES_${d})

  get_property(
    PM_SUBSYS_PREPROCESSED_${d}
    GLOBAL PROPERTY
    PM_SUBSYS_PREPROCESSED_${d})

  # Partition manager is enabled because we have populated PM_DOMAINS,
  # or because the application has specified a static configuration.
  if (EXISTS ${static_configuration_file})
    set(static_configuration --static-config ${static_configuration_file})
  endif()

  if (PM_${d}_DYNAMIC_PARTITION)
    set(dyn ${PM_${d}_DYNAMIC_PARTITION})
    set(dyn_prjbin ${${d}_${dyn}_PROJECT_BINARY_DIR})
    set(dynamic_partition_arg "-d ${dyn}")
  else()
    # 'app' is the dynamic partition
    # Special treatment of the app image.
    list(APPEND images ${d}:app)
    list(APPEND ${d}_input_files ${PROJECT_BINARY_DIR}/${generated_path}/pm.yml)
    list(APPEND header_files ${PROJECT_BINARY_DIR}/${generated_path}/pm_config.h)

    set(${d}_app_PROJECT_BINARY_DIR ${PROJECT_BINARY_DIR})

    set_property(GLOBAL PROPERTY
      PM_${d}_app_HEX_FILE
      ${PROJECT_BINARY_DIR}/${KERNEL_HEX_NAME}
      )

    set_property(GLOBAL PROPERTY
      PM_${d}_app_TARGET
      ${logical_target_for_zephyr_elf}
      )
  endif()

  # Prepare the input_files, header_files, and images lists
  foreach (image_name ${PM_IMAGES_${d}})
    list(APPEND ${d}_images ${image_name}) # List per domain
    list(APPEND images ${d}:${image_name}) # Global list prefixed with domain

    list(APPEND
      ${d}_input_files
      ${${d}_${image_name}_PROJECT_BINARY_DIR}/${generated_path}/pm.yml
      )

    list(APPEND
      header_files
      ${${d}_${image_name}_PROJECT_BINARY_DIR}/${generated_path}/pm_config.h
      )

  endforeach()

  # Add subsys defined pm.yml to the input_files
  list(APPEND ${d}_input_files ${PM_SUBSYS_PREPROCESSED_${d}})

  set(pm_out ${CMAKE_BINARY_DIR}/partitions_${d}.yml)
  # Store in list to create global configuration across domains

  list(APPEND pm_out_files ${pm_out})

  set(pm_cmd
    ${PYTHON_EXECUTABLE}
    ${NRF_DIR}/scripts/partition_manager.py
    --input-files ${${d}_input_files}
    --flash-start ${PM_DOMAINS_${d}_FLASH_BASE_ADDRESS}
    --flash-size ${PM_DOMAINS_${d}_FLASH_SIZE}
    --output ${pm_out}
    ${dynamic_partition_arg}
    ${static_configuration}
    )

  # Run the partition manager algorithm.
  execute_process(
    COMMAND
    ${pm_cmd}
    RESULT_VARIABLE ret
    )

  if(NOT ${ret} EQUAL "0")
    message(FATAL_ERROR "Partition Manager failed, aborting."
      "Command: ${pm_cmd}")
  endif()
endforeach()

set(pm_output_cmd
  ${PYTHON_EXECUTABLE}
  ${NRF_DIR}/scripts/partition_manager_output.py
  --input ${pm_out_files}
  --config-file ${CMAKE_BINARY_DIR}/pm.config
  --images ${images}
  --header-files ${header_files}
  )

# Produce header files and config file.
execute_process(
  COMMAND
  ${pm_output_cmd}
  RESULT_VARIABLE ret
  )

if(NOT ${ret} EQUAL "0")
  message(
    FATAL_ERROR
    "Partition Manager output generation failed, aborting."
    "Command: ${pm_output_cmd}")
endif()

# Make Partition Manager configuration available in CMake
import_kconfig(PM_ ${CMAKE_BINARY_DIR}/pm.config pm_var_names)

foreach(name ${pm_var_names})
  set_property(
    TARGET partition_manager
    PROPERTY ${name}
    ${${name}}
    )
endforeach()


foreach (d ${PM_DOMAINS})
  # Turn the space-separated list into a Cmake list.
  string(REPLACE " " ";" PM_${d}_ALL_BY_SIZE ${PM_${d}_ALL_BY_SIZE})

  # Iterate over every partition, from smallest to largest.
  # Assign hex files to partitions.
  # Populate lists of implicitly and explicitly assigned partition targets.
  foreach(part ${PM_${d}_ALL_BY_SIZE})
    string(TOUPPER ${part} PART)

    # Process container partitions (if it has a SPAN list it is a container
    # partition). Note that PART is written upper case in imported values.
    if(DEFINED PM_${d}_${PART}_SPAN)
      string(REPLACE " " ";" PM_${d}_${PART}_SPAN ${PM_${d}_${PART}_SPAN})
      list(APPEND ${d}_containers ${part})
    endif()

    # Include the partition in the merge operation if it has a hex file set
    # explicitly. This override any implicitly set hex file.
    get_property(PM_${d}_${part}_HEX_FILE GLOBAL PROPERTY PM_${d}_${part}_HEX_FILE)
    if(DEFINED PM_${d}_${part}_HEX_FILE)
      get_property(PM_${d}_${part}_TARGET GLOBAL PROPERTY PM_${d}_${part}_TARGET)
      list(APPEND ${d}_explicitly_assigned ${part})
    else()
      if(${part} IN_LIST ${d}_images)
        # The partition is an image partition. Get the path to the zephyr.hex
        # for this image and the target for building that hex file.
        set(PM_${d}_${part}_HEX_FILE ${${d}_${part}_PROJECT_BINARY_DIR}/zephyr.hex)
        set(PM_${d}_${part}_TARGET ${d}_${part}_subimage)
      elseif(${part} IN_LIST ${d}_containers)
        # The partition is a container/span partition. Set the '_HEX_FILE'
        # and '_TARGET' values to the name of the container. These hex
        # files will be populated with they hex files they span.
        set(PM_${d}_${part}_HEX_FILE ${PROJECT_BINARY_DIR}/${d}_${part}.hex)
        set(PM_${d}_${part}_TARGET ${d}_${part}_hex)
      endif()
      list(APPEND ${d}_implicitly_assigned ${part})
    endif()
  endforeach()

  # Create span for the default 'merged' partition
  set(PM_${d}_MERGED_SPAN
    ${${d}_implicitly_assigned} ${${d}_explicitly_assigned}
    )

  set(MERGED_overlap TRUE) # Enable overlapping for the merged hex file.

  # Iterate over all container partitions, plus the "fake" merged paritition.
  # The loop will create a hex file for each iteration.
  foreach(container ${${d}_containers} MERGED)
    string(TOUPPER ${container} CONTAINER)

    # Prepare the list of hex files and list of dependencies for the merge
    # command. Add all hex files inside span of container.
    foreach(part ${PM_${d}_${CONTAINER}_SPAN})
      list(APPEND ${d}_${container}_hex_files ${PM_${d}_${part}_HEX_FILE})
      list(APPEND ${d}_${container}_targets ${PM_${d}_${part}_TARGET})
    endforeach()


    if(${${container}_overlap})
      set(${container}_overlap_arg --overlap=replace)
    else()
      set(${container}_overlap_arg "")
    endif()

    set(merge_out ${PROJECT_BINARY_DIR}/${d}_${container}.hex)

    print(${d}_${container}_hex_files)
    # Add command to merge files.
    add_custom_command(
      OUTPUT ${merge_out}
      COMMAND
      ${PYTHON_EXECUTABLE}
      ${ZEPHYR_BASE}/scripts/mergehex.py
      -o ${merge_out}
      ${${container}_overlap_arg}
      ${${d}_${container}_hex_files}
      DEPENDS
      ${${d}_${container}_targets}
      )

    message("creating ")
    print(${d}_${container}_hex)
    # Wrapper target for the merge command.
    # We have to prepend with domain to make it unique.
    add_custom_target(
      ${d}_${container}_hex
      DEPENDS
      ${merge_out}
      )

    if ("${container}" STREQUAL "MERGED")
      list(APPEND domain_hex_files ${merge_out})
      list(APPEND domain_targets ${d}_${container}_hex)
    endif()
  endforeach()
endforeach()

# Merged hex files for all domains are created.
# Now, merge these together in order to get the "global" domain.
add_custom_command(
  OUTPUT ${PROJECT_BINARY_DIR}/merged.hex
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${ZEPHYR_BASE}/scripts/mergehex.py
  -o ${PROJECT_BINARY_DIR}/merged.hex
  --overlap=replace
  ${domain_hex_files}
  DEPENDS
  ${domain_hex_files} ${domain_targets}
  )

add_custom_target(merged_hex ALL DEPENDS
  ${PROJECT_BINARY_DIR}/merged.hex)

# Add $merged.hex as the representative hex file for flashing this app.
if(TARGET flash)
  add_dependencies(flash merged_hex)
endif()
set(ZEPHYR_RUNNER_CONFIG_KERNEL_HEX "${PROJECT_BINARY_DIR}/merged.hex"
  CACHE STRING "Path to merged image in Intel Hex format" FORCE)

if (CONFIG_SECURE_BOOT AND CONFIG_BOOTLOADER_MCUBOOT)
  # Create symbols for the offsets required for moving test update hex files
  # to MCUBoots secondary slot. This is needed because objcopy does not
  # support arithmetic expressions as argument (e.g. '0x100+0x200'), and all
  # of the symbols used to generate the offset is only available as a
  # generator expression when MCUBoots cmake code exectues. This because
  # partition manager is performed as the last step in the configuration
  # stage.
  get_domain(${BOARD} domain)
  math(EXPR s0_offset
    "${PM_${domain}_MCUBOOT_SECONDARY_ADDRESS} - ${PM_${domain}_S0_ADDRESS}")
  math(EXPR s1_offset
    "${PM_${domain}_MCUBOOT_SECONDARY_ADDRESS} - ${PM_${domain}_S1_ADDRESS}")

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
