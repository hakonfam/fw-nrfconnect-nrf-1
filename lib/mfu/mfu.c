/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "mfu.h"
#include "mfu_types.h"
#include "mfu_state.h"

#include <string.h>
#include <drivers/flash.h>
#include <sys/byteorder.h>
#include <logging/log.h>
#include <bsd_platform.h>

#include "modem_dfu_rpc.h"

#if CONFIG_MFU_SHA256_BACKEND_MBEDTLS
#include "mbedtls/platform.h"
#include "mbedtls/sha256.h"
#elif CONFIG_MFU_SHA256_BACKEND_TINYCRYPT
#include <tinycrypt/constants.h>
#include <tinycrypt/sha256.h>
#else
#error No hash backend selected
#endif

#if CONFIG_MFU_ECDSA_BACKEND_TINYCRYPT
#include <tinycrypt/constants.h>
#include <tinycrypt/ecc_dsa.h>
#else
#error No verification backend selected
#endif

#define RPC_BUF_SIZE MODEM_RPC_BUFFER_MIN_SIZE
#define MFU_DIGEST_SIZE 32 /* SHA256 digest size */

LOG_MODULE_REGISTER(mfu, CONFIG_MFU_LOG_LEVEL);

#if CONFIG_MFU_SHA256_BACKEND_MBEDTLS
typedef struct mbedtls_sha256_context mfu_sha_ctx_t;
#elif CONFIG_MFU_SHA256_BACKEND_TINYCRYPT
typedef struct tc_sha256_state_struct mfu_sha_ctx_t;
BUILD_ASSERT(MFU_DIGEST_SIZE == TC_SHA256_DIGEST_SIZE);
#endif

struct mfu_verify_stream_state {
	u32_t total_len;
	u32_t processed;
	u8_t header_hash[MFU_DIGEST_SIZE];
	mfu_sha_ctx_t sha_ctx;
};

#if CONFIG_MFU_STREAM_VERIFICATION_ENABLE
static struct mfu_verify_stream_state stream_state;
#endif

static struct device *device;
static u32_t dev_offset;
static bool initialized;
static const u8_t *verification_key;

static u8_t flash_read_buf[CONFIG_MFU_FLASH_BUF_SIZE];

BUILD_ASSERT(sizeof(flash_read_buf) >= MFU_HEADER_MAX_LEN);
BUILD_ASSERT(RPC_BUF_SIZE <= BSD_RESERVED_MEMORY_SIZE);

static void mfu_sha_init(void *ctx)
{
#if CONFIG_MFU_SHA256_BACKEND_MBEDTLS
	mbedtls_sha256_init(ctx);
	int err = mbedtls_sha256_starts_ret(ctx, 0);

	__ASSERT_NO_MSG(err == 0);
#elif CONFIG_MFU_SHA256_BACKEND_TINYCRYPT
	int err = tc_sha256_init(ctx);

	__ASSERT_NO_MSG(err == TC_CRYPTO_SUCCESS);
#endif
}

static int mfu_sha_process(void *ctx, u8_t *input, size_t input_len)
{
#if CONFIG_MFU_SHA256_BACKEND_MBEDTLS
	int err = mbedtls_sha256_update_ret(ctx, input, input_len);

	if (err) {
		return -EINVAL;
	} else {
		return 0;
	}
#elif CONFIG_MFU_SHA256_BACKEND_TINYCRYPT
	int err = tc_sha256_update(ctx, input, input_len);

	if (err != TC_CRYPTO_SUCCESS) {
		return -EINVAL;
	} else {
		return 0;
	}
#endif

	return -EINVAL;
}

static int mfu_sha_finalize(void *ctx, u8_t digest[MFU_DIGEST_SIZE])
{
#if CONFIG_MFU_SHA256_BACKEND_MBEDTLS
	int err = mbedtls_sha256_finish_ret(ctx, digest);

	if (err) {
		return -EINVAL;
	} else {
		return 0;
	}
#elif CONFIG_MFU_SHA256_BACKEND_TINYCRYPT
	int err = tc_sha256_final(digest, ctx);

	if (err != TC_CRYPTO_SUCCESS) {
		return -EINVAL;
	} else {
		return 0;
	}
#endif

	return -EINVAL;
}


