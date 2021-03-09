/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <init.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <zephyr.h>
#include <drivers/entropy.h>
#include <sys/util.h>

#if defined(CONFIG_SPM)
#include "secure_services.h"
#elif defined(CONFIG_BUILD_WITH_TFM)
#include <psa/crypto.h>
#include <psa/crypto_extra.h>
#include <tfm_ns_interface.h>
#else
#include "nrf_cc3xx_platform_entropy.h"
#endif

#define CTR_DRBG_MAX_REQUEST 1024

static int entropy_cc3xx_rng_get_entropy(
	const struct device *dev,
	uint8_t *buffer,
	uint16_t length)
{
	int res = -EINVAL;

	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(buffer != NULL);

#if defined(CONFIG_BUILD_WITH_TFM)
	res = psa_generate_random(buffer, length);
	if (res != PSA_SUCCESS) {
		return -EINVAL;
	}

#elif (CONFIG_SPM)
	size_t olen;

	res = spm_request_random_number(buffer,
						length,
						&olen);

	if (olen != length) {
		return -EINVAL;
	}
#else
	size_t olen;

	res = nrf_cc3xx_platform_entropy_get(
						buffer,
						length,
						&olen);

	if (olen != length) {
		return -EINVAL;
	}
#endif



	return res;
}

static int entropy_cc3xx_rng_init(const struct device *dev)
{
	(void)dev;

	#if defined(CONFIG_BUILD_WITH_TFM)
		int ret = -1;
		enum tfm_status_e tfm_status;

		tfm_status = tfm_ns_interface_init();
		if (tfm_status != TFM_SUCCESS) {
			return -EINVAL;
		}

		ret = psa_crypto_init();
		if (ret != PSA_SUCCESS) {
			return -EINVAL;
		}
	#endif

	return 0;
}

static const struct entropy_driver_api entropy_cc3xx_rng_api = {
	.get_entropy = entropy_cc3xx_rng_get_entropy
};

#if DT_NODE_HAS_STATUS(DT_NODELABEL(cryptocell), okay)
#define CRYPTOCELL_NODE_ID DT_NODELABEL(cryptocell)
#elif DT_NODE_HAS_STATUS(DT_NODELABEL(cryptocell_sw), okay)
#define CRYPTOCELL_NODE_ID DT_NODELABEL(cryptocell_sw)
#else
/*
 * TODO is there a better way to handle this?
 *
 * The problem is that when this driver is configured for use by
 * non-secure applications, calling through SPM leaves our application
 * devicetree without an actual cryptocell node, so we fall back on
 * cryptocell_sw. This works, but it's a bit hacky and requires an out
 * of tree zephyr patch.
 */
#error "No cryptocell or cryptocell_sw node labels in the devicetree"
#endif

DEVICE_DT_DEFINE(CRYPTOCELL_NODE_ID, entropy_cc3xx_rng_init,
		 device_pm_control_nop, NULL, NULL, PRE_KERNEL_1,
		 CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &entropy_cc3xx_rng_api);
