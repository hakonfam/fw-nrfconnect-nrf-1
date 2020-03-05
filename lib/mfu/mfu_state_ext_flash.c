/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "mfu_state.h"

#include <string.h>
#include <zephyr.h>
#include <sys/crc.h>
#include <drivers/flash.h>
#include <logging/log.h>

#define MAGIC_HEADER_NUMBER 0x55B4ED91

LOG_MODULE_REGISTER(mfu_state, CONFIG_MFU_LOG_LEVEL);

struct state_layout {
	u32_t magic_number;
	u32_t change_counter;
	enum mfu_state state;
	u32_t crc32;
};

static struct state_layout state;
static struct device *flash_dev;
static u32_t flash_offset;
static u32_t pagesize;
static u8_t current_page;

static bool state_struct_load(u32_t addr, struct state_layout *state)
{
	u32_t crc32;
	int err;

	err = flash_read(flash_dev, addr, state, sizeof(*state));
	if (err) {
		LOG_ERR("flash_read: %d", err);
		return false;
	}

	if (state->magic_number != MAGIC_HEADER_NUMBER) {
		return false;
	}

	crc32 = crc32_ieee((u8_t *) state, offsetof(struct state_layout, crc32));
	if (crc32 != state->crc32) {
		return false;
	}

	return true;
}

static int state_load(void)
{
	struct state_layout state1;
	struct state_layout state2;
	bool state1_valid;
	bool state2_valid;
	int err;

	state1_valid = state_struct_load(flash_offset, &state1);
	state2_valid = state_struct_load(flash_offset + pagesize, &state2);

	if (!state1_valid && !state2_valid) {
		LOG_DBG("No state info found. Initialize defaults");

		state.magic_number = MAGIC_HEADER_NUMBER;
		state.change_counter = 0;
		state.state =  MFU_STATE_NO_UPDATE_AVAILABLE;
		state.crc32 = crc32_ieee((u8_t *) &state, offsetof(struct state_layout, crc32));

		err = flash_erase(flash_dev, flash_offset, pagesize);
		if (err) {
			LOG_ERR("flash_erase: %d", err);
			return err;
		}

		err = flash_write(flash_dev, flash_offset, &state, sizeof(state));
		if (err) {
			LOG_ERR("flash_write: %d", err);
			return err;
		}

		current_page = 0;

		return 0;
	} else if (state1_valid && state2_valid) {
		LOG_DBG("Two valid state info found: %d vs %d",
			state1.change_counter,
			state2.change_counter);

		if (state1.change_counter >= state2.change_counter) {
			memcpy(&state, &state1, sizeof(state));
			current_page = 0;
		} else {
			memcpy(&state, &state2, sizeof(state));
			current_page = 1;
		}
	} else if (state1_valid) {
		LOG_DBG("Valid state found in page 0");
		memcpy(&state, &state1, sizeof(state));
		current_page = 0;
	} else {
		LOG_DBG("Valid state found in page 1");
		memcpy(&state, &state2, sizeof(state));
		current_page = 1;
	}

	LOG_DBG("Loaded state: %s", mfu_state_name_get(state.state));

	return 0;
}

static int state_store(void)
{
	u32_t offset;
	int err;

	LOG_DBG("state_store");

	offset = flash_offset;
	if (current_page == 0) {
		/* Do erase+write to alternating pages */
		offset += pagesize;
	}

	state.change_counter += 1;
	state.crc32 = crc32_ieee((u8_t *) &state, offsetof(struct state_layout, crc32));

	err = flash_erase(flash_dev, offset, pagesize);
	if (err) {
		LOG_ERR("flash_erase: %d", err);
		return err;
	}

	err = flash_write(flash_dev, offset, &state, sizeof(state));
	if (err) {
		LOG_ERR("flash_write: %d", err);
		return err;
	}

	/* Read back data */
	struct state_layout state_readout;

	err = flash_read(flash_dev, offset, &state_readout, sizeof(state_readout));
	if (err) {
		LOG_ERR("flash_read: %d", err);
		return err;
	}

	if (memcmp(&state, &state_readout, sizeof(state)) != 0) {
		LOG_ERR("Flash readback differs");
		return -ENODATA;
	}

	LOG_DBG("State stored to page %d", current_page);

	/* Safe to erase old state */

	current_page = current_page == 0 ? 1 : 0;

	offset = flash_offset;
	if (current_page == 0) {
		offset += pagesize;
	}

	err = flash_erase(flash_dev, offset, pagesize);
	if (err) {
		LOG_ERR("flash_erase: %d", err);
		return err;
	}

	LOG_DBG("Erased old state on page %d", current_page);

	return 0;
}

int mfu_state_init(void *device, int offset)
{
	if (!device || offset < 0) {
		LOG_ERR("Invalid dev");
		return -EINVAL;
	}

	LOG_DBG("mfu_state_init");

	flash_dev = device;
	flash_offset = offset;

	size_t layout_size;
	const struct flash_pages_layout *layout;
	const struct flash_driver_api *api = flash_dev->driver_api;

	api->page_layout(flash_dev, &layout, &layout_size);

	if (layout_size != 1) {
		LOG_ERR("Invalid flash layout");
		return -ENODEV;
	}

	pagesize = layout->pages_size;
	if ((flash_offset + 2 * pagesize) > (layout->pages_count * layout->pages_size)) {
		LOG_ERR("Invalid start address.");
		return -EINVAL;
	}

	return state_load();
}

enum mfu_state mfu_state_get(void)
{
	return state.state;
}

int mfu_state_set(enum mfu_state new_state)
{
	state.state = new_state;

	LOG_DBG("mfu_state_set: %s", mfu_state_name_get(new_state));

	return state_store();
}
