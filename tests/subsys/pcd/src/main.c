/* Copyright (c) 2020 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <ztest.h>
#include <string.h>
#include <stdbool.h>
#include <zephyr/types.h>
#include <devicetree.h>
#include <drivers/flash.h>
#include <dfu/pcd.h>

#define FLASH_NAME DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL

#define BUF_LEN ((32*1024) + 10) /* Some pages, and then some */

#define WRITE_OFFSET 0x80000

#define CMD_WRITE(cmd)                                          \
	(zassert_equal(0,                                       \
		       pcd_cmd_write(&cmd, write_buf_offset,    \
				     (void *)0xf00ba17, 10, 0), \
		       "unexpected failure"))


/* Only 'a' */
static const uint8_t data[BUF_LEN] = {[0 ... BUF_LEN - 1] = 'a'};
static uint8_t read_buf[sizeof(data)];
static uint8_t write_buf[100];
static off_t write_buf_offset = (off_t)write_buf;

static void test_pcd_cmd_read(void)
{
	struct pcd_cmd *cmd = NULL;
	struct pcd_cmd *cmd_2 = NULL;
	int err;

	err = pcd_cmd_read(&cmd, write_buf_offset);
	zassert_true(err < 0, "should return error");

	/* Performs valid write */
	CMD_WRITE(cmd);

	err = pcd_cmd_read(&cmd_2, write_buf_offset);
	zassert_true(err >= 0, "should not return error");

	zassert_equal_ptr(cmd, cmd_2, "commands are not equal");
}

static void test_pcd_cmd_write(void)
{
	struct pcd_cmd *cmd = NULL;
	int err;

	/* Null checks */
	err = pcd_cmd_write(&cmd, write_buf_offset, (const void *)data,
			    0, 42);
	zassert_true(err < 0, "should return error");

	err = pcd_cmd_write(&cmd, write_buf_offset, NULL, sizeof(data),
			    WRITE_OFFSET);
	zassert_true(err < 0, "should return error");

	/* Valid call */
	CMD_WRITE(cmd);
	zassert_not_equal(cmd, NULL, "should not be NULL");
}

static void test_pcd_invalidate(void)
{
	int err;
	struct pcd_cmd *cmd = NULL;

	err = pcd_cmd_invalidate(NULL);
	zassert_true(err < 0, "Unexpected success");

	CMD_WRITE(cmd);
	zassert_not_equal(cmd, NULL, "should not be NULL");

	err = pcd_cmd_status_get(cmd);
	zassert_true(err >= 0, "should return non-negative int");

	err = pcd_cmd_invalidate(cmd);
	zassert_equal(err, 0, "Unexpected failure");

	err = pcd_cmd_status_get(cmd);
	zassert_true(err < 0, "should return negative int");
}


static void test_pcd_fw_copy(void)
{
	int err;
	struct pcd_cmd *cmd = NULL;
	struct device *fdev;

	err = pcd_cmd_write(&cmd, write_buf_offset, (const void *)data,
			    sizeof(data), WRITE_OFFSET);
	zassert_true(cmd >= 0, "Unexpected failure");

	fdev = device_get_binding(FLASH_NAME);
	zassert_true(fdev != NULL, "fdev is NULL");

	err = pcd_cmd_status_get(cmd);
	zassert_equal(err, 0, "pcd_cmd_status_get should be zero when not complete");

	/* Null check */
	err = pcd_fw_copy(NULL, fdev);
	zassert_not_equal(err, 0, "Unexpected success");

	err = pcd_fw_copy(cmd, NULL);
	zassert_not_equal(err, 0, "Unexpected success");

	/* Valid fetch */
	err = pcd_fw_copy(cmd, fdev);
	zassert_equal(err, 0, "Unexpected failure");

	err = pcd_cmd_status_get(cmd);
	zassert_true(err > 0, "pcd_cmd_status_get should return positive int");

	err = flash_read(fdev, WRITE_OFFSET, read_buf, sizeof(data));
	zassert_equal(err, 0, "Unexpected failure");

	zassert_true(memcmp((const void *)data, (const void *)read_buf,
			    sizeof(data)) == 0, "neq");
}

static void test_pcd_cmd_status(void)
{
	int err;
	struct pcd_cmd *cmd = NULL;

	CMD_WRITE(cmd);

	err = pcd_cmd_status_get(NULL);
	zassert_true(err < 0, "Unexpected success");

	err = pcd_cmd_status_get(cmd);
	zassert_true(err >= 0, "Unexpected failure");
}

void test_main(void)
{
	ztest_test_suite(pcd_test,
			 ztest_unit_test(test_pcd_cmd_read),
			 ztest_unit_test(test_pcd_cmd_write),
			 ztest_unit_test(test_pcd_invalidate),
			 ztest_unit_test(test_pcd_fw_copy),
			 ztest_unit_test(test_pcd_cmd_status)
			 );

	ztest_run_test_suite(pcd_test);
}
