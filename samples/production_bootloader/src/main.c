/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "uart_transport.h"
#include "recovery_bootloader.h"


int main(void)
{
    // Move interrupt vector table to point at R-BL IRQs.
    SCB->VTOR = BOOTLOADER_START_ADDRESS;

    // Initialize serial transport for DFU protocol.
    ret_code = uart_transport_init();
}

/**
 * @}
 */
