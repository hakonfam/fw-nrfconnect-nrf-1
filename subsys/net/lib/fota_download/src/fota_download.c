/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdint.h>
#include <zephyr.h>
#include <flash.h>
#include <download_client.h>
#include <dfu_data_writer.h>
#include <dfu/mcuboot.h>
#include <pm_config.h>
#include <logging/log.h>
#include <fota_download.h>

LOG_MODULE_REGISTER(fota_download, CONFIG_FOTA_DOWNLOAD_LOG_LEVEL);

static		fota_download_callback_t callback;
static struct	download_client dfu;

static int download_client_callback(const struct download_client_evt *event)
{
	int err;
	__ASSERT(event != NULL, "invalid dfu object\n");

	switch (event->id) {
	case DOWNLOAD_CLIENT_EVT_FRAGMENT: {
		size_t size;

		err = dfu_data_writer_verify_size(size);
		if (err) {
			LOG_ERR("Requested file too big to fit in flash\n");
			return err;
		}

		err = download_client_file_size_get(&dfu, &size);
		if (err != 0) {
			LOG_ERR("download_client_file_size_get error %d\n",
					err);
			return err;
		}

		err = dfu_data_writer_fragment(event->fragment.buf,
				event->fragment.len);
		if (err != 0) {
			LOG_ERR("dfu_data_writer_fragment error %d\n", err);
			return 1;
		}
		break;
	}

	case DOWNLOAD_CLIENT_EVT_DONE:
		err = dfu_data_writer_finalize();
		if (err != 0) {
			LOG_ERR("dfu_data_writer_finalize error %d\n", err);
			return err;
		}
		err = download_client_disconnect(&dfu);
		if (err != 0) {
			LOG_ERR("download_client_disconncet error %d\n", err);
			return err;
		}
		callback(FOTA_DOWNLOAD_EVT_FINISHED);
		break;

	case DOWNLOAD_CLIENT_EVT_ERROR: {
		download_client_disconnect(&dfu);
		LOG_ERR("Download client error\n");
		callback(FOTA_DOWNLOAD_EVT_ERROR);
		return -1;
	}
	default:
		break;
	}

	return 0;
}

int fota_download_start(char *host, char *file)
{
	__ASSERT(host != NULL, "invalid host\n");
	__ASSERT(file != NULL, "invalid file\n");
	__ASSERT(callback != NULL, "invalid callback\n");

	/* Verify that a download is not already ongoing */
	if (dfu.fd != -1) {
		return -EALREADY;
	}

	int err = download_client_connect(&dfu, host);
	if (err != 0) {
		LOG_ERR("download_client_connect error %d\n", err);
		return err;
	}

	err = download_client_start(&dfu, file, 0);
	if (err != 0) {
		LOG_ERR("download_client_start error %d\n", err);
		download_client_disconnect(&dfu);
		return err;
	}
	return 0;
}

int fota_download_init(fota_download_callback_t client_callback)
{
	__ASSERT(client_callback != NULL, "invalid client_callback\n");

	callback = client_callback;

	int err = dfu_data_writer_init();
	if (err != 0) {
		LOG_ERR("dfu_data_writer_init error %d\n", err);
		return err;
	}

	err = download_client_init(&dfu, download_client_callback);
	if (err != 0) {
		LOG_ERR("download_client_init error %d\n", err);
		return err;
	}

	return 0;
}

