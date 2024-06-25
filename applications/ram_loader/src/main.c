/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/printk.h>
#include <zephyr/linker/linker-defs.h>

int main(void)
{
	printk("RAM loader running from %p\n", __rom_region_start);

	return 0;
}
