/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef MFU_EXT_H__
#define MFU_EXT_H__

#include <dfu/mfu_stream.h>

/**
 * @defgroup mfu_ext Full Modem Firmware from External Flash
 * @{
 *
 * @brief Functions for applying a full modem firmware update from external
 *        flash.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize buffer.
 *
 * @param[in] buf_in Pointer to buffer.
 * @param[in] buf_len Length of provided buffer.
 *
 * @return 0 if success, otherwise negative value if unable to get the offset
 */
int mfu_ext_init(uint8_t *buf_in, size_t buf_len);

/**
 * @brief Load the modem firmware update from the external flash to the modem.
 *
 * @param[in] fdev External flash device.
 * @param[in] offset Offset within external flash device to first byte of
 *                   modem update data.
 *
 * @return 0 if success, otherwise negative value if unable to get the offset
 */
int mfu_ext_load(const struct device *fdev, size_t offset);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* MFU_EXT_H__ */
