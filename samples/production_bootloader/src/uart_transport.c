/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "uart_transport.h"

#include <string.h>
#include <slip.h>
#include <nrfx_uarte.h>
#include "recovery_bootloader.h"

#include <logging/log.h>

LOG_MODULE_REGISTER(uart_transport, CONFIG_UART_TRANSPORT_LOG_LEVEL);

/**< Size of maximum UART easy DMA transmission. */
/* TODO remove division */
#define MAX_UARTE_PACKET_LEN (((1 << UARTE0_EASYDMA_MAXCNT_SIZE) -1)/1024)

/**
 * @brief   UART transport layer instance.
 *
 * @details This structure contains status information
 *          and data buffer for the the transport layer.
 */
typedef struct {
    slip_t  slip;
    uint8_t uart_buffer;
    uint8_t recv_buffer[RECOVERY_BL_TRANSPORT_MTU + 1];
    uint8_t tx_buffer[RECOVERY_BL_TRANSPORT_MTU + 1];
} uart_transport_t;

static uart_transport_t transport;
static bool tx_flag;
static nrfx_uarte_t uart_instance = NRFX_UARTE_INSTANCE(0);
static uint32_t encoded_response_length;
static uint8_t *data_chunk_ptr;

 /**< Buffer for the response message encoded in the SLIP format. */
static uint8_t encoded_response[(2 * RECOVERY_BL_TRANSPORT_MTU) + 1];

/**
 * @brief   Function for transmitting R-BL response packet.
 *
 * @details Encodes message into SLIP format and sends
 *          through UART.
 *
 * @param[in] p_response Response message to be transmitted.
 * @param[in] length     Response message length.
 */
static void response_send(recovery_bl_msg_resp_t *p_response,
                          uint32_t length)
{
    /* Discard message if there is ongoing transmission. */
    if (!tx_flag)
    {
        /* Reserve TX buffer. */
        tx_flag = true;

        /* Encode message into SLIP buffer. */
        (void)slip_encode(encoded_response,
                          (uint8_t *)p_response,
                          length,
                          &encoded_response_length);

        /* Trigger UART transmission. */
        if (encoded_response_length < MAX_UARTE_PACKET_LEN)
        {
            (void)nrfx_uarte_tx(&uart_instance, encoded_response,
			        encoded_response_length);
            encoded_response_length = 0;
        }
        else
        {
            (void)nrfx_uarte_tx(&uart_instance, encoded_response,
				MAX_UARTE_PACKET_LEN);
            encoded_response_length -= MAX_UARTE_PACKET_LEN;
            data_chunk_ptr = &encoded_response[MAX_UARTE_PACKET_LEN];
        }
    }
}

/**
 * @brief   Callback function to be called on SLIP packet
 *          reception.
 *
 * @details Calls higher layer function for R-BL message
 *          processing and sends response.
 *
 * @param[in] p_dfu Pointer to the serial transport layer instance.
 */
static void on_packet_received(uart_transport_t *p_dfu)
{
    const uint32_t packet_payload_len = p_dfu->slip.current_index;

    if (packet_payload_len != 0)
    {
        /* Process received message. */
        uint32_t resp_len = recovery_bl_msg_process(
				(recovery_bl_msg_req_t *)p_dfu->recv_buffer,
				(recovery_bl_msg_resp_t *)p_dfu->tx_buffer,
				packet_payload_len);

        /* Send response if there is any. */
        if (resp_len != 0)
        {
            response_send((recovery_bl_msg_resp_t *)p_dfu->tx_buffer, resp_len);
        }
    }
}

/**
 * @brief   Callback function to be called on UART byte
 *          reception.
 *
 * @details Decodes received SLIP byte and calls received
 *          packet handler if decoded whole packet.
 *
 * @param[in] p_dfu  Pointer to the serial transport layer instance.
 * @param[in] p_data Pointer to the received bytes table.
 * @param[in] len    Received data length.
 */
