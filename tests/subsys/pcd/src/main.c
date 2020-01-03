/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <ztest.h>
#include <string.h>
#include <stdbool.h>
#include <zephyr/types.h>
#include <devicetree.h>
#include <drivers/flash.h>
#include <pcd.h>

#define FLASH_NAME DT_FLASH_DEV_NAME

#define FLASH_BASE (64*1024)

#define BUF_LEN ((32*1024) + 10) /* Some pages, and then some */

/* Only 'a' */
static const u8_t data[BUF_LEN] = {[0 ... BUF_LEN - 1] = 'a'};

/* As per https://emn178.github.io/online-tools/sha256.html, only reversed */
static u32_t digest[] = {
	0x6ae35194, 0x3f16b239, 0x2cf0171b, 0x43fff6e7,
	0x37db49b4, 0xd2a9a249, 0x9e1c3e2c, 0xa7d7b026
};

static void test_pcd_transfer_and_hash(void)
{
	int rc;

	struct pcd_cmd config = {.src = &data[0],
				.src_len = sizeof(data),
				.dst_addr = FLASH_BASE};

	rc = pcd_transfer_and_hash(&config, FLASH_NAME);
	zassert_equal(rc, 0, "Unexpected failure");

	zassert_mem_equal(config.hash, digest, sizeof(digest), "wrong hash");
}

void test_main(void)
{
	ztest_test_suite(pcd_test,
			 ztest_unit_test(test_pcd_transfer_and_hash)
			 );

	ztest_run_test_suite(pcd_test);
}
