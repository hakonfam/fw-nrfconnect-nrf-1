/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <drivers/flash.h>
#include <logging/log.h>
#include <dfu/dfu_target.h>
#include <dfu/dfu_target_stream.h>

LOG_MODULE_REGISTER(dfu_target_modem_full, CONFIG_DFU_TARGET_LOG_LEVEL);

#define MODEM_FULL_HEADER_MAGIC 0x7fdeb13c

static struct stream_flash_ctx stream;
static uint8_t *stream_buf;
static size_t stream_buf_len;
static off_t fdev_offset;
static size_t fdev_size;
static struct device *fdev;
static struct dfu_target_stream_ctx stream_modem_full;


bool dfu_target_modem_full_identify(const void *const buf)
{
	/* Flash dev headers starts with 4 byte magic word */
	return *((const uint32_t *)buf) == MODEM_FULL_HEADER_MAGIC;
}

int dfu_target_modem_full_cfg(uint8_t *buf, size_t len, struct device *dev,
			      off_t dev_offset, size_t dev_size)
{
	if (buf == NULL) {
		return -EINVAL;
	}

	fdev = dev;
	fdev_offset = offset;
	fdev_size = size;
	stream_buf = buf;
	stream_buf_len = len;

	return 0;
}

int dfu_target_modem_full_init(size_t file_size)
{
	/* TODO add more checking here */
	if (stream_buf == NULL) {
		LOG_ERR("Missing stream_buf, call '..set_buf' before '..init");
		return -ENODEV;
	}

	flash_dev = device_get_binding(PM_MCUBOOT_SECONDARY_DEV_NAME);

	err = stream_flash_init(&stream, fdev, stream_buf, stream_buf_len,
				fdev_offset, fdev_size, NULL);
	if (err < 0) {
		LOG_ERR("stream_flash_init failed %d", err);
		return err;
	}

	/* TODO check file size? */
	return dfu_target_stream_init(&stream_modem_full, &stream,
				      DFU_TARGET_STREAM_FLASH);
}

int dfu_target_modem_full_offset_get(size_t *out)
{
	return dfu_target_stream_offset_get(out);
}

int dfu_target_modem_full_write(const void *const buf, size_t len)
{
	return dfu_target_stream_write(buf, len);
}

int dfu_target_modem_full_done(bool successful)
{
	return dfu_target_stream_done(successful);
}
