/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/*
 * Generated with cddl_gen.py (https://github.com/oyvindronningstad/cddl_gen)
 * Generated with a default_maxq of 16
 */

#ifndef MODEM_UPDATE_TYPES_H__
#define MODEM_UPDATE_TYPES_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "cbor_decode.h"

#define DEFAULT_MAXQ 16

typedef struct {
	uint32_t _Segment_target_addr;
	uint32_t _Segment_len;
} Segment_t;

typedef struct {
	Segment_t _Segments__Segment[16];
	size_t _Segments__Segment_count;
} Segments_t;

typedef struct {
} header_map_t;

typedef struct {
	cbor_string_type_t _Sig_structure1_body_protected;
	header_map_t _Sig_structure1_body_protected_cbor;
	cbor_string_type_t _Sig_structure1_payload;
} Sig_structure1_t;

typedef struct {
	uint32_t _Manifest_version;
	uint32_t _Manifest_compat;
	cbor_string_type_t _Manifest_blob_hash;
	cbor_string_type_t _Manifest_segments;
} Manifest_t;

typedef struct {
	cbor_string_type_t _COSE_Sign1_Manifest_protected;
	header_map_t _COSE_Sign1_Manifest_protected_cbor;
	cbor_string_type_t _COSE_Sign1_Manifest_payload;
	Manifest_t _COSE_Sign1_Manifest_payload_cbor;
	cbor_string_type_t _COSE_Sign1_Manifest_signature;
} COSE_Sign1_Manifest_t;

#endif /* MODEM_UPDATE_TYPES_H__ */
