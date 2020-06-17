/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**@file
 *
 * @defgroup uart_transport Recovery bootloader UART transport layer
 * @{
 * @ingroup  sdk_recovery_bl
 * @brief    Recovery bootloader serial transport layer using UART.
 *
 * @details  The transport layer can be used for performing firmware updates
 *           over UART. The implementation uses SLIP to encode packets.
 */

#ifndef UART_TRANSPORT_H__
#define UART_TRANSPORT_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Function for initializing the transport layer.
 *
 * @retval NRF_SUCCESS              If the transport layer was successfully initialized.
 * @retval NRFX_ERROR_INVALID_STATE If UART driver is already initialized.
 * @retval NRFX_ERROR_BUSY          If other peripheral with the same ID is already in use.
 * @retval NRFX_ERROR_INTERNAL      If UART peripheral reported an error.
 */
int uart_transport_init(void);

#ifdef __cplusplus
}
#endif

#endif // UART_TRANSPORT_H__

/** @} */
