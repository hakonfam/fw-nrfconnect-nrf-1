zephyr_compile_definitions(
  USE_PARTITION_MANAGER=$<TARGET_EXISTS:partition_manager>
  )
set_property(GLOBAL APPEND PROPERTY
  ${IMAGE}PROPERTY_LINKER_SCRIPT_DEFINES
  -DUSE_PARTITION_MANAGER=$<TARGET_EXISTS:partition_manager>
  )

if(EXISTS ${APPLICATION_SOURCE_DIR}/pm.yml)
  execute_process(
    COMMAND ${CMAKE_C_COMPILER}
    -x assembler-with-cpp
    -nostdinc
    -I${PROJECT_BINARY_DIR}/include/generated
    ${NOSYSDEF_CFLAG}
    -P
    -E ${APPLICATION_SOURCE_DIR}/pm.yml
    -o ${PROJECT_BINARY_DIR}/include/generated/pm.yml
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    RESULT_VARIABLE ret
    )
  if(NOT "${ret}" STREQUAL "0")
    message(FATAL_ERROR "command failed with return code: ${ret}")
  endif()
  if (IMAGE)
    string(LENGTH ${IMAGE} len)
    MATH(EXPR len "${len}-1")
    string(SUBSTRING ${IMAGE} 0 ${len} image_name)
  else()
    # It is required that a partition with the same name as the application
    # folder is defined in the pm.yml of an application.
    # So it is safe to use this as the image name.
    get_filename_component(image_name ${APPLICATION_SOURCE_DIR} NAME)
  endif()
  set_property(
    GLOBAL APPEND PROPERTY
    PARTITION_MANAGER_CONFIG_FILES
    "${image_name}:${PROJECT_BINARY_DIR}/include/generated/pm.yml:${PROJECT_BINARY_DIR}:${PROJECT_BINARY_DIR}/include/generated"
    )
endif()
