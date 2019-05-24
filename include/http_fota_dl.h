/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file http_fota_dl.h
 *
 * @defgroup http_fota_dl Client to download a firmware update.
 * @{
 * @brief HTTP Firmware Over the Air Download
 *
 * @details Downloads the specified resource from the specified host to
 * MCUboots secondary partition.
 *
 */

#ifndef HTTP_FOTA_DL_H_
#define HTTP_FOTA_DL_H_

#include <zephyr.h>
#include <zephyr/types.h>
#include <download_client.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief HTTP FOTA DL event IDs.
 */
enum http_fota_dl_evt_id {
	/** Event contains a download client event. */
	HTTP_FOTA_DL_EVT_DOWNLOAD_CLIENT,
	/** Event contians a flash error code. */
	HTTP_FOTA_DL_FLASH_ERR,
};

/**
 * @brief HTTP FOTA DL event.
 */
struct http_fota_dl_evt {
	/** The event ID */
	enum http_fota_dl_evt_id id;
	union {
		const struct download_client_evt *dlc_event;
		int flash_error_code;
	};
};

/**
 * @brief HTTP FOTA DL asynchronous callback function.
 *
 * The application receives events through this callback. These events can
 * come from two sources, the flash storage system or the download client.
 * The source of the event is given by the http_fota_dl_evt_id.
 *
 * @param[in] event The event.
 *
 */
typedef void (*http_fota_dl_callback_t)(
	const struct http_fota_dl_evt *evt);

/**@brief Start downloading the given file from the given host.
 *
 * @retval 0 If download has started successfully.
 *           Otherwise, a negative value is returned.
 */
int http_fota_dl_start(char *host, char *file);

/**@brief Initialize the HTTP Firmware Over the Air Download.
 *
 * @param client_callback Callback for events generated.
 *
 * @retval 0 If successfully initialized.
 *           Otherwise, a negative value is returned.
 */
int http_fota_dl_init(http_fota_dl_callback_t client_callback);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_FOTA_DL_H_ */

/**@} */
