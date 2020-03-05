/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file mfu.h
 *
 * @defgroup mfu Modem firmware update library
 * @{
 * @brief Library for verifying and installing a downloaded modem firmware image.
 *
 * @details This library contains functionality to reads, validates, and install
 * a modem update image that has been downloaded to a flash device.
 * A modem image is generally too large to fit in nRF9160 internal flash,
 * which necessitates the use of external non-volatile memory.
 *
 * Verification is done via SHA256 hash, and optionally an ECDSA signature.
 *
 * Persistent state information is kept in order to resume an interrupted
 * update.
 *
 */

#ifndef __MFU_H__
#define __MFU_H__

#include <zephyr/types.h>
#include <device.h>

/**
 * @brief Initialize Modem Firmware Update library.
 *
 * @note This function must be called before BSDLib is initialized
 *
 * @note If the internal state indicates that an update was started but not finished,
 * the update will be automatically resumed/restarted.
 * The update process can take several minutes depending on the underlying
 * flash device used to store the update file.
 *
 * @param dev_name Name of flash device where update file is stored
 * @param addr Flash device address where update file is stored
 * @param public_key ECDSA public key. Can only be NULL if CONFIG_MFU_ALLOW_UNSIGNED
 * @param update_applied set to true if an interrupted update was resumed
 *
 * @retval 0 if successful
 */
int mfu_init(const char *dev_name, u32_t addr, const u8_t *public_key, bool *update_resumed);

/**
 * @brief Inform MFU that an update is now available
 *
 * @note This function only updates the persistent state.
 * Use @ref mfu_update_verify_integrity to validate update integrity.
 *
 * @retval 0 if successful
 * @retval -EPERM if MFU is not initialized
 * @retval -EINVAL if state is currently MFU_STATE_INSTALLING
 */
int mfu_update_available_set(void);

/**
 * @brief Inform MFU that an update is no longer available
 *
 * @note This function only updates the persistent state.
 *
 * @retval 0 if successful
 * @retval -EPERM if MFU is not initialized
 * @retval -EINVAL if state is currently MFU_STATE_INSTALLING
 */
int mfu_update_available_clear(void);

/**
 * @brief Check if MFU is marked as update available.
 *
 * @details This only checks internal state of MFU. @ref mfu_update_verify_integrity
 * is used to checks the integrity of the full update file.
 *
 * @retval true if update has been marked as available
 */
bool mfu_update_available_get(void);

/**
 * @brief Verify integrity of entire modem update file.
 *
 * @details This can take a while as the update file can be several megabytes in size.
 * The duration depends on the underlying flash device.
 * @ref mfu_update_apply will automatically parform this integrity check.
 *
 * @retval 0 if successful
 * @retval -EPERM if MFU is not initialized
 * @retval -ENODATA if verification fails
 */
int mfu_update_verify_integrity(void);


/*
 * TODO: Revisit "stream verification" concept and code
 *
 * Currently there is a bug where mfu_update_verify_stream_finalize() indicates
 * error when it is run directly from fota_dl_handler callback.
 * Potential flash access conflict?
 *
 * Furthermore, is this functionality really useful? The verification is either way performed again
 * before the image is actually installed.
 */

/**
 * @brief Initialize stream verification state.
 *
 * @details Stream verification is used to incrementally verify an update file
 * as it is being downloaded. This function resets the stream state information.
 *
 * @note When stream verification is done, take care not to use the same flash
 * buffer for both this module and the downloader module.
 */
void mfu_update_verify_stream_init(void);

/**
 * @brief Process a chunk of received modem update file
 *
 * @param offset Size/offset of data received so far
 *
 * @retval 0 if successful
 * @retval -EINVAL if the modem update header is invalid
 * @retval -EINVAL if the offset exceeds the size given in header
 */
int mfu_update_verify_stream_process(u32_t offset);

/**
 * @brief Complete the verification of modem update file
 *
 * @details This function runs @ref mfu_update_verify_stream_process for the
 * remaining part of the modem update file (if any) and compares the
 * generated hash digest with the modem update header.
 *
 * @retval 0 if successful
 * @retval -EINVAL if the verification fails
 */
int mfu_update_verify_stream_finalize(void);

/**
 * @brief Install modem update file
 *
 * @details This function always validates the image (@ref mfu_update_verify_integrity)
 * before installing it.
 * This process can take several minutes, depending on the underlying flash
 * device. One should make sure the application state will allow being suspended
 * for this period. E.g. ensure sufficient battery and that any running watchdog
 * is fed.
 *
 * @note This function must be called before BSD lib is initialized.
 * @note The modem should also reset after the update has completed.
 *
 * @retval 0 if successful
 */
int mfu_update_apply(void);

#endif /* __MFU_H__*/

/**@} */
