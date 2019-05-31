/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdint.h>
#include <zephyr.h>
#include <flash.h>
#include <dfu/mcuboot.h>
#include <pm_config.h>
#include <logging/log.h>
#include <dfu_data_writer.h>

LOG_MODULE_REGISTER(dfu_data_writer, CONFIG_DFU_DATA_WRITER_LOG_LEVEL);

static u32_t	flash_address;
static struct	device *flash_dev;
static bool	is_flash_page_erased[FLASH_PAGE_MAX_CNT];

static int flash_page_erase_if_needed(u32_t address)
{
	struct flash_pages_info info;
	int err = flash_get_page_info_by_offs(flash_dev, address, &info);

	if (err != 0) {
		LOG_ERR("flash_get_page_info_by_offs error %d\n", err);
		return err;
	}
	if (!is_flash_page_erased[info.index]) {
		err = flash_write_protection_set(flash_dev, false);
		if (err != 0) {
			LOG_ERR("flash_write_protection_set error %d\n", err);
			return err;
		}
		err = flash_erase(flash_dev, info.start_offset, info.size);
		if (err != 0) {
			LOG_ERR("flash_erase error %d at address %08x\n",
				err, info.start_offset);
			return err;
		}
		is_flash_page_erased[info.index] = true;
		err = flash_write_protection_set(flash_dev, true);
		if (err != 0) {
			LOG_ERR("flash_write_protection_set error %d\n", err);
			return err;
		}
	}
	return 0;
}

int dfu_data_writer_verify_size(size_t size)
{
	if (size > PM_MCUBOOT_SECONDARY_SIZE) {
		LOG_ERR("Requested file too big to fit in flash\n");
		return -EFBIG;
	}
	return 0;
}

int dfu_data_writer_finalize(void)
{
	int last_page_in_slot_address = PM_MCUBOOT_SECONDARY_ADDRESS +
		PM_MCUBOOT_SECONDARY_SIZE - 0x4;
	int err = flash_page_erase_if_needed(last_page_in_slot_address);

	if (err != 0) {
		return err;
	}
	err = boot_request_upgrade(0);
	if (err != 0) {
		LOG_ERR("boot_request_upgrade error %d\n", err);
		return err;
	}

	return 0;
}

int dfu_data_writer_fragment(const u8_t *data, size_t len)
{
	int err;

	__ASSERT(data != NULL, "invalid data");
	__ASSERT(len != 0, "invalid len");

	if (flash_address + len < PM_MCUBOOT_SECONDARY_ADDRESS +
			PM_MCUBOOT_SECONDARY_SIZE) {
		return -ENOSPC;
	}

	err = flash_page_erase_if_needed(flash_address);
	if (err != 0) {
		return err;
	}

	err = flash_write_protection_set(flash_dev, false);
	if (err != 0) {
		LOG_ERR("flash_write_protection_set error %d\n", err);
		return err;
	}

	err = flash_write(flash_dev, flash_address, data, len);
	if (err != 0) {
		flash_write_protection_set(flash_dev, true);
		LOG_ERR("Flash write error %d at address %08x\n",
				err, flash_address);
		return err;
	}

	err = flash_write_protection_set(flash_dev, true);
	if (err != 0) {
		LOG_ERR("flash_write_protection_set error %d\n", err);
		return err;
	}

	flash_address += len;

	return 0;
}

int dfu_data_writer_init(void)
{
	flash_address = PM_MCUBOOT_SECONDARY_ADDRESS;
	flash_dev = device_get_binding(DT_FLASH_DEV_NAME);
	if (flash_dev == 0) {
		LOG_ERR("Nordic nRF flash driver was not found!\n");
		return -ENXIO;
	}

	for (int i = 0; i < FLASH_PAGE_MAX_CNT; i++) {
		is_flash_page_erased[i] = false;
	}

	return 0;
}

