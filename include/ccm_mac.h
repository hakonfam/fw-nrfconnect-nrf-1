/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef CCM_MAC_H_
#define CCM_MAC_H_

#include <stdint.h>
#include <string.h>
#include <zephyr/types.h>

/**
 * @file
 * @defgroup ccm_mac Calculate MAC using CCM
 * @{
 *
 * @brief API for calculating MAC using CCM peripheral.
 */

#ifdef __cplusplus
extern "C" {
#endif

struct ccm_mac_ctx {
	size_t total;
	u32_t mic;
	u8_t ibuf[254];
	u8_t obuf[254];
	u8_t scratch[254];
	u8_t ccm_data[26];
};

int ccm_mac_init(struct ccm_mac_ctx *ctx);

int ccm_mac_free(struct ccm_mac_ctx *ctx);

int ccm_mac_update(struct ccm_mac_ctx *ctx, const unsigned char *input, size_t ilen);

int ccm_mac_finish(struct ccm_mac_ctx *ctx, int *output);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* CCM_MAC_H_ */
