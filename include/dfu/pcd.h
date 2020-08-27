/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file pcd.h
 *
 * @defgroup pcd Peripheral Core DFU
 * @{
 * @brief API for handling DFU of peripheral cores.
 *
 * The PCD API provides functions for transferring DFU images from a generic
 * core to a peripheral core to which there is no flash access from the generic
 * core.
 *
 * The cores communicate through a command structure (CMD) which is stored in
 * a shared memory location.
 *
 * The nRF5340 is an example of a system with these properties.
 */

#ifndef PCD_H__
#define PCD_H__

#include <device.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_SOC_SERIES_NRF53X

/* These must be hard coded as this code is preprocessed for both net and app
 * core.
 */
#define APP_CORE_SRAM_START 0x20000000
#define APP_CORE_SRAM_SIZE KB(512)
#define RAM_SECURE_ATTRIBUTION_REGION_SIZE 0x2000
#define PCD_CMD_ADDRESS (APP_CORE_SRAM_START \
			+ APP_CORE_SRAM_SIZE \
			- RAM_SECURE_ATTRIBUTION_REGION_SIZE)
#endif

enum pcd_status {
	COPY = 0,
	COPY_DONE = 1
};

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
int pcd_network_core_update(void *src_addr, size_t len);

/** @brief Lock the RAM section used for IPC with the network core bootloader.
 */
void pcd_lock_ram(void);


/** @brief Invalidate the PCD CMD, indicating that the copy failed.
 */
void pcd_fw_copy_invalidate(void);

/** @brief Check the PCD CMD to find the status of the update.
 *
 * @retval 0 if operation is not complete, positive integer if operation is
 *           complete, negative integer on failure.
 */
enum pcd_status pcd_fw_copy_status_get(void);

/** @brief Perform the DFU image transfer.
 *
 * Use the information in the provided PCD CMD to load a DFU image to the
 * provided flash device.
 *
 * @param fdev The flash device to transfer the DFU image to.
 *
 * @retval non-negative integer on success, negative errno code on failure.
 */
int pcd_fw_copy(struct device *fdev);

#endif /* PCD_H__ */

/**@} */