static int mfu_signature_verify(u8_t *sig, const u8_t *pk, u8_t *buf, size_t buf_size)
{
	mfu_sha_ctx_t sha_ctx;
	u8_t digest[MFU_DIGEST_SIZE];
	int err;

	mfu_sha_init(&sha_ctx);
	err = mfu_sha_process(&sha_ctx, buf, buf_size);
	if (err) {
		LOG_ERR("mfu_sha_process: %d", err);
		return -EINVAL;
	}

	err = mfu_sha_finalize(&sha_ctx, digest);
	if (err) {
		LOG_ERR("mfu_sha_finalize: %d", err);
		return -EINVAL;
	}

#if CONFIG_MFU_ECDSA_BACKEND_TINYCRYPT
	u8_t pk_bytes[NUM_ECC_BYTES * 2]; /* ECDSA public key is 2 x privkey size */

	uECC_vli_nativeToBytes(pk_bytes, NUM_ECC_BYTES, ((unsigned int *) pk));
	uECC_vli_nativeToBytes(&pk_bytes[NUM_ECC_BYTES], NUM_ECC_BYTES, &((unsigned int *) pk)[NUM_ECC_WORDS]);

	err = uECC_verify(pk_bytes, digest, sizeof(digest), sig, uECC_secp256r1());
	if (err != TC_CRYPTO_SUCCESS) {
		return -EINVAL;
	} else {
		return 0;
	}
#endif

	return -EINVAL;
}

/* Validate package header (including signature).
 * sha_buf is filled with relevant parts of header that will be part of
 * SHA256 calculation of the entire package.
 */
static int mfu_package_header_validate(
	enum mfu_header_type header_type,
	void *type_buf,
	u8_t *sha_buf,
	u32_t *data_length,
	u8_t **package_hash)
{
	u8_t *header_signature;
	struct mfu_header_type_pkg *ptr;
	int pos;
	int err;

	ptr = (struct mfu_header_type_pkg *) type_buf;
	header_signature = NULL;
	pos = 0;

	switch (header_type) {
	case MFU_HEADER_TYPE_ECDSA_SIGNED_PKG:
		header_signature =
			((struct mfu_header_type_ecdsa_signed_pkg *) type_buf)->signature;
		/* Fall-through: */
		/* Signed and unsigned package headers have the same layout */
		/* except for the signature at the end. */
	case MFU_HEADER_TYPE_UNSIGNED_PKG:
		sys_put_le32(ptr->magic_value, &sha_buf[pos]);
		pos += sizeof(u32_t);
		sys_put_le32(ptr->type, &sha_buf[pos]);
		pos += sizeof(u32_t);
		sys_put_le32(ptr->data_length, &sha_buf[pos]);
		pos += sizeof(u32_t);

		*data_length = ptr->data_length;
		*package_hash = ptr->package_hash;
		break;
	default:
		LOG_ERR("Unexpected package header");
		return -EPERM;
	}

	LOG_DBG("Header is signed: %s", header_signature ? "yes" : "no");

	if (header_signature && (verification_key == NULL)) {
		LOG_ERR("No public key for verification");
		return -EPERM;
	}

	if ((header_signature == NULL) && !IS_ENABLED(CONFIG_MFU_ALLOW_UNSIGNED)) {
		LOG_ERR("Unsigned update not allowed");
		return -EPERM;
	}

	if (header_signature) {
		err = mfu_signature_verify(
			header_signature,
			verification_key,
			type_buf,
			offsetof(struct mfu_header_type_ecdsa_signed_pkg, signature));
		if (err) {
			LOG_ERR("mfu_signature_verify: %d", err);
			return -EPERM;
		}
	}

	return 0;
}

/* Iterate through headers in package
 * The initial package header is skipped
 */
