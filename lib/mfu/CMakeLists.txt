#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

target_sources_ifdef(CONFIG_MFU_ENABLE app PRIVATE
		     ${CMAKE_CURRENT_SOURCE_DIR}/mfu.c
		     ${CMAKE_CURRENT_SOURCE_DIR}/mfu_types.c
		     ${CMAKE_CURRENT_SOURCE_DIR}/modem_dfu_rpc.c)

target_sources_ifdef(CONFIG_MFU_LOG_LEVEL_DBG
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/mfu_state.c)

target_sources_ifdef(CONFIG_MFU_STATE_SETTINGS
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/mfu_state_ncs_settings.c)

target_sources_ifdef(CONFIG_MFU_STATE_EXTFLASH
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/mfu_state_ext_flash.c)
