/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <pm_config.h>
#include <logging/log.h>
#include <dfu/mcuboot.h>
#include <dfu/dfu_target.h>
#include <dfu/dfu_target_stream.h>

LOG_MODULE_REGISTER(dfu_target_mcuboot, CONFIG_DFU_TARGET_LOG_LEVEL);

#define MAX_FILE_SEARCH_LEN 500
#define MCUBOOT_HEADER_MAGIC 0x96f3b83d

static struct stream_flash_ctx stream;
static u8_t *stream_buf;
static size_t stream_buf_len;
static struct dfu_target_stream_ctx stream_dfu;

bool dfu_target_mcuboot_identify(const void *const buf)
{
	/* MCUBoot headers starts with 4 byte magic word */
	return *((const u32_t *)buf) == MCUBOOT_HEADER_MAGIC;
}

int dfu_target_mcuboot_set_buf(u8_t *buf, len)
{
	if (buf == NULL) {
		return -EINVAL;
	}

	stream_buf = buf;
	stream_buf_len = len;

	return 0;
}

int dfu_target_mcuboot_init(size_t file_size, dfu_target_callback_t cb)
{
	ARG_UNUSED(cb);
	struct device *flash_dev;
	int err;

	if (stream_buf == NULL) {
		LOG_ERR("Missing stream_buf, call '..set_buf' before '..init");
		return -ENODEV;
	}

	if (file_size > PM_MCUBOOT_SECONDARY_SIZE) {
		LOG_ERR("Requested file too big to fit in flash %zu > 0x%x",
			file_size, PM_MCUBOOT_SECONDARY_SIZE);
		return -EFBIG;
	}

	flash_dev = device_get_binding(PM_MCUBOOT_SECONDARY_DEV_NAME);

	err = stream_flash_init(&stream, flash_dev, stream_buf, stream_buf_len,
				0, 0, NULL);
	if (err < 0) {
		LOG_ERR("stream_flash_init failed %d", err);
		return err;
	}

	err = dfu_target_stream_init(&stream_dfu, &stream,
				     DFU_TARGET_STERAM_MCUBOOT);
	if (err < 0) {
		LOG_ERR("dfu_target_stream_init failed %d", err);
		return err;
	}

	return 0;
}

int dfu_target_mcuboot_offset_get(size_t *out)
{
	return dfu_target_stream_offset_get(out);
}

int dfu_target_mcuboot_write(const void *const buf, size_t len)
{
	return dfu_target_stream_write(buf, len);
}

int dfu_target_mcuboot_done(bool successful)
{
	int err = 0;

	err = dfu_target_stream_done(successful);
	if (err != 0) {
		LOG_ERR("dfu_target_stream_done error %d", err);
		return err;
	}

	if (successful) {
		err = boot_request_upgrade(BOOT_UPGRADE_TEST);
		if (err != 0) {
			LOG_ERR("boot_request_upgrade error %d", err);
			reset_flash_context();
			return err;
		}
		LOG_INF("MCUBoot image upgrade scheduled. Reset the device to "
			"apply");
	} else {
		LOG_INF("MCUBoot image upgrade aborted.");
	}

	return err;
}