static int mfu_subheaders_iterate(
	enum mfu_header_type *header_type,
	void *header,
	size_t header_size,
	u32_t *offset,
	bool first)
{
	int err;
	u32_t data_length;
	u8_t read_buf[MFU_HEADER_MAX_LEN];
	u32_t read_offset;
	enum mfu_header_type next_type;

	if (first) {
		u8_t type_buf[MFU_HEADER_MAX_LEN];

		read_offset = *offset;

		err = flash_read(device, read_offset, read_buf, sizeof(read_buf));
		if (err) {
			LOG_ERR("flash_read: %d", err);
			return err;
		}

		err = mfu_header_get(
			header_type,
			read_buf, sizeof(read_buf),
			type_buf, sizeof(type_buf));
		if (err) {
			LOG_ERR("mfu_header_get: %d", err);
			return err;
		}
	}

	if (*header_type == MFU_HEADER_TYPE_UNSIGNED_PKG ||
		*header_type == MFU_HEADER_TYPE_ECDSA_SIGNED_PKG) {
		/* Package headers have data length covering entire update */
		/* The purpose here is to iterate over headers within update */
		data_length = 0;
	} else {
		data_length = mfu_header_data_length_get(*header_type, header);
	}

	read_offset = *offset + mfu_header_length_get(*header_type) + data_length;

	err = flash_read(device, read_offset, read_buf, sizeof(read_buf));
	if (err) {
		LOG_ERR("flash_read: %d", err);
		return err;
	}

	err = mfu_header_get(
		&next_type,
		read_buf, sizeof(read_buf),
		header, header_size);
	if (err) {
		return err;
	}

	*header_type = next_type;
	*offset = read_offset;

	return 0;
}

static int mfu_bl_segment_header_get(
	struct mfu_header_type_bl_segment *bl_segment_header)
{
	enum mfu_header_type type;
	u8_t header_read_buf[MFU_HEADER_MAX_LEN];
	u32_t offset = dev_offset;
	int err;
	bool first;

	first = true;

	/* Iterate over all headers and return first instance of BL segment */
	do {
		err = mfu_subheaders_iterate(&type, header_read_buf, sizeof(header_read_buf), &offset, first);
		first = false;
		if (!err) {
			switch (type) {
			case MFU_HEADER_TYPE_BL_SEGMENT:
				memcpy(
					bl_segment_header,
					header_read_buf,
					mfu_header_length_get(type));
				return 0;
			default:
				break;
			}
		}
	} while (!err);

	return -ENODATA;
}

static int mfu_modem_dfu_program_bootloader(
	struct mfu_header_type_bl_segment *header,
	u32_t bl_data_offset)
{
	modem_err_t modem_err;
	int err;

	LOG_DBG("mfu_modem_dfu_program_bootloader");

	modem_err = modem_start_transfer();
	if (modem_err != MODEM_SUCCESS) {
		LOG_ERR("modem_start_transfer: %d", modem_err);
		return -ENODEV;
	}

	u32_t bytes_read = 0;
	modem_memory_chunk_t bootloader_chunk = {
		.target_address = 0, /* Not used with bootloader firmware */
		.data = flash_read_buf,
		.data_len = 0,
	};

	do {
		u32_t read_length;
		u32_t read_offset;

		read_offset = bl_data_offset + bytes_read;
		read_length = (header->data_length - bytes_read);
		if (read_length > sizeof(flash_read_buf)) {
			read_length = sizeof(flash_read_buf);
		}

		LOG_DBG("Read %d bytes @ 0x%08X", read_length, read_offset);
		err = flash_read(device, read_offset, flash_read_buf, read_length);
		if (err) {
			LOG_ERR("flash_read: %d", err);
			goto abort;
		}

		bootloader_chunk.data_len = read_length;
		modem_err = modem_write_bootloader_chunk(&bootloader_chunk);
		if (modem_err != MODEM_SUCCESS) {
			LOG_ERR("modem_write_bootloader_chunk(%d / %d): %d",
				bytes_read, header->data_length, modem_err);
			err = -EINVAL;
			goto abort;
		}

		bytes_read += read_length;
	} while (bytes_read != header->data_length);

	modem_err = modem_end_transfer();
	if (modem_err != MODEM_SUCCESS) {
		LOG_ERR("modem_end_transfer: %d", modem_err);
		return -ENODEV;
	}

	return 0;

abort:
	modem_err = modem_end_transfer();
	if (modem_err != MODEM_SUCCESS) {
		LOG_ERR("modem_start_transfer: %d", modem_err);
		return -ENODEV;
	}

	return err;
}

