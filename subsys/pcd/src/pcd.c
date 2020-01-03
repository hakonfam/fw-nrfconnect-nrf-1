/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <drivers/flash.h>
#include <device.h>
#include <pcd.h>
#include <tinycrypt/sha256.h>
#include <tinycrypt/constants.h>
#include <logging/log.h>
#include <dfu_target_flash.h>

LOG_MODULE_REGISTER(pcd, CONFIG_PCD_LOG_LEVEL);

static u8_t buf[4096];
struct tc_sha256_state_struct hash;

static int data_written_callback(u8_t *buf, size_t len, size_t offset)
{
	int rc;

	rc = tc_sha256_update(&hash, buf, len);
	if (rc != TC_CRYPTO_SUCCESS) {
		LOG_ERR("sha256 fail: %d", rc);
		return rc;
	}

	return 0;
}

int pcd_transfer_and_hash(struct pcd_cmd *config, const char *dev_name)
{
	int rc;

	rc = dfu_target_flash_cfg(dev_name, config->dst_addr, 0, buf,
				  sizeof(buf));
	if (rc != 0) {
		LOG_ERR("dfu_target_flash failed: %d", rc);
		return rc;
	}

	(void)dfu_target_flash_set_callback(data_written_callback);

	rc = tc_sha256_init(&hash);
	if (rc != TC_CRYPTO_SUCCESS) {
		LOG_ERR("tc_sha256_init fail: %d", rc);
		return rc;
	}

	rc = dfu_target_flash_write(config->src, config->src_len);
	if (rc != 0) {
		LOG_ERR("dfu_target_flash_write fail: %d", rc);
		return rc;
	}

	rc = dfu_target_flash_done(true);
	if (rc != 0) {
		LOG_ERR("dfu_target_flash_done fail: %d", rc);
		return rc;
	}

	rc = tc_sha256_final(config->hash, &hash);
	if (rc != TC_CRYPTO_SUCCESS) {
		LOG_ERR("sha256_final fail: %d", rc);
		return rc;
	}

	return 0;
}
