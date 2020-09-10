/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <logging/log.h>
#include <storage/stream_flash.h>

LOG_MODULE_REGISTER(dfu_target_stream, CONFIG_DFU_TARGET_LOG_LEVEL);

static struct stream_flash_ctx stream;
static const char *current_id;

#ifdef CONFIG_DFU_TARGET_STREAM_SAVE_PROGRESS

#define MODULE "dfu_stream"
#include <settings/settings.h>
/**
 * @brief Store the information stored in the stream_flash instance so that it
 *        can be restored from flash in case of a power failure, reboot etc.
 */
static int store_progress(void)
{
	size_t bytes_written = stream_flash_bytes_written(&stream);

	int err = settings_save_one(current_id, &bytes_written,
			sizeof(bytes_written));

	if (err) {
		LOG_ERR("Problem storing offset (err %d)", err);
		return err;
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

	if (!strcmp(key, current_id)) {
		ssize_t len = read_cb(cb_arg, &stream.bytes_written,
				      sizeof(stream.bytes_written));
		if (len != sizeof(stream.bytes_written)) {
			LOG_ERR("Can't read stream.bytes_written from storage");
			return len;
		}
	}

	return 0;
}
#endif /* CONFIG_DFU_TARGET_STREAM_SAVE_PROGRESS */

struct stream_flash_ctx * dfu_target_stream_get_stream(void)
{
	return &stream;
}

int dfu_target_stream_init(const char *id, struct device *fdev, uint8_t *buf,
			   size_t len, size_t offset, size_t size,
			   stream_flash_callback_t cb)
{
	int err;

	ARG_UNUSED(cb);

	if (current_id != NULL) {
		return -EFAULT;
	}

	if (id == NULL || fdev == NULL || buf == NULL) {
		return -EINVAL;
	}

	current_id = id;

	err = stream_flash_init(&stream, fdev, buf, len, offset, size, NULL);
	if (err) {
		LOG_ERR("stream_flash_init failed (err %d)", err);
		return err;
	}

#ifdef CONFIG_DFU_TARGET_STREAM_SAVE_PROGRESS
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
#endif /* CONFIG_DFU_TARGET_STREAM_SAVE_PROGRESS */

	return 0;
}

int dfu_target_stream_offset_get(size_t *out)
{
	*out = stream_flash_bytes_written(&stream);

	return 0;
}

int dfu_target_stream_write(const uint8_t *buf, size_t len)
{
	int err = stream_flash_buffered_write(&stream, buf, len, false);

	if (err != 0) {
		LOG_ERR("stream_flash_buffered_write error %d", err);
		return err;
	}

#ifdef CONFIG_DFU_TARGET_STREAM_SAVE_PROGRESS
	/* TODO try to avoid all these ifdefs and use IS_ENABLED instaed */
	err = store_progress();
	if (err != 0) {
		/* Failing to store progress is not a critical error you'll just
		 * be left to download a bit more if you fail and resume.
		 */
		LOG_WRN("Unable to store write progress: %d", err);
	}
#endif

	return 0;
}

int dfu_target_stream_done(bool successful)
{
	int err = 0;

	/* TODO how to handle ID here? should we set it to NULL and then
	 * add some NULL checks in the write/init code?
	 * By doing this, we ensure that init must be called again,
	 * and hence we don't need enough info here to call
	 * stream_init again. */
	current_id = NULL;

	if (successful) {
		err = stream_flash_buffered_write(&stream, NULL, 0, true);
		if (err != 0) {
			LOG_ERR("stream_flash_buffered_write error %d", err);
		}
	}

#ifdef CONFIG_DFU_TARGET_STREAM_SAVE_PROGRESS
	err = store_progress();
	if (err != 0) {
		LOG_ERR("Unable to reset write progress: %d", err);
	}
#endif


	return err;
}