static int mfu_modem_dfu_program_fw(
	bool has_digest,
	void *header,
	u32_t fw_data_offset)
{
	modem_err_t modem_err;
	u32_t fw_offset;
	u32_t fw_length;
	u8_t *fw_hash;
	int err;

	LOG_DBG("mfu_modem_dfu_program_fw(%d)", has_digest);

	if (has_digest) {
		struct mfu_header_type_fw_segment_w_digest *type_header = header;

		fw_offset = type_header->data_address;
		fw_length = type_header->data_length;
		fw_hash = type_header->plaintext_hash;
	} else {
		struct mfu_header_type_fw_segment *type_header = header;

		fw_offset = type_header->data_address;
		fw_length = type_header->data_length;
		fw_hash = NULL;
	}

	modem_err = modem_start_transfer();
	if (modem_err != MODEM_SUCCESS) {
		LOG_ERR("modem_start_transfer: %d", modem_err);
		return -ENODEV;
	}

	u32_t bytes_read = 0;
	modem_memory_chunk_t firmware_chunk = {
		.target_address = fw_offset,
		.data = flash_read_buf,
		.data_len = 0,
	};

	do {
		u32_t read_length;
		u32_t read_offset;

		read_offset = fw_data_offset + bytes_read;
		read_length = (fw_length - bytes_read);
		if (read_length > sizeof(flash_read_buf)) {
			read_length = sizeof(flash_read_buf);
		}

		err = flash_read(device, read_offset, flash_read_buf, read_length);
		if (err) {
			LOG_ERR("flash_read: %d", err);
			goto abort;
		}

		firmware_chunk.data_len = read_length;
		modem_err = modem_write_firmware_chunk(&firmware_chunk);
		if (modem_err != MODEM_SUCCESS) {
			LOG_ERR("modem_write_firmware_chunk(%d / %d): %d",
				bytes_read, fw_length, modem_err);
			err = -EINVAL;
			goto abort;
		}

		firmware_chunk.target_address += read_length;
		bytes_read += read_length;
	} while (bytes_read != fw_length);

	modem_err = modem_end_transfer();
	if (modem_err != MODEM_SUCCESS) {
		LOG_ERR("modem_end_transfer: %d", modem_err);
		return -ENODEV;
	}

	if (has_digest) {
		modem_digest_buffer_t digest_buffer;

		/* Address convention is "up to and including" for end address */
		modem_err = modem_get_memory_hash(
			fw_offset,
			fw_offset + fw_length - 1,
			&digest_buffer);
		if (modem_err != MODEM_SUCCESS) {
			LOG_ERR("modem_get_memory_hash: %d", modem_err);
			return -ENODEV;
		}

		if (memcmp(digest_buffer.data, fw_hash, sizeof(digest_buffer.data))) {
			LOG_ERR("Modem digest mismatch");
			return -ENODEV;
		}
	}

	return 0;

abort:
	modem_err = modem_end_transfer();
	if (modem_err != MODEM_SUCCESS) {
		LOG_ERR("modem_start_transfer: %d", modem_err);
		return -ENODEV;
	}

	return err;
}

int mfu_init(const char *dev_name, u32_t addr, const u8_t *public_key, bool *update_resumed)
{
	bool resume_update;
	int err;

	if (!update_resumed) {
		return -EINVAL;
	}

	*update_resumed = false;

	if (public_key) {
		verification_key = public_key;
	} else if (!IS_ENABLED(CONFIG_MFU_ALLOW_UNSIGNED)) {
		return -EINVAL;
	}

	device = device_get_binding(dev_name);
	if (!device) {
		return -ENODEV;
	}

	dev_offset = addr;

	err = mfu_state_init(device, CONFIG_MFU_STATE_ADDR);
	if (err) {
		return err;
	}

	resume_update = false;

	switch (mfu_state_get()) {
	case MFU_STATE_NO_UPDATE_AVAILABLE:
	case MFU_STATE_UPDATE_AVAILABLE:
	case MFU_STATE_INSTALL_FINISHED:
		/* No action needed */
		break;
	case MFU_STATE_INSTALLING:
		/* Update was started but not marked as finished: apply again */
		resume_update = true;
		break;
	default:
		return -EINVAL;
	}

	initialized = true;

	if (resume_update) {
		err = mfu_update_apply();
		if (err) {
			return err;
		}
		*update_resumed = true;
	} else {
		*update_resumed = false;
	}

	return 0;
}

