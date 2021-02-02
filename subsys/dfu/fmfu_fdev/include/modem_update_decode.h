/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/*
 * Generated with cddl_gen.py (https://github.com/oyvindronningstad/cddl_gen)
 * Generated with a default_maxq of 128
 */

#ifndef MODEM_UPDATE_DECODE_H__
#define MODEM_UPDATE_DECODE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "cbor_decode.h"
#include "modem_update_types.h"

#if DEFAULT_MAXQ != 128
#error "The type file was generated with a different default_maxq than this file"
#endif

bool cbor_decode_Wrapper(const uint8_t *payload, size_t payload_len,
			 COSE_Sign1_Manifest_t *result,
			 size_t *payload_len_out);

bool cbor_decode_Sig_structure1(const uint8_t *payload, size_t payload_len,
				Sig_structure1_t *result,
				size_t *payload_len_out);

bool cbor_decode_Segments(const uint8_t *payload, size_t payload_len,
			  Segments_t *result, size_t *payload_len_out);

#endif /* MODEM_UPDATE_DECODE_H__ */
