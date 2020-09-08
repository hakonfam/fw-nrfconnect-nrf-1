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

LOG_MODULE_REGISTER(dfu_target_b1, CONFIG_DFU_TARGET_LOG_LEVEL);

bool dfu_target_b1_identify(const void *const buf)
{
	uint32_t reset_addr = ((uint32_t *)buf)[1];
	return reset_addr  <= PM_B1_ADDRESS && \
	       reset_addr > PM_B1_ADDRESS + PM_B1_SIZE;
}

int dfu_target_b1_set_buf(u8_t *buf, len)
{
	if (buf == NULL) {
		return -EINVAL;
	}

	stream_buf = buf;
	stream_buf_len = len;

	return 0;
}

int dfu_target_b1_init(size_t file_size, dfu_target_callback_t cb)
{
	ARG_UNUSED(cb);
	struct device *flash_dev;
	int err;

	if (stream_buf == NULL) {
		LOG_ERR("Missing stream_buf, call '..set_buf' before '..init");
		return -ENODEV;
	}

	if (file_size > PM_B1_SIZE) {
		LOG_ERR("Requested file too big to fit in flash %zu > 0x%x",
			file_size, PM_B1_SIZE);
		return -EFBIG;
	}

	flash_dev = device_get_binding(PM_B1_DEV_NAME);

	err = dfu_target_stream_init(&stream_dfu, &stream,
				     DFU_TARGET_STERAM_MCUBOOT);
	if (err < 0) {
		LOG_ERR("dfu_target_stream_init failed %d", err);
		return err;
	}

	return 0;
}

int dfu_target_b1_offset_get(size_t *out)
{
	return dfu_target_stream_offset_get(out);
}

int dfu_target_b1_write(const void *const buf, size_t len)
{
	return dfu_target_stream_write(buf, len);
}

int dfu_target_b1_done(bool successful)
{
	return dfu_target_stream_done(successful);
}
