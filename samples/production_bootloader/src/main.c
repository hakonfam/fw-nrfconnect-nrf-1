/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "uart_transport.h"
#include "recovery_bootloader.h"

#include <kernel.h>
#include <zephyr.h>

extern void arm_core_mpu_disable(void);

int main(void)
{
	int err;

	printk("HEllo schmello\n");
    // Move interrupt vector table to point at R-BL IRQs.
    // Initialize serial transport for DFU protocol.
    arm_core_mpu_disable();
    err = uart_transport_init();

    while (1) {
    }

    return 0;
}

/**
 * @}
 */
