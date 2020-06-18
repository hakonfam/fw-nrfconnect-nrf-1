/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "uart_transport.h"
#include "recovery_bootloader.h"


int main(void)
{
	int err;

	printk("HEllo schmello\n");
    // Move interrupt vector table to point at R-BL IRQs.
    // Initialize serial transport for DFU protocol.
    err = uart_transport_init();

    while (1) {
    }

    return 0;
}

/**
 * @}
 */
