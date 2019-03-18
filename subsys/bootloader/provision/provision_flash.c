/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "provision.h"
#include <pm_config.h>
#include <string.h>
#include <stdbool.h>
#include <generated_dts_board.h>
#include <errno.h>
#include <nrf.h>
#include <assert.h>

typedef struct {
	u32_t s0_address;
	u32_t s1_address;
	u32_t num_public_keys;
	u8_t pkd[1];
} provision_flash_t;

static const provision_flash_t *p_provision_data =
	(provision_flash_t *)PM_CFG_PROVISION_ADDRESS;

u32_t s0_address_read(void)
{
	return p_provision_data->s0_address;
}

u32_t s1_address_read(void)
{
	return p_provision_data->s1_address;
}

u32_t num_public_keys_read(void)
{
	return p_provision_data->num_public_keys;
}

int public_key_data_read(u32_t key_idx, u8_t *p_buf, size_t buf_size)
{
	const u8_t *p_key;

	if (buf_size < CONFIG_SB_PUBLIC_KEY_HASH_LEN) {
		return -ENOMEM;
	}

	if (key_idx >= p_provision_data->num_public_keys) {
		return -EINVAL;
	}

	p_key = &p_provision_data->pkd[key_idx * CONFIG_SB_PUBLIC_KEY_HASH_LEN];

	/*
	 * Ensure word alignment, as provision data might be stored in area
	 * with word sized read limitation.
	 */
	__ASSERT(!(p_key & 3), "Key address is not multiple of 4");
	memcpy(p_buf, p_key, CONFIG_SB_PUBLIC_KEY_HASH_LEN);

	return CONFIG_SB_PUBLIC_KEY_HASH_LEN;
}
