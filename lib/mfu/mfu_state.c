/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "mfu_state.h"

#include <zephyr.h>

static const char *state_names[] = {
#define X(_name) STRINGIFY(_name),
	MFU_STATE_LIST
#undef X
};

const char *mfu_state_name_get(enum mfu_state state)
{
	__ASSERT_NO_MSG(state < ARRAY_SIZE(state_names));

	return state_names[state];
}
