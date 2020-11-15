/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <modem_update_decode.h>
#include <drivers/flash.h>
#include <logging/log.h>
#include <dfu/mfu_ext.h>
#include <nrf_fmfu.h>
#include <export/nrf_fmfu.h>
#include <mbedtls/sha256.h>

LOG_MODULE_REGISTER(mfu_ext, CONFIG_MFU_EXT_LOG_LEVEL);

#define MAX_META_LEN sizeof(COSE_Sign1_Manifest_t)

static uint8_t *buf;
static size_t buf_len;

static int get_hash_from_flash(const struct device *fdev, size_t offset,
				size_t data_len, uint8_t *hash,
				uint8_t *buffer, size_t buffer_len)
{
	int err;
	mbedtls_sha256_context sha256_ctx;
	size_t end = offset + data_len;

	mbedtls_sha256_init(&sha256_ctx);

	err = mbedtls_sha256_starts_ret(&sha256_ctx, false);
	if (err != 0) {
		return err;
	}

	for (size_t p_offs = offset; p_offs < end; p_offs += buffer_len) {
		size_t part_len = MIN(buffer_len, (end - p_offs));
		int err = flash_read(fdev, p_offs, buffer, part_len);

		if (err != 0) {
			return err;
		}

		err = mbedtls_sha256_update_ret(&sha256_ctx, buffer, part_len);
		if (err != 0) {
			return err;
		}
	}

	err = mbedtls_sha256_finish_ret(&sha256_ctx, hash);
	if (err != 0) {
		return err;
	}

	return 0;
}

static int prevalidate(const COSE_Sign1_Manifest_t *wrapper, const uint8_t *pk,
		       size_t pk_len, uint8_t *buf, size_t buf_len)
{
	return 0;
}

static int get_blob_len(const uint8_t *segments_buf, size_t buf_len,
			size_t *segments_len, Segments_t *segments)
{
	bool result;

	result = cbor_decode_Segments(segments_buf, buf_len, segments, NULL);

	if (!result) {
		return -EINVAL;
	}

	*segments_len = 0;
	for (int i = 0; i < segments._Segments__Segment_count; i++) {
		*segments_len += segments._Segments__Segment[i]._Segment_len;
	}

	return 0;
}

static int load_segment(const struct device *fdev, size_t seg_size,
			uint32_t seg_target_addr, uint32_t seg_offset)
{
	int err;
	uint32_t read_addr = seg_offset;
	size_t bytes_left = seg_size;

	while (bytes_left) {
		uint32_t read_len = MIN(RPC_BUFFER_SIZE, bytes_left);

		err = flash_read(fdev, read_addr, buf, read_len);
		if (err != 0) {
			LOG_ERR("flash_read failed2: %d", err);
			return err;
		}

		err = nrf_fmfu_memory_chunk_write(seg_target_addr, read_len,
						  buf);
		if (err != 0) {
			LOG_ERR("nrf_fmfu_memory_chunk_write failed: %d", err);
			return err;
		}

		LOG_DBG("wrote chunk: offset 0x%x target addr 0x%x size 0x%x",
			read_addr, seg_target_addr, read_len);

		seg_target_addr += read_len;
		bytes_left -= read_len;
		read_addr += read_len;
	}

	return 0;
}

