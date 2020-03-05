/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "mfu_state.h"

#include <string.h>
#include <zephyr.h>
#include <settings/settings.h>
#include <logging/log.h>

#define SETTINGS_KEY "mfu"
#define SETTINGS_SUBKEY_STATE "state"

static enum mfu_state state;

LOG_MODULE_REGISTER(mfu_settings, CONFIG_MFU_LOG_LEVEL);

static int settings_set(const char *key, size_t len_rd,
			settings_read_cb read_cb, void *cb_arg)
{
	if (strcmp(key, SETTINGS_SUBKEY_STATE) == 0) {
		enum mfu_state loaded_state;
		ssize_t len = read_cb(cb_arg, &loaded_state, sizeof(loaded_state));

		if (len != sizeof(loaded_state)) {
			LOG_ERR("Unexpected state length: %d", len);
		} else {
			LOG_DBG("Loaded state: %s", mfu_state_name_get(loaded_state));
			state = loaded_state;
		}
	} else {
		LOG_WRN("Unknown setting: %s", key);
	}

	return 0;
}

static int settings_store(void)
{
	static char state_key[] = SETTINGS_KEY "/" SETTINGS_SUBKEY_STATE;
	int err;

	err = settings_save_one(state_key, &state, sizeof(state));
	if (err) {
		LOG_ERR("settings_save_one: %d", err);
		return err;
	} else {
		LOG_DBG("Stored state: %s", mfu_state_name_get(state));
	}

	return 0;
}

static int commit(void)
{
	return 0;
}

int mfu_state_init(void *device, int offset)
{
	int err;

	state = MFU_STATE_NO_UPDATE_AVAILABLE;

	err = settings_subsys_init();
	if (err) {
		LOG_ERR("settings_subsys_init: %d", err);
		return err;
	}

	static struct settings_handler sh = {
		.name = SETTINGS_KEY,
		.h_set = settings_set,
		.h_commit = commit,
	};

	err = settings_register(&sh);
	if (err) {
		LOG_ERR("settings_register: %d", err);
		return err;
	}

	err = settings_load();
	if (err) {
		LOG_ERR("settings_load: %d", err);
		return err;
	}

	return 0;
}

enum mfu_state mfu_state_get(void)
{
	return state;
}

int mfu_state_set(enum mfu_state new_state)
{
	state = new_state;

	return settings_store();
}