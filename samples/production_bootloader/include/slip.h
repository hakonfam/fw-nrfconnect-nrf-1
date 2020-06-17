/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef SLIP_H__
#define SLIP_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @file
 *
 * @defgroup slip SLIP encoding and decoding
 * @{
 * @ingroup app_common
 *
 * @brief  This module encodes and decodes SLIP packages.
 *
 * @details The SLIP protocol is described in @linkSLIP.
 */

/** @brief Status information that is used while receiving and decoding a packet. */
typedef enum
{
  SLIP_STATE_DECODING, //!< Ready to receive the next byte.
  SLIP_STATE_ESC_RECEIVED, //!< An ESC byte has been received and the next byte must be decoded differently.
  SLIP_STATE_CLEARING_INVALID_PACKET //!< The received data is invalid and transfer must be restarted.
} slip_read_state_t;

  /** @brief Representation of a SLIP packet. */
typedef struct
{
  slip_read_state_t   state; //!< Current state of the packet (see @ref slip_read_state_t).

  uint8_t             * p_buffer; //!< Decoded data.
  uint32_t            current_index; //!< Current length of the packet that has been received.
  uint32_t            buffer_len; //!< Size of the buffer that is available.
} slip_t;

/**@brief Function for encoding a SLIP packet.
 *
 * The maximum size of the output data is (2*input size + 1) bytes. Ensure that the provided buffer is large enough.
 *
 * @param[in,out]   p_output                The buffer where the encoded SLIP packet is stored. Ensure that it is large enough.
 * @param[in,out]   p_input                 The buffer to be encoded.
 * @param[in,out]   input_length            The length of the input buffer.
 * @param[out]      p_output_buffer_length  The length of the output buffer after the input has been encoded.
 *
 * @retval  NRF_SUCCESS         If the input was successfully encoded into output.
 * @retval  NRF_ERROR_NULL      If one of the provided parameters is NULL.
 */
int slip_encode(uint8_t * p_output,  uint8_t * p_input, uint32_t input_length, uint32_t * p_output_buffer_length);

/**@brief Function for decoding a SLIP packet.
 *
 * The decoded packet is put into @p p_slip::p_buffer. The index and buffer state is updated.
 *
 * Ensure that @p p_slip is properly initialized. The initial state must be set to @ref SLIP_STATE_DECODING.
 *
 * @param[in,out]   p_slip  State of the decoding process.
 * @param[in]       c       Byte to decode.
 *
 * @retval NRF_SUCCESS              If a packet has been parsed. The received packet can be retrieved from @p p_slip.
 * @retval NRF_ERROR_NULL           If @p p_slip is NULL.
 * @retval NRF_ERROR_NO_MEM         If there is no more room in the buffer provided by @p p_slip.
 * @retval NRF_ERROR_BUSY           If the packet has not been parsed completely yet.
 * @retval NRF_ERROR_INVALID_DATA   If the packet is encoded wrong. In this case, @p p_slip::state is set to @ref SLIP_STATE_CLEARING_INVALID_PACKET,
 *                                  and decoding will stay in this state until the END byte is received.
 */
int slip_decode_add_byte(slip_t * p_slip, uint8_t c);

#ifdef __cplusplus
}
#endif

#endif // SLIP_H__

/** @} */
