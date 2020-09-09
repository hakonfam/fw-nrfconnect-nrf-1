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
#include <dfu_target_stream.h>

LOG_MODULE_REGISTER(dfu_target_mcuboot, CONFIG_DFU_TARGET_LOG_LEVEL);

#define MAX_FILE_SEARCH_LEN 500
#define MCUBOOT_HEADER_MAGIC 0x96f3b83d
#define MCUBOOT_SECONDARY_LAST_PAGE_ADDR \
	(PM_MCUBOOT_SECONDARY_ADDRESS + PM_MCUBOOT_SECONDARY_SIZE - 1)


static uint8_t *stream_buf = NULL;
static size_t stream_buf_len;
static const char *TARGET_MCUBOOT = "MCUBOOT";


bool dfu_target_mcuboot_identify(const void *const buf)
{
	/* MCUBoot headers starts with 4 byte magic word */
	return *((const u32_t *)buf) == MCUBOOT_HEADER_MAGIC;
}

int dfu_target_mcuboot_set_buf(uint8_t *buf, size_t len)
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
	if (flash_dev == NULL) {
		LOG_ERR("Failed to get device");
		return -EFAULT;
	}

	err = dfu_target_stream_init(TARGET_MCUBOOT, flash_dev,
				     stream_buf, stream_buf_len,
				     PM_MCUBOOT_SECONDARY_ADDRESS,
				     PM_MCUBOOT_SECONDARY_SIZE, NULL);
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
		err = stream_flash_erase_page(dfu_target_stream_get_stream(),
				MCUBOOT_SECONDARY_LAST_PAGE_ADDR);
		if (err != 0) {
			LOG_ERR("Unable to delete last page: %d", err);
			return err;
		}
		err = boot_request_upgrade(BOOT_UPGRADE_TEST);
		if (err != 0) {
			LOG_ERR("boot_request_upgrade error %d", err);
			return err;
		}
		LOG_INF("MCUBoot image upgrade scheduled. Reset the device to "
			"apply");
	} else {
		LOG_INF("MCUBoot image upgrade aborted.");
	}

	return err;
}
