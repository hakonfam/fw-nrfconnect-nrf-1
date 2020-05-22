/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/*
 * Ensure 'strnlen' is available even with -std=c99. If
 * _POSIX_C_SOURCE was defined we will get a warning when referencing
 * 'strnlen'. If this proves to cause trouble we could easily
 * re-implement strnlen instead, perhaps with a different name, as it
 * is such a simple function.
 */
#if !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif
#include <string.h>

#include <zephyr.h>
#include <pm_config.h>
#include <logging/log.h>
#include <dfu/dfu_target.h>
#include <dfu/dfu_target_stream.h>

LOG_MODULE_REGISTER(dfu_target_b1, CONFIG_DFU_TARGET_LOG_LEVEL);

#define MAX_FILE_SEARCH_LEN 500

int dfu_ctx_b1_set_file(const char *file, bool s0_active,
				const char **update)
{
	if (file == NULL || update == NULL) {
		return -EINVAL;
	}

	/* Ensure that 'file' is null-terminated. */
	if (strnlen(file, MAX_FILE_SEARCH_LEN) == MAX_FILE_SEARCH_LEN) {
		return -ENOTSUP;
	}

	/* We have verified that there is a null-terminator, so this is safe */
	char *space = strstr(file, " ");

	if (space == NULL) {
		/* Could not find space separator in input */
		*update = NULL;

		return 0;
	}

	if (s0_active) {
		/* Point after space to 'activate' second file path (S1) */
		*update = space + 1;
	} else {
		*update = file;

		/* Insert null-terminator to 'activate' first file path (S0) */
		*space = '\0';
	}

	return 0;
}

bool dfu_target_b1_identify(const void *const buf)
{
	u32_t *vtable = (u32_t *)(buf);
	u32_t reset_addr = vtable[1];
	return (reset_addr > PM_S0_ADDR) &&
	       (reset_addr < (PM_S1_ADDR + PM_S1_SIZE));
}

int dfu_target_b1_init(size_t file_size, dfu_target_callback_t cb)
{
	ARG_UNUSED(cb);
	int err = dfu_target_stream(file_size); /* TODO */

	if (err != 0) {
		LOG_ERR("stream_flash_init error %d", err);
		return err;
	}

	if (file_size > PM_S0_SIZE) {
		LOG_ERR("Requested file too big to fit in flash %zu > 0x%x",
			file_size, PM_S0_SIZE);
		return -EFBIG;
	}

	return 0;
}

int dfu_target_b1_offset_get(size_t *out)
{
	return dfu_target_stream_offset_get(out);
}

int dfu_target_b1_write(const void *const buf, size_t len)
{
	return dfu_target_stream_write(buf, len);
}

int dfu_target_b1_done(bool successful)
{
	return dfu_target_stream_done(successful);
}
