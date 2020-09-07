/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <logging/log.h>
#include <dfu/dfu_target.h>
#include <storage/stream_flash.h>
#include <settings/settings.h>

LOG_MODULE_REGISTER(dfu_target_stream, CONFIG_DFU_TARGET_LOG_LEVEL);

#define MODULE "dfu_stream"

struct dfu_target_stream_ctx {
	struct stream_flash_ctx stream;
	char id;
};

static struct dfu_target_stream_ctx *ctx;

/**
 * @brief Store the information stored in the stream_flash instance so that it
 *        can be restored from flash in case of a power failure, reboot etc.
 */
static int store_progress(void)
{
	if (IS_ENABLED(CONFIG_DFU_TARGET_STREAM_SAVE_PROGRESS)) {
		size_t bytes_written = stream_flash_bytes_written(&stream);

		int err = settings_save_one(ctx->id, &bytes_written,
					    sizeof(bytes_written));

		if (err) {
			LOG_ERR("Problem storing offset (err %d)", err);
			return err;
		}
	}

	return 0;
}

/**
 * @brief Function used by settings_load() to restore the stream_flash ctx.
 *	  See the Zephyr documentation of the settings subsystem for more
 *	  information.
 */
static int settings_set(const char *key, size_t len_rd,
			settings_read_cb read_cb, void *cb_arg)
{

	/* TODO  do we need to add 'module/key' here, or is simply 'key' enough? */
	/* TODO 2 here we must invoke a pre-known function from each target
	 *  in other words me must forward the result of this function to the
	 *  active target. */
	/* TODO also ensure that when we wake up the correct ID is pinged */

	if (!strcmp(key, ctx->id)) {
		size_t bytes_written = stream_flash_bytes_written(&stream);
		ssize_t len = read_cb(cb_arg, &bytes_written,
				      sizeof(bytes_written));
		if (len != sizeof(bytes_written)) {
			LOG_ERR("Can't read stream_ctx from storage");
			return len;
		}
	}

	return 0;
}

int dfu_target_stream_init(struct dfu_target_stream_ctx *target,
			   struct stream_flash_ctx *stream,
			   dfu_target_stream_id id)
{
	ARG_UNUSED(cb);

	if (ctx != NULL) {
		return -EPERM;
	}

	if (target == NULL || stream == NULL ||
	    id < DFU_TARGET_STREAM_ID_START ||
	    id >= DFU_TARGET_STREAM_ID_LAST) {
		return -EINVAL;
	}

	ctx = target;
	ctx->stream = stream;
	ctx->id = id;

	if (IS_ENABLED(CONFIG_DFU_TARGET_STREAM_SAVE_PROGRESS)) {
		int err;

		static struct settings_handler sh = {
			.name = MODULE,
			.h_set = settings_set,
		};

		/* settings_subsys_init is idempotent so this is safe to do. */
		err = settings_subsys_init();
		if (err) {
			LOG_ERR("settings_subsys_init failed (err %d)", err);
			return err;
		}

		err = settings_register(&sh);
		if (err) {
			LOG_ERR("Cannot register settings (err %d)", err);
			return err;
		}

		err = settings_load();
		if (err) {
			LOG_ERR("Cannot load settings (err %d)", err);
			return err;
		}
	}

	return 0;
}

int dfu_target_stream_offset_get(size_t *out)
{
	*out = stream_flash_bytes_written(&stream);

	return 0;
}

int dfu_target_stream_write(const void *const buf, size_t len)
{
	int err = stream_flash_buffered_write(ctx->stream,
					      (u8_t *)buf, len, false);

	if (err != 0) {
		LOG_ERR("stream_flash_buffered_write error %d", err);
		return err;
	}

	err = store_progress();
	if (err != 0) {
		/* Failing to store progress is not a critical error you'll just
		 * be left to download a bit more if you fail and resume.
		 */
		LOG_WRN("Unable to store write progress: %d", err);
	}

	return 0;
}

static void reset_stream_ctx(void)
{
	/* Need to set bytes_written to 0 */
	int err = stream_flash_init(&ctx->stream);

	if (err) {
		LOG_ERR("Unable to re-initialize stream");
	}

	err = store_progress();
	if (err != 0) {
		LOG_ERR("Unable to reset write progress: %d", err);
	}
}

int dfu_target_stream_done(bool successful)
{
	int err = 0;

	ctx = NULL;

	if (successful) {
		err = stream_flash_buffered_write(ctx->stream, NULL, 0, true);
		if (err != 0) {
			LOG_ERR("stream_flash_buffered_write error %d", err);
		}
	}

	reset_stream_ctx();

	return err;
}
