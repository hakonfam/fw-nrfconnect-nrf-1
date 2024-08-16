/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/sys/printk.h>

#include <psa/crypto.h>

static const uint8_t app_gen0[] = {
#include "MANIFEST_PUBKEY_APPLICATION_GEN0"
};

#define MANIFEST_PUBKEY_APP_GEN0 0x40022100
#define PSA_KEY_LOCATION_CRACEN ((psa_key_location_t)(0x800000 | ('N' << 8))) /* TODO */

static int load_verify_key(mbedtls_svc_key_id_t key_id, const uint8_t *key, size_t key_len)
{
	mbedtls_svc_key_id_t volatile_key_id = 0;
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_TWISTED_EDWARDS));
	psa_set_key_algorithm(&key_attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_512));
	psa_set_key_bits(&key_attributes, 255);
	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_VERIFY_MESSAGE | PSA_KEY_USAGE_VERIFY_HASH);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
				     PSA_KEY_LIFETIME_PERSISTENT, PSA_KEY_LOCATION_CRACEN));

	psa_set_key_id(&key_attributes, key_id);

	printk("attributes %p\n", &key_attributes);
	printk("usage address%p\n",&key_attributes.MBEDTLS_PRIVATE(core).MBEDTLS_PRIVATE(policy).MBEDTLS_PRIVATE(usage));
	printk("Usage %x\n", key_attributes.MBEDTLS_PRIVATE(core).MBEDTLS_PRIVATE(policy).MBEDTLS_PRIVATE(usage));
	psa_status_t status = psa_import_key(&key_attributes, key, key_len, &volatile_key_id);

	if (status == PSA_SUCCESS) {
		printk("Key loaded 0x%x\n", volatile_key_id);
		return 0;
	} else {
		printk("SDFW builtin keys - psa_import_key failed, status %d", status);
		return -1;
	}
}

static int load_verify_keys(void)
{

	psa_status_t status = PSA_ERROR_GENERIC_ERROR;

	/* Initialize PSA Crypto */
	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		return -1;
	}

	/* Known to work, but don't we need to set the lifetime as well?
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_set_key_id(&key_attributes, mbedtls_svc_key_id_make(0, 0x4000AA00));
	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_VERIFY_HASH);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1));
	*/

	return load_verify_key(MANIFEST_PUBKEY_APP_GEN0, app_gen0, sizeof(app_gen0));

}

static void log_keys(void)
{
	for(int i = 0; i < sizeof(app_gen0); i++) {
		printk("%x", app_gen0[i]);
	}

	printk("\n");

}

static int load_keys(void)
{
	log_keys();

	return load_verify_keys();
}

SYS_INIT(load_keys, APPLICATION, 99);
