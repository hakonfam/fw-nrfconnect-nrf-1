/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <device.h>
#include <dfu/pcd.h>
#include <tinycrypt/sha256.h>
#include <tinycrypt/constants.h>
#include <logging/log.h>
#include <storage/fbw.h>

LOG_MODULE_REGISTER(pcd, CONFIG_PCD_LOG_LEVEL);

static struct tc_sha256_state_struct hash;

static int data_written_cb(u8_t *buf, size_t len, size_t offset)
{
	int rc;

	rc = tc_sha256_update(&hash, buf, len);
	if (rc != TC_CRYPTO_SUCCESS) {
		LOG_ERR("sha256 fail: %d", rc);
		return rc;
	}

	return 0;
}

struct pcd_cmd *pcd_get_cmd(void *addr)
{
	struct pcd_cmd *cmd = (struct pcd_cmd *)addr;

	if (cmd->magic != PCD_CMD_MAGIC) {
		return NULL;
	}

	return cmd;
}

int pcd_invalidate(struct pcd_cmd *cmd)
{
	/* TODO add null checks here and elsewhere */
	memset(cmd->hash, 0, sizeof(cmd->hash));
	return 0;
}

int pcd_transfer_and_hash(struct pcd_cmd *cmd, struct device * fdev)
{
	struct fbw_ctx fbw;
	u8_t buf[CONFIG_PCD_BUF_SIZE];
	int rc;

	rc = fbw_init(&fbw, fdev, buf, sizeof(buf), cmd->offset, 0,
		      data_written_cb);
	if (rc != 0) {
		LOG_ERR("fbw_init failed: %d", rc);
		return rc;
	}

	rc = tc_sha256_init(&hash);
	if (rc != TC_CRYPTO_SUCCESS) {
		LOG_ERR("tc_sha256_init fail: %d", rc);
		return rc;
	}

	rc = fbw_write(&fbw, (u8_t *)cmd->src, cmd->len, true);
	if (rc != 0) {
		LOG_ERR("fbw_write fail: %d", rc);
		return rc;
	}

	rc = tc_sha256_final(cmd->hash, &hash);
	if (rc != TC_CRYPTO_SUCCESS) {
		LOG_ERR("sha256_final fail: %d", rc);
		return rc;
	}

	/* Signal complete by setting magic to 0 */
	cmd->magic = 0;

	return 0;
}
