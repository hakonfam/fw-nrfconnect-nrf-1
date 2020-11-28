/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <zephyr.h>
#include <modem/at_cmd.h>
#include <dfu/dfu_target_modem_full.h>
#include <nrf_fmfu.h>
#include <dfu/fmfu_fdev.h>
#include <stdio.h>

static char modem_version[256];
static uint8_t fota_buf[NRF_FMFU_MODEM_BUFFER_SIZE];

#define EXT_FLASH_DEVICE DT_LABEL(DT_INST(0, jedec_spi_nor))

static void print_modem_version(void)
{
	int err = at_cmd_write("AT+CGMR", modem_version, sizeof(modem_version),
				NULL);

	__ASSERT(err == 0, "Failed reading modem version");

	printk("Current modem firmware version: %s", modem_version);
}

static void finished_cb(void)
{
	int err;
	const struct device *flash_dev = device_get_binding(EXT_FLASH_DEVICE);

	err = libmodem_shutdown();
	if (err != 0) {
		printk("libmodem_shutdown() failed: %d\n", err);
		return;
	}

	err = fmfu_fdev_load(fota_buf, sizeof(fota_buf), flash_dev, 0);
	if (err != 0) {
		printk("fmfu_fdev_load failed: %d\n", err);
		return;
	}

	err = libmodem_init();
	if (err != 0) {
		printk("libmodem_init() failed: %d\n", err);
		return;
	}

	printk("Modem firmware update completed");

	print_modem_version();

}

static const char * get_host(void)
{
	return CONFIG_DOWNLOAD_MODEM_HOST;
}

static const char * get_file(void)
{
	const char *file = CONFIG_DOWNLOAD_MODEM_0_FILE;

	/* Check if we should download modem 0 or 1 */
	if (strncmp(modem_version, CONFIG_DOWNLOAD_MODEM_0_VERSION,
		    strlen(CONFIG_DOWNLOAD_MODEM_0_VERSION)) == 0) {
		file = CONFIG_DOWNLOAD_MODEM_1_FILE;
	}

	return file;
}

/**@brief Turn on LED0 and LED1 if CONFIG_APPLICATION_VERSION
 * is 2 and LED0 otherwise.
 */
static bool two_leds(void)
{
	/* TODO check modem version and return TRUE if 1.2.2 is installed */
	return 0;
}


void main(void)
{
	int err;

	printk("HTTP full modem update sample started\n");

	/* Pass 0 as size since that tells the stream_flash module
	 * that we want to use all the available flash in the device
	 */
	err = dfu_target_modem_full_cfg(fota_buf, sizeof(fota_buf), flash_dev,
					0, 0);
	if (err != 0) {
		printk("dfu_target_modem_full_cfg failed: %d\n", err);
		return;
	}

	/* START debug stuff */
	apply_fmfu();

	/* END debug stuff */

	err = update_sample_init(finished, get_host, get_file, two_leds);
	if (err != 0) {
		return;
	}

	print_modem_version();

	printk("Press Button 1 for full modem firmware update (fmfu)\n");
}
