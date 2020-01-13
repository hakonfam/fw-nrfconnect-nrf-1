#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

function(preprocess_pm_yml in_file out_file)
  execute_process(
    COMMAND ${CMAKE_C_COMPILER}
    -x assembler-with-cpp
    -nostdinc
    -I${PROJECT_BINARY_DIR}/include/generated
    ${NOSYSDEF_CFLAG}
    -P
    -E ${in_file}
    -o ${out_file}
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    RESULT_VARIABLE ret
    )
  if(NOT "${ret}" STREQUAL "0")
    message(FATAL_ERROR "command failed with return code: ${ret}")
  endif()

endfunction()

# We are using partition manager if we are a child image or if we are
# the root image and the 'partition_manager' target exists.
set(using_partition_manager
  $<OR:$<BOOL:${IMAGE_NAME}>,$<TARGET_EXISTS:partition_manager>>
  )
zephyr_compile_definitions(
  USE_PARTITION_MANAGER=${using_partition_manager}
  )

# TODO: check how this patch got lost and if more are missing
set_property(GLOBAL APPEND PROPERTY
  PROPERTY_LINKER_SCRIPT_DEFINES
  -DUSE_PARTITION_MANAGER=${using_partition_manager}
  )

get_domain(${BOARD} domain)

if (((EXISTS ${CMAKE_SOURCE_DIR}/pm.yml) AND IMAGE_NAME) OR
  ("${IMAGE_NAME}" STREQUAL "${PM_${domain}_DYNAMIC_PARTITION}_"))
  # Only preprocess pm.yml when being built as sub image.


  # Only add ${BOARD} as domain if not already added
  if (NOT (${BOARD} IN_LIST PM_DOMAINS))
    set_property(
      TARGET         zephyr_property_target
      APPEND_STRING
      PROPERTY       shared_vars
      "list(APPEND PM_DOMAINS ${BOARD})\n"
      )

    set_property(
      TARGET         zephyr_property_target
      APPEND_STRING
      PROPERTY       shared_vars
      "set(PM_DOMAINS_${BOARD}_FLASH_BASE_ADDRESS ${CONFIG_FLASH_BASE_ADDRESS} CACHE STRING \"\" FORCE)\n"
      )

    set_property(
      TARGET         zephyr_property_target
      APPEND_STRING
      PROPERTY       shared_vars
      "set(PM_DOMAINS_${BOARD}_FLASH_SIZE ${CONFIG_FLASH_SIZE} CACHE STRING \"\" FORCE)\n"
      )
  endif()

  # Only non-dynamic sub-images will have pm.yml files.
  # E.g. the main app on a domain will not have a pm.yml file.
  if (EXISTS ${CMAKE_SOURCE_DIR}/pm.yml)
    preprocess_pm_yml(
      ${CMAKE_SOURCE_DIR}/pm.yml
      ${PROJECT_BINARY_DIR}/include/generated/pm.yml
      )
    set_property(
      GLOBAL APPEND PROPERTY
      PM_IMAGES_${BOARD}
      ${IMAGE}
      )
  endif()
endif()


get_property(
  img_pm_subsys
  GLOBAL PROPERTY
  ${IMAGE}PM_SUBSYS # ${IMAGE} ends with '_' 
  )

if (img_pm_subsys)
  foreach (subsys_pm_yml ${img_pm_subsys})
    file(RELATIVE_PATH rel_path_to_yml ${ZEPHYR_BASE} ${subsys_pm_yml})
    set(subsys_pm_preprocessed
      ${PROJECT_BINARY_DIR}/${rel_path_to_yml})
    preprocess_pm_yml(
      ${subsys_pm_yml}
      ${subsys_pm_preprocessed}
      )
    set_property(
      GLOBAL APPEND PROPERTY
      PM_SUBSYS_PREPROCESSED
      ${subsys_pm_preprocessed}
      )
  endforeach()
endif()
