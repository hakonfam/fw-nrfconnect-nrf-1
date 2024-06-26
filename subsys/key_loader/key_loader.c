/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/sys/printk.h>

static const unsigned char application_gen0[] = {
#include CONFIG_NRF_KEY_LOADER_MANIFEST_PUBKEY_APPLICATION_GEN0_NAME
};

static int log_keys(void)
{
	for(int i = 0; i < sizeof(application_gen0); i++) {
		printk("%x", application_gen0[i]);
	}
	printk("\n");

	return 0;
}

SYS_INIT(log_keys, APPLICATION, 99);
