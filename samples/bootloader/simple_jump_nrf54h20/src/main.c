/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arm/arch.h>

/* Target image address - where the main application is located */
#define TARGET_IMAGE_ADDRESS 0x0e102000

struct arm_vector_table {
	uint32_t msp;	       /* Initial stack pointer */
	uint32_t reset_vector; /* Reset handler address */
};

/**
 * @brief Jump to another image at specified address
 *
 * This function performs a clean jump to another firmware image by:
 * 1. Disabling all interrupts
 * 2. Clearing NVIC pending interrupts
 * 3. Setting the vector table offset register (VTOR)
 * 4. Loading the new stack pointer and reset vector
 * 5. Jumping to the new image
 *
 * @param image_addr Address of the target image
 */
static void __attribute__((noreturn)) jump_to_image(uint32_t image_addr)
{
	struct arm_vector_table *vt = (struct arm_vector_table *)image_addr;

	printk("Jumping to image at address 0x%08x\n", image_addr);
	printk("Stack pointer: 0x%08x\n", vt->msp);
	printk("Reset vector: 0x%08x\n", vt->reset_vector);

	/* Disable all interrupts */
	__disable_irq();

	/* Disable SysTick */
	SysTick->CTRL = 0;

	/* Disable and clear all NVIC interrupts */
	NVIC_Type *nvic = NVIC;
	for (uint8_t i = 0; i < ARRAY_SIZE(nvic->ICER); i++) {
		nvic->ICER[i] = 0xFFFFFFFF;
	}
	for (uint8_t i = 0; i < ARRAY_SIZE(nvic->ICPR); i++) {
		nvic->ICPR[i] = 0xFFFFFFFF;
	}

	/* Clear pending system exceptions */
	SCB->ICSR |= SCB_ICSR_PENDSTCLR_Msk;

#ifndef CONFIG_CPU_CORTEX_M0
	/* Disable fault handlers */
	SCB->SHCSR &= ~(SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk |
			SCB_SHCSR_MEMFAULTENA_Msk);
#endif

	/* Switch to Main Stack Pointer if currently using Process Stack Pointer */
	if (CONTROL_SPSEL_Msk & __get_CONTROL()) {
		__set_CONTROL(__get_CONTROL() & ~CONTROL_SPSEL_Msk);
	}

#if defined(CONFIG_BUILTIN_STACK_GUARD) && defined(CONFIG_CPU_CORTEX_M_HAS_SPLIM)
	/* Reset stack limit registers */
	__set_PSPLIM(0);
	__set_MSPLIM(0);
#endif

	/* Memory barrier to ensure all writes complete */
	__DSB();
	__ISB();

	/* Set new vector table offset register */
	SCB->VTOR = image_addr;

	/* Set new stack pointer */
	__set_MSP(vt->msp);
	__set_PSP(0);

	/* Memory barriers before jump */
	__DSB();
	__ISB();

	/* Jump to the new image reset vector */
	((void (*)(void))vt->reset_vector)();

	/* Should never reach here */
	CODE_UNREACHABLE;
}

/**
 * @brief Simple validation of the target image
 *
 * Performs basic checks to ensure the target address contains
 * what appears to be a valid ARM vector table.
 *
 * @param image_addr Address to validate
 * @return true if basic validation passes, false otherwise
 */
static bool validate_image(uint32_t image_addr)
{
	struct arm_vector_table *vt = (struct arm_vector_table *)image_addr;

	/* Check if the stack pointer looks reasonable (should be in RAM) */
	if (vt->msp < 0x20000000 || vt->msp > 0x30000000) {
		printk("Invalid stack pointer: 0x%08x\n", vt->msp);
		return false;
	}

	/* Check if reset vector is in flash and has thumb bit set */
	if ((vt->reset_vector & 0x1) == 0) {
		printk("Reset vector missing thumb bit: 0x%08x\n", vt->reset_vector);
		return false;
	}

	/* Check if reset vector points to reasonable flash address */
	uint32_t reset_addr = vt->reset_vector & 0xFFFFFFFE;
	if (reset_addr < 0x00000000 || reset_addr > 0x10000000) {
		printk("Reset vector out of flash range: 0x%08x\n", reset_addr);
		return false;
	}

	return true;
}

int main(void)
{
	printk("\n=== Simple Jump Bootloader for nRF54H20 ===\n");
	printk("Bootloader started\n");
	printk("Target image address: 0x%08x\n", TARGET_IMAGE_ADDRESS);

	/* Give some time for console output to be transmitted */
	k_msleep(100);

	/* Validate the target image */
	if (!validate_image(TARGET_IMAGE_ADDRESS)) {
		printk("ERROR: Target image validation failed!\n");
		printk("Bootloader will not jump to invalid image.\n");
		while (1) {
			k_msleep(1000);
			printk("Waiting...\n");
		}
	}

	printk("Target image validation passed\n");
	printk("Preparing to jump...\n");

	/* Small delay to ensure console output */
	k_msleep(100);

	/* Jump to the target image - this function does not return */
	jump_to_image(TARGET_IMAGE_ADDRESS);

	/* Should never reach here */
	return 0;
}
