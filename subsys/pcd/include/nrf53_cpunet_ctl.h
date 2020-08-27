/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRF53_CPUNET_CTL_H__
#define NRF53_CPUNET_CTL_H__

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Sets up the PCD command structure with the location and size of the
 *	   firmware update. Then boots the network core and checks if the
 *	   update completed successfully.
 *
 * @param src_addr Start address of the data which is to be copied into the
 *                 network core.
 * @param len Length of the data which is to be copied into the network core.
 *
 * @retval 0 on success, -1 on failure.
 */
int start_network_core_update(void *src_addr, size_t len);

/** @brief Lock the RAM section used for IPC with the network core bootloader.
 */
void lock_ipc_ram_with_spu(void);

#ifdef __cplusplus
}
#endif

#endif /* NRF53_CPUNET_CTL_H__ */
