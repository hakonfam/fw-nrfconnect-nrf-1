/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file dfu_data_writer.h
 *
 * @defgroup dfu_data_writer Library to store firmware update data.
 * @{
 * @brief DFU Data Writer
 *
 * @details Writes the given fragments to MCUboots secondary partition.
 * When the 'finalize' function is called, the secondary slot is tagged to
 * have a valid firmware inside it.
 */

#ifndef DFU_DATA_WRITER_H_
#define DFU_DATA_WRITER_H_

#include <zephyr.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Verify total size of firmware to be written.
 *
 * @param size    The total size of the firmware to be written.
 *
 * @retval 0      If the size is acceptable.
 * @retval -EFBIG If the size is too big.
 */
int dfu_data_writer_verify_size(size_t size);


/**@brief Initialize library.
 *
 * @retval 0 If successfully initialized.
 *           Otherwise, a negative value is returned.
 */
int dfu_data_writer_init(void);

/**@brief Write firmware fragment.
 *
 * @param[in] data The data to be written.
 * @param[in] len  The length of the data.
 *
 * @retval 0       If data was successfully written.
 * @retval -ENOSPC If the current address + len exceeds the secondary slot.
 *           Otherwise, a negative value is returned.
 */
int dfu_data_writer_fragment(const u8_t *data, size_t len);

/**@brief Finalize the firmware write operation.
 *
 * When the write is finalized, MCUboots secondary slot is tagged as having
 * a valid firmware inside it. The completion is reported through an event.
 *
 * @retval 0 If successfully initialized.
 *           Otherwise, a negative value is returned.
 */
int dfu_data_writer_finalize(void);

#ifdef __cplusplus
}
#endif

#endif /* DFU_DATA_WRITER_H_ */

/**@} */
