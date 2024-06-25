/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/sys/printk.h>

#include <psa/crypto.h>

static const uint8_t APPLICATION_PUBKEY_GEN0[] = {
#include "APPLICATION_PUBKEY_GEN0"
};
static const uint8_t APPLICATION_PUBKEY_GEN1[] = {
#include "APPLICATION_PUBKEY_GEN1"
};
static const uint8_t APPLICATION_PUBKEY_GEN2[] = {
#include "APPLICATION_PUBKEY_GEN2"
};
static const uint8_t APPLICATION_FWENC_GEN0[] = {
#include "APPLICATION_FWENC_GEN0"
};
static const uint8_t APPLICATION_FWENC_GEN1[] = {
#include "APPLICATION_FWENC_GEN1"
};
static const uint8_t RADIO_PUBKEY_GEN0[] = {
#include "RADIO_PUBKEY_GEN0"
};
static const uint8_t RADIO_PUBKEY_GEN1[] = {
#include "RADIO_PUBKEY_GEN1"
};
static const uint8_t RADIO_PUBKEY_GEN2[] = {
#include "RADIO_PUBKEY_GEN2"
};
static const uint8_t RADIO_FWENC_GEN0[] = {
#include "RADIO_FWENC_GEN0"
};
static const uint8_t RADIO_FWENC_GEN1[] = {
#include "RADIO_FWENC_GEN1"
};
static const uint8_t NONE_OEMPUBKEY_GEN0[] = {
#include "NONE_OEMPUBKEY_GEN0"
};
static const uint8_t NONE_OEMPUBKEY_GEN1[] = {
#include "NONE_OEMPUBKEY_GEN1"
};
static const uint8_t NONE_OEMPUBKEY_GEN2[] = {
#include "NONE_OEMPUBKEY_GEN2"
};

#define PSA_KEY_LOCATION_CRACEN ((psa_key_location_t)(0x800000 | ('N' << 8))) /* TODO */

#define USAGE_FWENC		       0x20
#define USAGE_PUBKEY		       0x21
#define USAGE_AUTHDEBUG		       0x23
#define USAGE_STMTRACE		       0x25
#define USAGE_COREDUMP		       0x26
#define USAGE_OEMPUBKEY 	       0xAA

#define DOMAIN_NONE	   0x00
#define DOMAIN_SECURE      0x01
#define DOMAIN_APPLICATION 0x02
#define DOMAIN_RADIO	   0x03
#define DOMAIN_CELL	   0x04
#define DOMAIN_ISIM	   0x05
#define DOMAIN_WIFI	   0x06
#define DOMAIN_SYSCTRL	   0x08

static void load_verify_key(mbedtls_svc_key_id_t key_id, const uint8_t *key, size_t key_len)
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

	psa_status_t status = psa_import_key(&key_attributes, key, key_len, &volatile_key_id);

	if (status == PSA_SUCCESS) {
		printk("ECC public key loaded 0x%x\n", volatile_key_id);
	} else {
		printk("SDFW builtin keys - psa_import_key failed, status %d\n", status);
	}
}

static void load_enc_key(mbedtls_svc_key_id_t key_id, const uint8_t *key, size_t key_len)
{
	mbedtls_svc_key_id_t volatile_key_id = 0;
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_set_key_id(&key_attributes, key_id);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&key_attributes, 256);
	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
				     PSA_KEY_LIFETIME_PERSISTENT, PSA_KEY_LOCATION_CRACEN));

	psa_status_t status = psa_import_key(&key_attributes, key, key_len, &volatile_key_id);

	if (status == PSA_SUCCESS) {
		printk("AES key loaded 0x%x\n", volatile_key_id);
	} else {
		printk("SDFW builtin keys - psa_import_key failed, status %d\n", status);
	}
}

#define PLATFORM_KEY_BASE (0x40000000)

#define PLATFORM_KEY_ID(domain, usage, gen)                                                        \
	(PLATFORM_KEY_BASE | (UTIL_EVAL(UTIL_CAT(DOMAIN##_, domain)) << 16) |                      \
	 (UTIL_EVAL(UTIL_CAT(USAGE##_, usage)) << 8) | gen)

#define LOAD_PUBKEY(domain, usage, gen)                                                            \
	do {                                                                                       \
		load_verify_key(                                                                   \
			UTIL_EVAL(PLATFORM_KEY_ID(domain, usage, gen)),                            \
			UTIL_CAT(UTIL_CAT(UTIL_CAT(domain##_, usage), _GEN), gen),                 \
			sizeof(UTIL_CAT(UTIL_CAT(UTIL_CAT(domain##_, usage), _GEN), gen)));        \
	} while (0)

#define LOAD_ENC_KEY(domain, usage, gen)                                                           \
	do {                                                                                       \
		load_enc_key(                                                                   \
			UTIL_EVAL(PLATFORM_KEY_ID(domain, usage, gen)),                            \
			UTIL_CAT(UTIL_CAT(UTIL_CAT(domain##_, usage), _GEN), gen),                 \
			sizeof(UTIL_CAT(UTIL_CAT(UTIL_CAT(domain##_, usage), _GEN), gen)));        \
	} while (0)

static int load_verify_keys(void)
{

	psa_status_t status = PSA_ERROR_GENERIC_ERROR;

	/* Initialize PSA Crypto */
	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		return -1;
	}

	LOAD_PUBKEY(APPLICATION, PUBKEY, 0);
	LOAD_PUBKEY(APPLICATION, PUBKEY, 1);
	LOAD_PUBKEY(APPLICATION, PUBKEY, 2);

	LOAD_ENC_KEY(APPLICATION, FWENC, 0);
	LOAD_ENC_KEY(APPLICATION, FWENC, 1);

	LOAD_PUBKEY(RADIO, PUBKEY, 0);
	LOAD_PUBKEY(RADIO, PUBKEY, 1);
	LOAD_PUBKEY(RADIO, PUBKEY, 2);

	LOAD_ENC_KEY(RADIO, FWENC, 0);
	LOAD_ENC_KEY(RADIO, FWENC, 1);

	LOAD_PUBKEY(NONE, OEMPUBKEY, 0);
	LOAD_PUBKEY(NONE, OEMPUBKEY, 1);
	LOAD_PUBKEY(NONE, OEMPUBKEY, 2);

	return 0;

}

static int load_keys(void)
{
	return load_verify_keys();
}

SYS_INIT(load_keys, APPLICATION, 99);
