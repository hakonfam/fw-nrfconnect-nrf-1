/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "uart_transport.h"
#include "recovery_bootloader.h"

#include <kernel.h>
#include <zephyr.h>
#include <nrfx_uarte.h>

#define VECTOR_TABLE_SIZE 512
#define VECTOR_TABLE_ALIGNMENT 0x100

extern void arm_core_mpu_disable(void);
extern uint32_t __VECTOR_TABLE[VECTOR_TABLE_SIZE];

/* TODO ramfunc? */
__ramfunc static void wrap(void *param)
{
	ARG_UNUSED(param);

	nrfx_uarte_0_irq_handler();
	
}

uint32_t vector_table_RAM[VECTOR_TABLE_SIZE] __aligned(VECTOR_TABLE_ALIGNMENT);

int main(void)
{
	int err;
	arm_core_mpu_disable();

	IRQ_CONNECT(UARTE0_UART0_IRQn,
			4,
			wrap,
			NULL,
			0);

	for (int i = 0; i < VECTOR_TABLE_SIZE; i++) {
		vector_table_RAM[i] = *(uint8_t *)i;
	}

	// vector_table_RAM[UARTE0_UART0_IRQn + 16] = (uint32_t)wrap;

	__disable_irq();
	SCB->VTOR = (uint32_t)&vector_table_RAM;
	__DSB();
	__enable_irq();

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
