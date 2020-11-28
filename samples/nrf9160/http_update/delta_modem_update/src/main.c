/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <zephyr.h>
#include <modem/libmodem.h>
#include <stdio.h>

void main(void)
{
	int err;

	/* TODO check the modem version and fail if it is not
	 * exactly what we expect it to be
	 */

	printk("HTTP delta modem update sample started\n");

	printk("Initializing modem library\n");
#if !defined(CONFIG_LIBMODEM_SYS_INIT)
	err = libmodem_init();
#else
	/* If libmodem is initialized on post-kernel we should
	 * fetch the returned error code instead of libmodem_init
	 */
	err = libmodem_get_init_ret();
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
		printk("Could not initialize momdem library.\n");
		printk("Fatal error.\n");
		return;
	default:
		break;
	}

	printk("Initialized modem library\n");

	err = update_sample_init(finished, get_host, get_file, two_leds);
	if (err != 0) {
		return;
	}


}
