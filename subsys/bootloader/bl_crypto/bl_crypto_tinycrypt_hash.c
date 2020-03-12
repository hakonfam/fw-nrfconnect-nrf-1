/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <tinycrypt/sha256.h>
#include <tinycrypt/constants.h>
#include <stddef.h>
#include <stdbool.h>
#include <errno.h>
#include <bl_crypto.h>


BUILD_ASSERT_MSG(SHA256_CTX_SIZE >= sizeof(struct tc_sha256_state_struct), \
	"tc_sha256_state_struct can no longer fit inside bl_sha256_ctx_t.");

int get_hash(u8_t *hash, const u8_t *data, u32_t data_len, bool external)
{
	struct tc_sha256_state_struct ctx;
	int ret;
	(void) external;

	ret = bl_sha256_init(&ctx);
	if (ret != 0) {
		return ret;
	}

	ret = bl_sha256_update(&ctx, data, data_len);
	if (ret != 0) {
		return ret;
	}

	ret = bl_sha256_finalize(&ctx, hash);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

int bl_sha256_init(struct tc_sha256_state_struct *ctx)
{
	int ret;

	if (ctx == NULL) {
		return -EINVAL;
	}

	ret = tc_sha256_init(ctx);
	if (ret != TC_CRYPTO_SUCCESS) {
		return -EFAULT;
	}

	return 0;
}

int bl_sha256_update(struct tc_sha256_state_struct *ctx, const u8_t *data,
		     u32_t data_len)
{
	int ret;

	if (!ctx || !data) {
		return -EINVAL;
	}

	ret = tc_sha256_update(ctx, data, data_len);
	if (ret != TC_CRYPTO_SUCCESS) {
		return -EFAULT;
	}

	return 0;
}

int bl_sha256_finalize(struct tc_sha256_state_struct *ctx, u8_t *output)
{
	int ret;

	if (!ctx || !output) {
		return -EINVAL;
	}

	ret = tc_sha256_final(output, ctx);
	if (ret != TC_CRYPTO_SUCCESS) {
		return -EFAULT;
	}

	return 0;
}
