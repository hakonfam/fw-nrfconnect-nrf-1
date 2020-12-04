/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <zephyr.h>
#include <dfu/mcuboot.h>
#include <modem/bsdlib.h>
#include <stdio.h>
#include "update.h"

static const char *get_host(void)
{
	return CONFIG_DOWNLOAD_HOST;
}

static const char *get_file(void)
{
#if CONFIG_APPLICATION_VERSION == 2
	return CONFIG_DOWNLOAD_FILE_V1;
#else
	return CONFIG_DOWNLOAD_FILE_V2;
#endif
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

	err = bsdlib_init();
	if (err) {
		printk("Failed to initialize bsdlib!");
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
