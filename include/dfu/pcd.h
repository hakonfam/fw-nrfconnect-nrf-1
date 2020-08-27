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


/** @brief Opaque type */
struct pcd_cmd;

/** @brief Get a PCD CMD from the specified address.
 *
 * @param cmd  Pointer to where the output cmd pointer is stored
 * @param addr The address to check for a valid PCD CMD.
 *
 * @retval non-negative integer on success, negative errno code on failure.
 */
int pcd_cmd_read(struct pcd_cmd **cmd, off_t from);

/** @brief Construct a PCD CMD for copying data/firmware.
 *
 * @param cmd    Pointer to where the output cmd pointer is stored
 * @param dest   The address to write the CMD to.
 * @param data   The data to copy.
 * @param len    The number of bytes that should be copied.
 * @param offset The offset within the flash device to write the data to.
 *               For internal flash, the offset is the same as the address.
 *
 * @retval non-negative integer on success, negative errno code on failure.
 */
int pcd_cmd_write(struct pcd_cmd **cmd, off_t dest, const void *data,
		  size_t len, off_t offset);

/** @brief Invalidate the PCD CMD, indicating that the copy failed.
 *
 * @param cmd The PCD CMD to invalidate.
 *
 * @retval non-negative integer on success, negative errno code on failure.
 */
int pcd_cmd_invalidate(struct pcd_cmd *cmd);

/** @brief Check the PCD CMD to find the status of the update.
 *
 * @param cmd The PCD CMD to check
 *
 * @retval 0 if operation is not complete, positive integer if operation is
 *           complete, negative integer on failure.
 */
int pcd_cmd_status_get(const struct pcd_cmd *cmd);

/** @brief Perform the DFU image transfer.
 *
 * Use the information in the provided PCD CMD to load a DFU image to the
 * provided flash device.
 *
 * @param cmd The PCD CMD whose configuration will be used for the transfer.
 * @param fdev The flash device to transfer the DFU image to.
 *
 * @retval non-negative integer on success, negative errno code on failure.
 */
int pcd_fw_copy(struct pcd_cmd *cmd, struct device *fdev);

#endif /* PCD_H__ */

/**@} */
