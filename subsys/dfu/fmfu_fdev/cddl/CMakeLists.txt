#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

# This file contains the logic for generating the decoder for the
# 'modem_update.cddl' CDDL schema. Execute the 'cddl_gen_modem_update'
# target to run the generation procedure.

set(src_out ${NRF_DIR}/subsys/dfu/fmfu_fdev/src)
set(include_out ${NRF_DIR}/subsys/dfu/fmfu_fdev/include)

set(cddl_file ${CMAKE_CURRENT_LIST_DIR}/modem_update.cddl)
set(decode_c ${src_out}/modem_update_decode.c)
set(decode_h ${include_out}/modem_update_decode.h)
set(types_h ${include_out}/modem_update_types.h)

set(entry_types Wrapper Sig_structure1 Segments)

add_custom_target(
  cddl_gen_modem_update
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${ZEPHYR_CDDL_GEN_MODULE_DIR}/scripts/cddl_gen.py
  -i ${cddl_file}
  --oc ${decode_c}
  --oh ${decode_h}
  --oht ${types_h}
  -t ${entry_types}
  -d # Decode
  --default-maxq 16 # Up to 16 segments is supported.
  COMMENT
  "Re-generating files based on ${cddl_file}"
  )
