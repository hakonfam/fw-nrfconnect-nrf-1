/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <hal/nrf_reset.h>
#include <hal/nrf_spu.h>
#include <dfu/pcd.h>
#include <pm_config.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(mcuboot);

#define NET_CORE_APP_OFFSET PM_CPUNET_B0N_SIZE

int start_network_core_update(void *src_addr, size_t len)
{
	int err;

	/* Retain nRF5340 Network MCU in Secure domain (bus
	 * accesses by Network MCU will have Secure attribute set).
	 * This is needed for the network core to be able to read the
	 * shared RAM area used for IPC.
	 */
	nrf_spu_extdomain_set(NRF_SPU, 0, true, false);

	/* Ensure that the network core is turned off */
	nrf_reset_network_force_off(NRF_RESET, true);

	LOG_INF("Writing cmd to addr: 0x%x", PCD_CMD_ADDRESS);
	err = pcd_cmd_write(src_addr, len, NET_CORE_APP_OFFSET);
	if (cmd == NULL) {
		LOG_INF("Error while writing PCD cmd");
		return -1;
	}

	nrf_reset_network_force_off(NRF_RESET, false);
	LOG_INF("Turned on network core");

	do {
		err = pcd_cmd_status_get();
	} while (err == 0);

	if (err < 0) {
		LOG_ERR("Network core update failed");
		return err;
	}

	nrf_reset_network_force_off(NRF_RESET, true);
	LOG_INF("Turned off network core");

	return 0;
}

void lock_ipc_ram_with_spu(void)
{
	uint32_t region = (PCD_CMD_ADDRESS/CONFIG_NRF_SPU_RAM_REGION_SIZE) + 1;

	nrf_spu_ramregion_set(NRF_SPU, region, true, NRF_SPU_MEM_PERM_READ,
			true);
}
