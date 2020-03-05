/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef __MFU_STATE_H__
#define __MFU_STATE_H__

#define MFU_STATE_LIST \
	X(NO_UPDATE_AVAILABLE)\
	X(UPDATE_AVAILABLE)\
	X(INSTALLING)\
	X(INSTALL_FINISHED)

enum mfu_state {
#define X(_name) MFU_STATE_##_name,
	MFU_STATE_LIST
#undef X
};

int mfu_state_init(void *device, int offset);
enum mfu_state mfu_state_get(void);
int mfu_state_set(enum mfu_state new_state);
const char *mfu_state_name_get(enum mfu_state state);

#endif /* __MFU_STATE_H__*/
