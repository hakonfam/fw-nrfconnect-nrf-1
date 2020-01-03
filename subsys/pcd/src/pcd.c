/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <pcd.h>
#include <tinycrypt/sha256.h>
#include <tinycrypt/constants.h>
#include <logging/log.h>
#include <dfu/flash_img.h>

LOG_MODULE_REGISTER(pcd, CONFIG_PCD_LOG_LEVEL);

static struct tc_sha256_state_struct hash;
static struct flash_img_context flash_img;

int pcd_transfer_and_hash(struct pcd_cmd *cfg)
{
	int rc;
	size_t base;

	rc = flash_img_init_id(&flash_img, cfg->area_id);
	if (rc != 0) {
		LOG_ERR("flash_img_init failed: %d", rc);
		return rc;
	}

	rc = tc_sha256_init(&hash);
	if (rc != TC_CRYPTO_SUCCESS) {
		LOG_ERR("tc_sha256_init fail: %d", rc);
		return rc;
	}

	/* Copy this out now since the flash area is closed when the write
	 * operation finishes.
	 */
	base = flash_img.flash_area->fa_off;

	rc = flash_img_buffered_write(&flash_img, cfg->src, cfg->src_len,
				      true);
	if (rc != 0) {
		LOG_ERR("flash_img_buffered_write fail: %d", rc);
		return rc;
	}

	rc = tc_sha256_update(&hash, (u8_t *)base, cfg->src_len);
	if (rc != TC_CRYPTO_SUCCESS) {
		LOG_ERR("sha256 fail: %d", rc);
		return rc;
	}

	rc = tc_sha256_final(cfg->hash, &hash);
	if (rc != TC_CRYPTO_SUCCESS) {
		LOG_ERR("sha256_final fail: %d", rc);
		return rc;
	}

	return 0;
}