int mfu_update_available_set(void)
{
	if (!initialized) {
		return -EPERM;
	}

	switch (mfu_state_get()) {
	case MFU_STATE_NO_UPDATE_AVAILABLE:
	case MFU_STATE_INSTALL_FINISHED:
		return mfu_state_set(MFU_STATE_UPDATE_AVAILABLE);
	case MFU_STATE_UPDATE_AVAILABLE:
	case MFU_STATE_INSTALLING:
		return 0;
	default:
		return -EPERM;
	}
}

int mfu_update_available_clear(void)
{
	if (!initialized) {
		return -EPERM;
	}

	switch (mfu_state_get()) {
	case MFU_STATE_INSTALLING:
		return -EINVAL;
	default:
		return mfu_state_set(MFU_STATE_UPDATE_AVAILABLE);
	}
}

bool mfu_update_available_get(void)
{
	if (!initialized) {
		return -EPERM;
	}

	switch (mfu_state_get()) {
	case MFU_STATE_UPDATE_AVAILABLE:
		return true;
	default:
		return false;
	}
}

int mfu_update_verify_integrity(void)
{
	/* sha_buf holds relevant parts of header for SHA calculation */
	u8_t sha_buf[offsetof(struct mfu_header_type_pkg, package_hash)];
	u8_t sha_digest[MFU_DIGEST_SIZE];
	u32_t data_length;
	u8_t *header_hash;
	enum mfu_header_type header_type;
	u8_t header_read_buf[MFU_HEADER_MAX_LEN];
	u8_t type_buf[MFU_HEADER_MAX_LEN];
	int err;

	if (!initialized) {
		return -EPERM;
	}

	/* Verification steps: */
	/* 1) Ensure that header can be parsed properly */
	/* 2) If header includes a signature, verification is done right away */
	/*    (Allowing unsigned packages is a config option) */
    /* 3) Generate SHA256 of entire update file (minus sha256 value in header)*/
	/*    and ensure it matches header value */

	err = flash_read(device, dev_offset, header_read_buf, sizeof(header_read_buf));
	if (err) {
		LOG_ERR("flash_read: %d", err);
		return err;
	}

	err = mfu_header_get(
		&header_type,
		header_read_buf, sizeof(header_read_buf),
		type_buf, sizeof(type_buf));
	if (err) {
		LOG_ERR("mfu_header_get: %d", err);
		return err;
	}

	err = mfu_package_header_validate(
		header_type,
		type_buf,
		sha_buf,
		&data_length,
		&header_hash);
	if (err) {
		LOG_ERR("mfu_package_header_validate: %d", err);
		return err;
	}

	mfu_sha_ctx_t sha_ctx;

	mfu_sha_init(&sha_ctx);
	err = mfu_sha_process(&sha_ctx, sha_buf, sizeof(sha_buf));
	if (err) {
		return err;
	}

	size_t bytes_read = 0;
	u32_t read_offset = dev_offset + mfu_header_length_get(header_type);

	for (int i = 0; i < data_length; i += sizeof(flash_read_buf)) {
		u32_t read_length;

		if ((bytes_read + sizeof(flash_read_buf)) <= data_length) {
			read_length = sizeof(flash_read_buf);
		} else {
			read_length = data_length - bytes_read;
		}

		err = flash_read(device, read_offset, flash_read_buf, read_length);
		if (err) {
			return err;
		}

		err = mfu_sha_process(&sha_ctx, flash_read_buf, read_length);
		if (err) {
			return err;
		}

		bytes_read += read_length;
		read_offset += read_length;
	}

	err = mfu_sha_finalize(&sha_ctx, sha_digest);
	if (err) {
		return err;
	}

	if (memcmp(sha_digest, header_hash, sizeof(sha_digest)) != 0) {
		LOG_ERR("Hash mismatch");
		return -ENODATA;
	}

	return 0;
}

