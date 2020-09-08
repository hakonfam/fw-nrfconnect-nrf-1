/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <device.h>
#include <dfu/pcd.h>
#include <logging/log.h>
#include <storage/stream_flash.h>

LOG_MODULE_REGISTER(pcd, CONFIG_PCD_LOG_LEVEL);

#ifdef CONFIG_SOC_SERIES_NRF53X

/* These must be hard coded as this code is preprocessed for both net and app
 * core.
 */
#define APP_CORE_SRAM_START 0x20000000
#define APP_CORE_SRAM_SIZE KB(512)
#define RAM_SECURE_ATTRIBUTION_REGION_SIZE 0x2000
#define PCD_CMD_ADDRESS (APP_CORE_SRAM_START \
			+ APP_CORE_SRAM_SIZE \
			- RAM_SECURE_ATTRIBUTION_REGION_SIZE)
#endif

/** Magic value written to indicate that a copy should take place. */
#define PCD_CMD_MAGIC_COPY 0xb5b4b3b6
/** Magic value written to indicate that a something failed. */
#define PCD_CMD_MAGIC_FAIL 0x25bafc15
/** Magic value written to indicate that a copy is done. */
#define PCD_CMD_MAGIC_DONE 0xf103ce5d

struct pcd_cmd {
	uint32_t magic; /* Magic value to identify this structure in memory */
	const void *data;     /* Data to copy*/
	size_t len;           /* Number of bytes to copy */
	off_t offset;         /* Offset to store the flash image in */
} __aligned(4);

static struct pcd_cmd *cmd = (struct pcd_cmd*)PCD_CMD_ADDRESS;

int pcd_cmd_write(const void *data, size_t len, off_t offset)
{
	if (data == NULL || len == 0) {
		return -EINVAL;
	}

	cmd->magic = PCD_CMD_MAGIC_COPY;
	cmd->data = data;
	cmd->len = len;
	cmd->offset = offset;

	return 0;
}

void pcd_cmd_invalidate(void)
{
	cmd->magic = PCD_CMD_MAGIC_FAIL;
}

int pcd_cmd_status_get(void)
{
	if (cmd->magic == PCD_CMD_MAGIC_COPY) {
		return 0;
	} else if (cmd->magic == PCD_CMD_MAGIC_DONE) {
		return 1;
	} else {
		return -EINVAL;
	}
}

int pcd_fw_copy(struct device *fdev)
{
	struct stream_flash_ctx stream;
	uint8_t buf[CONFIG_PCD_BUF_SIZE];
	int rc;

	rc = stream_flash_init(&stream, fdev, buf, sizeof(buf),
			       cmd->offset, 0, NULL);
	if (rc != 0) {
		LOG_ERR("stream_flash_init failed: %d", rc);
		return rc;
	}

	rc = stream_flash_buffered_write(&stream, (uint8_t *)cmd->data,
					 cmd->len, true);
	if (rc != 0) {
		LOG_ERR("stream_flash_buffered_write fail: %d", rc);
		return rc;
	}

	LOG_INF("Transfer done");

	/* Signal complete by setting magic to DONE */
	cmd->magic = PCD_CMD_MAGIC_DONE;

	return 0;

