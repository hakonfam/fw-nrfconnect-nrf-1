# Included by zephyr/cmake/extensions.cmake

# Usage:
#   add_partition_manager_config(pm.yml)
#
# Will add all configurations defined in pm.yml to the global list of partition
# manager configurations.
function(add_partition_manager_config config_file)
  get_filename_component(pm_path ${config_file} REALPATH)
  set_property(
    GLOBAL APPEND PROPERTY
    ${BOARD}_${IMAGE}PM_SUBSYS #TODO I don't think this will work
    ${pm_path}
    )
endfunction()

# Set 'domain_out' to domain for given board
function(get_domain board domain_out)
  string(REGEX MATCH "(.*)ns$" unused_out_var ${board})
  if (CMAKE_MATCH_1)
    set(${domain_out} ${CMAKE_MATCH_1} PARENT_SCOPE)
  else()
    set(${domain_out} ${board} PARENT_SCOPE)
  endif()
endfunction()