static int load_segments(Segments_t *seg, size_t blob_offset)
{
	int err;
	size_t prev_segments_len = 0;
	bool result;


	LOG_INF("Writing %d segments", seg._Segments__Segment_count);

	for (int i = 0; i < seg._Segments__Segment_count; i++) {
		size_t seg_size = seg._Segments__Segment[i]._Segment_len;
		uint32_t seg_addr =
			seg._Segments__Segment[i]._Segment_target_addr;
		uint32_t read_addr = blob_offset + prev_segments_len;

		err = nrf_fmfu_transfer_start();
		if (err != 0) {
			LOG_ERR("nrf_fmfu_transfer_start failed1: %d", err);
			return err;
		}

		err = load_segment(fdev, seg_size, seg_addr, read_addr);
		if (err != 0) {
			LOG_ERR("load_segment failed: %d", err);
			return err;
		}

		err = nrf_fmfu_transfer_end();
		if (err != 0) {
			LOG_ERR("nrf_fmfu_transfer_end failed: %d", err);
			return err;
		}

		prev_segments_len += seg_size;

		LOG_INF("Segment %d written. Target addr: 0x%x, size: 0%x",
			i, seg_addr, seg_size);
	}

	err = nrf_fmfu_end();
	if (err != 0) {
		LOG_ERR("nrf_fmfu_end failed: %d", err);
		return err;
	}

	LOG_INF("FMFU finished");

	return 0;
}

int mfu_ext_init(uint8_t *buf_in, size_t buf_len_in)
{
	int err;

	if (buf_in == NULL) {
		return -EINVAL;
	}

	buf = buf_in;
	buf_len = buf_len_in;

	return 0;
}

int mfu_ext_load(const struct device *fdev, size_t offset)
{
	const cbor_string_type_t *segments_string;
	struct nrf_fmfu_digest digest_buffer;
	COSE_Sign1_Manifest_t wrapper;
	bool hash_len_valid = false;
	uint8_t expected_hash[32];
	bool hash_valid = false;
	bool sig_valid = false;
	Segments_t segments;
	uint8_t hash[32];
	size_t blob_len;
	int err;

	if (buf == NULL) {
		LOG_ERR("Not initialized, please call mfu_ext_init()");
		return -ENOMEM;
	}

	/* Put modem in DFU/RPC state */
	err = nrf_fmfu_init(&digest_buffer, buf_len, buf);
	if (err != 0) {
		LOG_ERR("nrf_fmfu_init failed: %d\n", err);
	}

	/* Read the whole wrapper. */
	err = flash_read(fdev, offset, buf, MAX_META_LEN);
	if (err != 0) {
		return err;
	}

	/* Pass 'blob_offset' so that it will get the length of the decoded
	 * data
	 */
	if (!cbor_decode_Wrapper(buf, MAX_META_LEN, &wrapper, blob_offset)) {
		return -EINVAL;
	}

	/* Prevalidate as soon as possible to avoid reading unvalidated data */
	err = prevalidate(&wrapper, NULL, 0, &buf[MAX_META_LEN], MAX_META_LEN);
	if (err == 0) {
		sig_valid = true;
	} else {
		return err;
	}

	/* Add the base offset of the wrapper to the blob offset to get the
	 * absolute offset to the blob in the flash device.
	 */
	*blob_offset += offset;

	/* Get a pointer to the segments as this is a cbor encoded structure
	 * in itself.
	 */
	segments_string = &wrapper._COSE_Sign1_Manifest_payload_cbor
			._Manifest_segments;
	*seg_offset = offset + (size_t)segments_string->value - (size_t)buf;


	/* Extract the expected hash from the manifest */
	memcpy(expected_hash, wrapper._COSE_Sign1_Manifest_payload_cbor
			._Manifest_blob_hash.value, sizeof(expected_hash));

	/* Calculate total length of all segments */
	err = get_blob_len(segments_string->value, segments_string->len,
			   &blob_len, &segments);
	if (err != 0) {
		return err;
	}

	if (sizeof(hash) == wrapper._COSE_Sign1_Manifest_payload_cbor
			._Manifest_blob_hash.len) {
		hash_len_valid = true;
	} else {
		return -EINVAL;
	}

	err = get_hash_from_flash(fdev, *blob_offset, blob_len, hash, buf,
				  buf_len);
	if (err != 0) {
		return err;
	}

	if (memcmp(expected_hash, hash, sizeof(hash)) == 0) {
		hash_valid = true;
	} else {
		return -EINVAL;
	}

	if (sig_valid && hash_len_valid && hash_valid) {
		return load_segments(&segments, blob_offset);
	} else {
		return -EINVAL;
	}
}

