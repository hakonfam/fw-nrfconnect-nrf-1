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

/** @brief Construct a PCD CMD for copying data/firmware.
 *
 * @param data   The data to copy.
 * @param len    The number of bytes that should be copied.
 * @param offset The offset within the flash device to write the data to.
 *               For internal flash, the offset is the same as the address.
 *
 * @retval non-negative integer on success, negative errno code on failure.
 */
int pcd_cmd_write(const void *data, size_t len, off_t offset);

/** @brief Invalidate the PCD CMD, indicating that the copy failed.
 *
 * @retval non-negative integer on success, negative errno code on failure.
 */
int pcd_cmd_invalidate(void);

/** @brief Check the PCD CMD to find the status of the update.
 *
 * @retval 0 if operation is not complete, positive integer if operation is
 *           complete, negative integer on failure.
 */
int pcd_cmd_status_get(void);

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
