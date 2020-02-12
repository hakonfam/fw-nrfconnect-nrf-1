/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <flash_img_raw.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(dfu_target_flash, CONFIG_DFU_TARGET_LOG_LEVEL);

#define FLASH_HEADER_MAGIC 0x7fdeb13c

static bool initialized;
static size_t available;

int dfu_target_flash_cfg(const char *dev_name, size_t start, size_t end,
			 void *buf, size_t buf_len)
{
	initialized = true;
	return flash_img_raw_cfg(dev_name, start, end, buf, buf_len,
				 &available);
}


int dfu_target_flash_init(size_t file_size)
{
	if (!initialized) {
		LOG_ERR("Flash dev not initialized");
		return -EFAULT;
	}

	if (file_size > available_size) {
		LOG_ERR("Requested file too big to fit in flash");
		return -EFBIG;
	}

	return 0;
}

bool dfu_target_flash_identify(const void *const buf)
{
	/* Flash dev headers starts with 4 byte magic word */
	return *((const u32_t *)buf) == FLASH_HEADER_MAGIC;
}

int dfu_target_flash_offset_get(size_t *out)
{
	return flash_img_raw_offset_get(out);
}

int dfu_target_flash_write(const void *const buf, size_t len)
{
	return flash_img_raw_write(buf, len);
}

int dfu_target_flash_done(bool successful)
{
	if (successful) {
		return flash_img_raw_done();
	}

	return 0;
}
