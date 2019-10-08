/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file dfu_ctx_mcuboot.h
 *
 * @defgroup dfu_ctx_mcuboot MCUBoot DFU Context Handler
 * @{
 * @brief DFU Context Handler for MCUBoot style updates
 */

#ifndef DFU_CTX_MCUBOOT_H__
#define DFU_CTX_MCUBOOT_H__

/**
 * @brief Initialize dfu context, perform steps necessary for preparing to
 *        receive new firmware.
 *
 * @retval 0 If successful, negative errno otherwise.
 */
int dfu_ctx_mcuboot_init(void);

/**
 * @brief Get offset of firmware
 *
 * @return Offset of firmware
 */
int dfu_ctx_mcuboot_offset(void);

/**
 * @brief Write firmware data.
 *
 * @param buf Pointer to data that should be written.
 * @param len Length of data to write.
 *
 * @return 0 on success, negative errno otherwise.
 */
int dfu_ctx_mcuboot_write(const void *const buf, size_t len);

/**
 * @brief Finalize firmware transfer.
 *
 * @return 0 on success, negative errno otherwise.
 */
int dfu_ctx_mcuboot_done(void);

#endif /* DFU_CTX_MCUBOOT_H__ */

/**@} */
