#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

# Set 'domain_out' to domain for given board
function(get_domain board domain_out)
  string(REGEX MATCH "(.*)ns$" unused_out_var ${board})
  if (CMAKE_MATCH_1)
    set(${domain_out} ${CMAKE_MATCH_1} PARENT_SCOPE)
  else()
    set(${domain_out} ${board} PARENT_SCOPE)
  endif()
endfunction()