static __INLINE void on_rx_complete(uart_transport_t *p_dfu, uint8_t *p_data)
{
    int err;


    /* Decode bytes received in SLIP format. */
    err = slip_decode_add_byte(&p_dfu->slip, p_data[0]);

    /* Check if received whole packet. */
    if (err == 0)
    {
        /* Process received message. */
        on_packet_received(p_dfu);

        /* Reset the SLIP decoding. */
        p_dfu->slip.current_index = 0;
        p_dfu->slip.state         = SLIP_STATE_DECODING;
    }

    /* Resume bytes reception. */
    (void)nrfx_uarte_rx(&uart_instance, &transport.uart_buffer, 1);
}

/**
 * @brief   Callback function to be called by UART driver.
 *
 * @details Reacts to the UART events.
 *
 * @param[in] p_event   Pointer to the UART event structure.
 * @param[in] p_context Pointer to the context.
 */
static void uart_event_handler(nrfx_uarte_event_t const *p_event,
			       void *p_context)
{
    switch (p_event->type)
    {
        case NRFX_UARTE_EVT_TX_DONE:
        {
            if (encoded_response_length != 0)
            {
                /* Trigger UART transmission. */
                if (encoded_response_length < MAX_UARTE_PACKET_LEN)
                {
                    (void)nrfx_uarte_tx(&uart_instance,
                                         data_chunk_ptr,
                                         encoded_response_length);
                    encoded_response_length = 0;
                }
                else
                {
                    (void)nrfx_uarte_tx(&uart_instance, data_chunk_ptr,
				        MAX_UARTE_PACKET_LEN);
                    encoded_response_length -= MAX_UARTE_PACKET_LEN;
                    data_chunk_ptr          += MAX_UARTE_PACKET_LEN;
                }
            }
            else
            {
                /* Release TX buffer. */
                tx_flag = false;
                /* Call higher layer transmission end callback. */
                recovery_bl_tx_end_callback();
            }
        } break;

        case NRFX_UARTE_EVT_RX_DONE:
        {
            /* Call RX data handler. */
            on_rx_complete((uart_transport_t *)p_context,
			   p_event->data.rxtx.p_data);
        } break;

        case NRFX_UARTE_EVT_ERROR:
        {
            /* Ignore error, continue receiving. */
            (void)nrfx_uarte_rx(&uart_instance, &transport.uart_buffer, 1);
        } break;
    }
}

int uart_transport_init(void)
{
    nrfx_err_t err = NRFX_SUCCESS;

    /* Initialize SLIP decoder. */
    transport.slip.p_buffer      = transport.recv_buffer;
    transport.slip.current_index = 0;
    transport.slip.buffer_len    = sizeof(transport.recv_buffer);
    transport.slip.state         = SLIP_STATE_DECODING;

    /* Set up UART port parameters. */
    /* TODO Use device tree for this  and below*/
    nrfx_uarte_config_t uart_config = NRFX_UARTE_DEFAULT_CONFIG(20, 22);

    /* TODO
       uart_config.pseltxd   = CUSTOM_UART_TX_PIN_NUMBER;
    uart_config.pselrxd   = CUSTOM_UART_RX_PIN_NUMBER;
    uart_config.pselcts   = CUSTOM_UART_CTS_PIN_NUMBER;
    uart_config.pselrts   = CUSTOM_UART_RTS_PIN_NUMBER;
    */

    uart_config.p_context = &transport;

    /* Start HFCLK clock - needed for high baudrate UART transfers. */
    NRF_CLOCK->TASKS_HFCLKSTART |= 1;

    /* Initialize UART. */
    err =  nrfx_uarte_init(&uart_instance, &uart_config, uart_event_handler);
    if (err != NRFX_SUCCESS) {
        LOG_ERR("Failed initializing uart");
        return err;
    }

    /* Start data reception. */
/* TODO add error check */
    err = nrfx_uarte_rx(&uart_instance, &transport.uart_buffer, 1);
    if (err != NRFX_SUCCESS)
    {
        LOG_ERR("Failed initializing rx");
    }

    LOG_DBG("UART initialized");

    return err;
}
