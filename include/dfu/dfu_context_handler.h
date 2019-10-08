/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file dfu_context_handler.h
 *
 * @defgroup dfu_ctx_handler DFU Context Handler
 * @{
 * @brief DFU Context Handler
 */

#ifndef DFU_CONTEXT_HANDLER_H__
#define DFU_CONTEXT_HANDLER_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Find the image type. Used to determine what dfu context to initialize.
 *
 * @param buf A buffer which contain the start of a firmware image.
 * @param len The length of the provided buffer.
 *
 * @return Positive identifier for an supported image type or a negative error
 *	   code identicating reason of failure.
 **/
int dfu_ctx_img_type(const void *const buf, size_t len);

/**
 * @brief Initialize the resources needed for the specific image type DFU
 *	  context. Invoking this function multiple times is supported, as it is
 *	  re-entrant. After its first invocation, no action is performed on
 *	  subsequent calls until dfu_ctx_done() has been called.
 *
 * @param img_type Image type identifier.
 *
 * @return 0 for an supported image type or a negative error
 *	   code identicating reason of failure.
 *
 **/
int dfu_ctx_init(int img_type);

/**
 * @brief Get offset of firmware stored in the current dfu context. Use this
 *	  function to resume an aborted DFU procedure. This function is 
 *	  idempotent.
 *
 * @return Offset of firmware
 */
int dfu_ctx_offset(void);

/**
 * @brief Write the given buffer to the initialized DFU context.
 *
 * @param buf Buffer to firmware data.
 * @param len The length of the provided buffer.
 *
 * @return 0 if success, otherwise a negative error code.
 *
 **/
int dfu_ctx_write(const void *const buf, size_t len);

/**
 * @brief Deinitialize the DFU context resources and finish DFU proceduce.
 * 	  Must be invoked after writing of firmware data has completed.
 *
 * @return 0 if success, otherwise a negative error code.
 *
 **/
int dfu_ctx_done(void);

#ifdef __cplusplus
}
#endif

#endif /* DFU_CONTEXT_HANDLER_H__ */

/**@} */
