/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/printk.h>
#include <zephyr/linker/linker-defs.h>

#include <sdfw/sdfw_services/echo_service.h>

static void echo_request(void)
{
	int err;
	char req_str[] = "wat";
	char rsp_str[sizeof(req_str) + 1];

	printk("Calling ssf_echo, str: \"%s\"", req_str);

	err = ssf_echo(req_str, rsp_str, sizeof(rsp_str));
	if (err != 0) {
		printk("ssf_echo failed");
	} else {
		printk("ssf_echo response: %s", rsp_str);
	}
}

int main(void)
{
	printk("RAM loader running from %p\n", __rom_region_start);
	//echo_request();

	return 0;
}
