/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <sys/util.h>
#include <errno.h>
#include <string.h>

#include <hal/nrf_ccm.h>
#include <ccm_mac.h>

int ccm_mac_init(struct ccm_mac_ctx *ctx)
{
	if (ctx == NULL) {
		return -EINVAL;
	}

	memset(ctx, 0, sizeof(*ctx));

	nrf_ccm_enable(NRF_CCM);

	nrf_ccm_config_t cfg = {
		.mode = NRF_CCM_MODE_ENCRYPTION,
		.datarate = NRF_CCM_DATARATE_500K,
		.length = 251
	};

	nrf_ccm_configure(NRF_CCM, &cfg);

	nrf_ccm_inptr_set(NRF_CCM, (u32_t*)ctx->ibuf);
	nrf_ccm_outptr_set(NRF_CCM, (u32_t*)ctx->obuf);

	nrf_ccm_maxpacketsize_set(NRF_CCM, 251);
	nrf_ccm_cnfptr_set(NRF_CCM, (u32_t *)ctx->ccm_data);

	return 0;
}

int ccm_mac_free(struct ccm_mac_ctx *ctx)
{
	return 0;
}

int ccm_mac_update(struct ccm_mac_ctx *ctx, const unsigned char *input,
		   size_t ilen)
{
	return 0;
}

int ccm_mac_finish(struct ccm_mac_ctx *ctx, int *output)
{
	return 0;
}