#if CONFIG_MFU_STREAM_VERIFICATION_ENABLE
static int mfu_update_verify_stream_process_block(u32_t offset)
{
	while (stream_state.processed < offset) {
			u32_t read_offset;
			u32_t read_length;
			int err;

			read_offset = dev_offset + stream_state.processed;
			read_length = (offset - stream_state.processed);
			if (read_length > sizeof(flash_read_buf)) {
				read_length = sizeof(flash_read_buf);
			}

			LOG_DBG("Read %d @ 0x%08X", read_length, read_offset);

			if (stream_state.total_len < (stream_state.processed + read_length)) {
				LOG_ERR("Over read");
				return -EINVAL;
			}

			err = flash_read(device, read_offset, flash_read_buf, read_length);
			if (err) {
				LOG_ERR("flash_read: %d", err);
				return err;
			}

			err = mfu_sha_process(&stream_state.sha_ctx, flash_read_buf, read_length);
			if (err) {
				LOG_ERR("mfu_sha_process: %d", err);
				return err;
			}

			stream_state.processed += read_length;
		}

	return 0;
}

void mfu_update_verify_stream_init(void)
{
	memset(&stream_state, 0, sizeof(stream_state));
	mfu_sha_init(&stream_state.sha_ctx);
}

int mfu_update_verify_stream_process(u32_t offset)
{
	int err;

	LOG_DBG("mfu_update_verify_stream_process(%d)", offset);

	if (offset < (MFU_HEADER_MAX_LEN + CONFIG_MFU_STREAM_READ_OFFSET)) {
		return 0;
	}

	if (stream_state.processed == 0) {
		u8_t sha_buf[offsetof(struct mfu_header_type_pkg, package_hash)];
		enum mfu_header_type type;
		u8_t type_buf[MFU_HEADER_MAX_LEN];
		u8_t read_buf[MFU_HEADER_MAX_LEN];
		u8_t *header_hash;
		u32_t data_length;

		err = flash_read(device, dev_offset, read_buf, sizeof(read_buf));
		if (err) {
			return err;
		}

		err = mfu_header_get(
			&type,
			read_buf, sizeof(read_buf),
			type_buf, sizeof(type_buf));
		if (err) {
			return err;
		}

		err = mfu_package_header_validate(
			type,
			type_buf,
			sha_buf,
			&data_length,
			&header_hash);
		if (err) {
			return err;
		}

		stream_state.total_len = data_length + mfu_header_length_get(type);
		stream_state.processed = mfu_header_length_get(type);

		memcpy(
			stream_state.header_hash,
			header_hash,
			sizeof(stream_state.header_hash));

		err = mfu_sha_process(&stream_state.sha_ctx, sha_buf, sizeof(sha_buf));
		if (err) {
			return err;
		}
	}

	if ((stream_state.processed + CONFIG_MFU_STREAM_READ_OFFSET) < offset) {
		return mfu_update_verify_stream_process_block(offset - CONFIG_MFU_STREAM_READ_OFFSET);
	} else {
		return 0;
	}
}

int mfu_update_verify_stream_finalize(void)
{
	u8_t sha_digest[MFU_DIGEST_SIZE];
	int err;

	if (stream_state.processed != stream_state.total_len) {
		err = mfu_update_verify_stream_process_block(stream_state.total_len);
		if (err) {
			LOG_ERR("mfu_update_verify_stream_process: %d", err);
			return err;
		}
	}

	err = mfu_sha_finalize(&stream_state.sha_ctx, sha_digest);
	if (err) {
		LOG_ERR("mfu_sha_finalize: %d", err);
		return err;
	}

	if (memcmp(sha_digest, stream_state.header_hash, sizeof(sha_digest)) == 0) {
		return 0;
	} else {
		return -EINVAL;
	}
}
#endif /* CONFIG_MFU_STREAM_VERIFICATION_ENABLE */

