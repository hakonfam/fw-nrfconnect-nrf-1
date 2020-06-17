/*$$$LICENCE_NORDIC_STANDARD<2015>$$$*/
#include "sdk_common.h"
#if NRF_MODULE_ENABLED(SLIP)
#include "slip.h"

#include <string.h>


#define SLIP_BYTE_END             0300    /* indicates end of packet */
#define SLIP_BYTE_ESC             0333    /* indicates byte stuffing */
#define SLIP_BYTE_ESC_END         0334    /* ESC ESC_END means END data byte */
#define SLIP_BYTE_ESC_ESC         0335    /* ESC ESC_ESC means ESC data byte */


ret_code_t slip_encode(uint8_t * p_output,  uint8_t * p_input, uint32_t input_length, uint32_t * p_output_buffer_length)
{
    if (p_output == NULL || p_input == NULL || p_output_buffer_length == NULL)
    {
        return NRF_ERROR_NULL;
    }

    *p_output_buffer_length = 0;
    uint32_t input_index;

    for (input_index = 0; input_index < input_length; input_index++)
    {
        switch (p_input[input_index])
        {
            case SLIP_BYTE_END:
                p_output[(*p_output_buffer_length)++] = SLIP_BYTE_ESC;
                p_output[(*p_output_buffer_length)++] = SLIP_BYTE_ESC_END;
                break;

            case SLIP_BYTE_ESC:
                p_output[(*p_output_buffer_length)++] = SLIP_BYTE_ESC;
                p_output[(*p_output_buffer_length)++] = SLIP_BYTE_ESC_ESC;
                break;

            default:
                p_output[(*p_output_buffer_length)++] = p_input[input_index];
        }
    }
    p_output[(*p_output_buffer_length)++] = SLIP_BYTE_END;

    return NRF_SUCCESS;
}

ret_code_t slip_decode_add_byte(slip_t * p_slip, uint8_t c)
{
    if (p_slip == NULL)
    {
        return NRF_ERROR_NULL;
    }

    if (p_slip->current_index == p_slip->buffer_len)
    {
        return NRF_ERROR_NO_MEM;
    }

    switch (p_slip->state)
    {
        case SLIP_STATE_DECODING:
            switch (c)
            {
                case SLIP_BYTE_END:
                    // finished reading packet
                    return NRF_SUCCESS;

                case SLIP_BYTE_ESC:
                    // wait for
                    p_slip->state = SLIP_STATE_ESC_RECEIVED;
                    break;

                default:
                    // add byte to buffer
                    p_slip->p_buffer[p_slip->current_index++] = c;
                    break;
            }
            break;

        case SLIP_STATE_ESC_RECEIVED:
            switch (c)
            {
                case SLIP_BYTE_ESC_END:
                    p_slip->p_buffer[p_slip->current_index++] = SLIP_BYTE_END;
                    p_slip->state = SLIP_STATE_DECODING;
                    break;

                case SLIP_BYTE_ESC_ESC:
                    p_slip->p_buffer[p_slip->current_index++] = SLIP_BYTE_ESC;
                    p_slip->state = SLIP_STATE_DECODING;
                    break;

                default:
                    // protocol violation
                    p_slip->state = SLIP_STATE_CLEARING_INVALID_PACKET;
                    return NRF_ERROR_INVALID_DATA;
            }
            break;

        case SLIP_STATE_CLEARING_INVALID_PACKET:
            if (c == SLIP_BYTE_END)
            {
                p_slip->state = SLIP_STATE_DECODING;
                p_slip->current_index = 0;
            }
            break;
    }

    return NRF_ERROR_BUSY;
}
#endif //NRF_MODULE_ENABLED(SLIP)
