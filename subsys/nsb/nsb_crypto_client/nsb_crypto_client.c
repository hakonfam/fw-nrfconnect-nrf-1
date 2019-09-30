/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <nsb_crypto.h>
#include "nsb_crypto_internal.h"
#include <fw_metadata.h>
#include <kernel.h>

enum abi_index {BL_ROT_VERIFY, BL_SHA256, BL_SECP256R1, LAST};

const struct fw_abi_info *get_nsb_crypto_abi(enum abi_index abi)
{
	__ASSERT(abi < LAST, "invalid abi argument.");

	static const struct fw_abi_info *nsb_crypto_abis[LAST] = {NULL};

	if (!nsb_crypto_abis[abi]) {
		switch(abi) {
		case BL_ROT_VERIFY:
			nsb_crypto_abis[abi] = fw_abi_find(BL_ROT_VERIFY_ABI_ID, BL_ROT_VERIFY_ABI_FLAGS,
					BL_ROT_VERIFY_ABI_VER, BL_ROT_VERIFY_ABI_MAX_VER);
			break;
		case BL_SHA256:
			nsb_crypto_abis[abi] = fw_abi_find(BL_SHA256_ABI_ID, BL_SHA256_ABI_FLAGS,
					BL_SHA256_ABI_VER, BL_SHA256_ABI_MAX_VER);
			break;
		case BL_SECP256R1:
			nsb_crypto_abis[abi] = fw_abi_find(BL_SECP256R1_ABI_ID, BL_SECP256R1_ABI_FLAGS,
					BL_SECP256R1_ABI_VER, BL_SECP256R1_ABI_MAX_VER);
			break;
		default:
			k_oops();
		}
	}
	if (!nsb_crypto_abis[abi]){
		k_oops();
	}
	return nsb_crypto_abis[abi];
}

const struct nsb_rot_verify_abi *get_nsb_rot_verify_abi(void)
{
	return (const struct nsb_rot_verify_abi *)get_nsb_crypto_abi(BL_ROT_VERIFY);
}

const struct nsb_sha256_abi *get_nsb_sha256_abi(void)
{
	return (const struct nsb_sha256_abi *)get_nsb_crypto_abi(BL_SHA256);
}

const struct nsb_secp256r1_abi *get_nsb_secp256r1_abi(void)
{
	return (const struct nsb_secp256r1_abi *)get_nsb_crypto_abi(BL_SECP256R1);
}

int nsb_root_of_trust_verify(const u8_t *public_key, const u8_t *public_key_hash,
			 const u8_t *signature, const u8_t *firmware,
			 const u32_t firmware_len)
{
	return get_nsb_rot_verify_abi()->abi.nsb_root_of_trust_verify(public_key,
			public_key_hash, signature, firmware, firmware_len);
}

int nsb_sha256_init(nsb_sha256_ctx_t *ctx)
{
	if (sizeof(*ctx) < get_nsb_sha256_abi()->abi.nsb_sha256_ctx_size) {
		return -EFAULT;
	}
	return get_nsb_sha256_abi()->abi.nsb_sha256_init(ctx);
}

int nsb_sha256_update(nsb_sha256_ctx_t *ctx, const u8_t *data, u32_t data_len)
{
	return get_nsb_sha256_abi()->abi.nsb_sha256_update(ctx, data, data_len);
}

int nsb_sha256_finalize(nsb_sha256_ctx_t *ctx, u8_t *output)
{
	return get_nsb_sha256_abi()->abi.nsb_sha256_finalize(ctx, output);
}

int nsb_sha256_verify(const u8_t *data, u32_t data_len, const u8_t *expected)
{
	return get_nsb_sha256_abi()->abi.nsb_sha256_verify(data, data_len, expected);
}

int nsb_secp256r1_validate(const u8_t *hash, u32_t hash_len, const u8_t *public_key, const u8_t *signature)
{
	return get_nsb_secp256r1_abi()->abi.nsb_secp256r1_validate(hash, hash_len, public_key, signature);
}