int mfu_update_apply(void)
{
	int err;
	bool update_abort_possible;
	modem_err_t modem_err;
	modem_state_t modem_state;
	modem_digest_buffer_t modem_digest;
	u32_t modem_digest_snippet;
	struct mfu_header_type_bl_segment bl_segment_header;
	u8_t *rpc_buf;

	if (!initialized) {
		return -EPERM;
	}

	/* TODO: Verify that BSDLib is not yet initialized */
	rpc_buf = (u8_t *) BSD_RESERVED_MEMORY_ADDRESS;

	err = mfu_update_verify_integrity();
	if (err) {
		return err;
	}

	err = mfu_bl_segment_header_get(&bl_segment_header);
	if (err) {
		LOG_ERR("mfu_bl_segment_header_get: %d", err);
		return err;
	}

	modem_err = modem_dfu_rpc_init(&modem_digest, rpc_buf, RPC_BUF_SIZE);
	if (modem_err != MODEM_SUCCESS) {
		LOG_ERR("modem_dfu_rpc_init: %d", modem_err);
		return -ENODEV;
	}

	/* Only 28 bits from modem_digest is used for validation at this point */
	modem_digest_snippet =
		((modem_digest.data[0] << 20) & 0x0FF00000) |
		((modem_digest.data[1] << 12) & 0x000FF000) |
		((modem_digest.data[2] << 4)  & 0x00000FF0) |
		((modem_digest.data[3] >> 4)  & 0x0000000F);

	if (modem_digest_snippet != bl_segment_header.digest) {
		LOG_ERR("Bootloader mismatch: expected 0x%08X, got 0x%08X",
			modem_digest_snippet,
			bl_segment_header.digest);
		return -ENODATA;
	}

	err = mfu_state_set(MFU_STATE_INSTALLING);
	if (err) {
		return err;
	}

	/* Update can be aborted until firmware chunks are written */
	update_abort_possible = true;

	enum mfu_header_type type;
	u8_t header_buf[MFU_HEADER_MAX_LEN];
	u32_t offset;
	bool first;

	offset = dev_offset;
	first = true;

	do {
		err = mfu_subheaders_iterate(&type, header_buf, sizeof(header_buf), &offset, first);
		first = false;
		if (!err) {
			switch (type) {
			case MFU_HEADER_TYPE_BL_SEGMENT:
				LOG_DBG("MFU_HEADER_TYPE_BL_SEGMENT");
				err = mfu_modem_dfu_program_bootloader(
				(struct mfu_header_type_bl_segment *) header_buf,
					offset + mfu_header_length_get(MFU_HEADER_TYPE_BL_SEGMENT));
				if (err) {
					goto abort;
				}

				modem_state = modem_get_state();
				if (modem_state != MODEM_STATE_READY_FOR_IPC_COMMANDS) {
					LOG_ERR("Unexpected state: %d", modem_state);
					goto abort;
				}
				break;
			case MFU_HEADER_TYPE_FW_SEGMENT:
				LOG_DBG("MFU_HEADER_TYPE_FW_SEGMENT");
				update_abort_possible = false;
				err = mfu_modem_dfu_program_fw(
					false,
					header_buf,
					offset + mfu_header_length_get(MFU_HEADER_TYPE_FW_SEGMENT));
				if (err) {
					goto abort;
				}
				break;
			case MFU_HEADER_TYPE_FW_SEGMENT_W_DIGEST:
				LOG_DBG("MFU_HEADER_TYPE_FW_SEGMENT_W_DIGEST");
				update_abort_possible = false;
				err = mfu_modem_dfu_program_fw(
					true,
					header_buf,
					offset + mfu_header_length_get(MFU_HEADER_TYPE_FW_SEGMENT_W_DIGEST));
				if (err) {
					goto abort;
				}
				break;
			case MFU_HEADER_TYPE_UNSIGNED_PKG:
				break;
			default:
				LOG_ERR("Invalid header type");
				break;
			}
		}
	} while (!err);

	err = mfu_state_set(MFU_STATE_INSTALL_FINISHED);
	if (err) {
		goto abort;
	}

	memset(rpc_buf, 0, RPC_BUF_SIZE);

	return 0;

abort:
	memset(rpc_buf, 0, RPC_BUF_SIZE);

	if (update_abort_possible) {
		err = mfu_state_set(MFU_STATE_UPDATE_AVAILABLE);
		if (err) {
			return err;
		}
	}

	return -ENODEV;
}
