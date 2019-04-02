cmake_minimum_required(VERSION 3.8.2)

zephyr_add_executable(spm require_build)

if (${require_build})
  include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
  project(NONE)

  target_sources(${IMAGE}app PRIVATE src/main.c)

  set_property(
    GLOBAL APPEND PROPERTY
    HEX_FILES_TO_MERGE
    ${ZEPHYR_BINARY_DIR}/${KERNEL_HEX_NAME}
    )

  set_property(
    GLOBAL APPEND PROPERTY
    HEX_FILES_TO_MERGE_TARGET
    ${IMAGE}kernel_elf
    )

  set_property(
    GLOBAL PROPERTY
    MERGE_ROOT_APP
    1)

endif ()
