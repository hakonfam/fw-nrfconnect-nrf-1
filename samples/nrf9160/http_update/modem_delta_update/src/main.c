/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include <bsd.h>
#include <modem/bsdlib.h>
#include <modem/at_cmd.h>
#include "update.h"

#define FOTA_TEST "FOTA-TEST"

static char version[256];

static bool is_test_firmware(void)
{
	static bool version_read;
	int err;

	if (!version_read) {
		err = at_cmd_write("AT+CGMR", version, sizeof(version), NULL);
		if (err != 0) {
			printk("Unable to read modem version: %d\n", err);
			return false;
		}

		if (strstr(version, CONFIG_SUPPORTED_BASE_VERSION) == NULL) {
			printk("Unsupported base modem version: %s\n", version);
			printk("Supported base version (set in prj.conf): %s\n",
			       CONFIG_SUPPORTED_BASE_VERSION);
			return false;
		}

		version_read = true;
	}

	return strstr(version, FOTA_TEST) != NULL;
}

static const char *get_host(void)
{
	return CONFIG_DOWNLOAD_HOST;
}

static const char *get_file(void)
{
	if (is_test_firmware()) {
		return CONFIG_DOWNLOAD_FILE_FOTA_TEST_TO_BASE;
	} else {
		return CONFIG_DOWNLOAD_FILE_BASE_TO_FOTA_TEST;
	}
}

static void finished(void)
{
	/* No handling needed */
}

static bool two_leds(void)
{
	return is_test_firmware();
}

void main(void)
{
	int err;

	printk("HTTP delta modem update sample started\n");

	printk("Initializing bsdlib\n");
#if !defined(CONFIG_BSD_LIBRARY_SYS_INIT)
	err = bsdlib_init();
#else
	/* If bsdlib is initialized on post-kernel we should
	 * fetch the returned error code instead of bsdlib_init
	 */
	err = bsdlib_get_init_ret();
#endif
	switch (err) {
	case MODEM_DFU_RESULT_OK:
		printk("Modem firmware update successful!\n");
		printk("Modem will run the new firmware after reboot\n");
		k_thread_suspend(k_current_get());
		break;
	case MODEM_DFU_RESULT_UUID_ERROR:
	case MODEM_DFU_RESULT_AUTH_ERROR:
		printk("Modem firmware update failed\n");
		printk("Modem will run non-updated firmware on reboot.\n");
		break;
	case MODEM_DFU_RESULT_HARDWARE_ERROR:
	case MODEM_DFU_RESULT_INTERNAL_ERROR:
		printk("Modem firmware update failed\n");
		printk("Fatal error.\n");
		break;
	case -1:
		printk("Could not initialize bsdlib.\n");
		printk("Fatal error.\n");
		return;
	default:
		break;
	}
	printk("Initialized bsdlib\n");

	err = update_sample_init(finished, get_host, get_file, two_leds);
	if (err != 0) {
		return;
	}

	printk("Current modem firmware version: %s\n", version);

	printk("Press Button 1 for modem delta update\n");
}
