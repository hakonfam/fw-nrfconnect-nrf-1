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
#include <dfu/pcd.h>

#define FLASH_NAME DT_FLASH_DEV_NAME

#define FLASH_BASE (64*1024)

#define BUF_LEN ((32*1024) + 10) /* Some pages, and then some */

/* Only 'a' */
static const u8_t data[BUF_LEN] = {[0 ... BUF_LEN - 1] = 'a'};

static const u8_t zero[256] = {0};

/* As per https://emn178.github.io/online-tools/sha256.html, only reversed */
static u32_t digest[] = {
	0x6ae35194, 0x3f16b239, 0x2cf0171b, 0x43fff6e7,
	0x37db49b4, 0xd2a9a249, 0x9e1c3e2c, 0xa7d7b026
};

static void test_pcd_get_cmd(void)
{
	u32_t buf;

	buf = 0;
	struct pcd_cmd *cmd = pcd_get_cmd(&buf);
	zassert_equal(cmd, NULL, "should be NULL");

	buf = PCD_CMD_MAGIC;
	cmd = pcd_get_cmd(&buf);
	zassert_not_equal(cmd, NULL, "should not be NULL");
}

static void test_pcd_validate(void)
{
	bool ret;
	struct pcd_cmd cmd;
	u8_t hbuf[sizeof(cmd.hash)];

	/* Make the hashes differ by one byte */
	memset(cmd.hash, 0xaa, sizeof(cmd.hash));
	memset(hbuf, 0xaa, sizeof(cmd.hash));
	hbuf[0] = 0;
	ret = pcd_validate(&cmd, hbuf);
	zassert_false(ret, "should return false since not equal");
	zassert_equal(0, memcmp(cmd.hash, zero, sizeof(cmd.hash)),
		      "hash in cmd should be set to 0 after failed validate");

	/* Now make them equal */
	memset(cmd.hash, 0xaa, sizeof(cmd.hash));
	memset(hbuf, 0xaa, sizeof(cmd.hash));
	ret = pcd_validate(&cmd, hbuf);
	zassert_true(ret, "should return true since equal");
	zassert_equal(0, memcmp(cmd.hash, hbuf, sizeof(cmd.hash)),
		      "hash in cmd should not change after equality");


}

static void test_pcd_transfer_and_hash(void)
{
	int rc;

	struct pcd_cmd cmd = {.src = (const void *)&data[0],
				 .len = sizeof(data),
				 .offset = 0x8000};

	struct device *fdev = device_get_binding(FLASH_NAME);
	zassert_true(fdev != NULL, "fdev is NULL");
	rc = pcd_transfer_and_hash(&cmd, fdev);
	zassert_equal(rc, 0, "Unexpected failure");

	zassert_mem_equal(cmd.hash, digest, sizeof(digest), "wrong hash");
}

void test_main(void)
{
	ztest_test_suite(pcd_test,
			 ztest_unit_test(test_pcd_get_cmd),
			 ztest_unit_test(test_pcd_validate),
			 ztest_unit_test(test_pcd_transfer_and_hash)
			 );

	ztest_run_test_suite(pcd_test);
}
