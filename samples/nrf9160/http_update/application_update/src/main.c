/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <zephyr.h>
#include <nrf_modem.h>
#include <dfu/dfu_target_mcuboot.h>
#include <dfu/mcuboot.h>
#include <stdio.h>
#include "update.h"

static uint8_t fota_buf[256];

static const char * get_host(void)
{
	return CONFIG_DOWNLOAD_HOST;
}

static const char * get_file(void)
{
	return CONFIG_DOWNLOAD_FILE;
}

static void finished(void)
{
	/* No handling needed */
}


static bool two_leds(void)
{
#if CONFIG_APPLICATION_VERSION == 2
	return true;
#else
	return false;
#endif
}

void main(void)
{
	int err;

	printk("HTTP application update sample started\n");

	err = dfu_target_mcuboot_set_buf(fota_buf, sizeof(fota_buf));
	if (err != 0) {
		printk("dfu_target_mcuboot_set_buf() failed, err %d\n", err);
		return;
	}

	/* This is needed so that MCUBoot won't revert the update */
	boot_write_img_confirmed();

	err = update_sample_init(finished, get_host, get_file, two_leds);
	if (err != 0) {
		return;
	}

	printk("Press Button 1 for application firmware update\n");
}
