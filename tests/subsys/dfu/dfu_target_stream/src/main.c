/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <string.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <ztest.h>
#include <dfu_target_stream.h>

#define FLASH_NAME DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL
#define FLASH_BASE (64*1024)
#define FLASH_SIZE DT_REG_SIZE(SOC_NV_FLASH_NODE)
#define FLASH_AVAILABLE (FLASH_SIZE-FLASH_BASE)

#define TEST_ID_1 "test_1"
#define TEST_ID_2 "test_2"

#define BUF_LEN 14000 /* Note, not page aligned */

static const struct device *fdev;
static uint8_t sbuf[128];
static uint8_t read_buf[BUF_LEN];
static uint8_t write_buf[BUF_LEN] = {[0 ... BUF_LEN - 1] = 0xaa};

static void test_dfu_target_stream(void)
{
	int err;
	size_t offset;

	/* Null checks */
	err = dfu_target_stream_init(NULL, fdev, sbuf, sizeof(sbuf),
				     FLASH_BASE, 0, NULL);
	zassert_true(err < 0, "Unexpected success");

	err = dfu_target_stream_init(TEST_ID_1, NULL, sbuf, sizeof(sbuf),
				     FLASH_BASE, 0, NULL);
	zassert_true(err < 0, "Unexpected success");

	err = dfu_target_stream_init(TEST_ID_1, fdev, NULL, sizeof(sbuf),
				     FLASH_BASE, 0, NULL);
	zassert_true(err < 0, "Unexpected success");

	/* Expected successful call */
	err = dfu_target_stream_init(TEST_ID_1, fdev, sbuf, sizeof(sbuf),
				     FLASH_BASE, 0, NULL);
	zassert_equal(err, 0, "Unexpected failure");

	/* Call _init again without calling "complete". This should result
	 * in an error since only one id is supported simultaneously
	 */
	err = dfu_target_stream_init(TEST_ID_2, fdev, sbuf, sizeof(sbuf),
				     FLASH_BASE, 0, NULL);
	zassert_true(err < 0, "Unexpected success");

	/* Perform write, and verify offset */
	err = dfu_target_stream_write(write_buf, sizeof(write_buf));
	zassert_equal(err, 0, "Unexpected failure");

	err = dfu_target_stream_offset_get(&offset);
	zassert_equal(err, 0, "Unexpected failure");

	/* Since 'write_buf' is not page aligned, the 'offset' should not
	 * correspond to the sice written.
	 */
	zassert_not_equal(offset, sizeof(write_buf), "Invalid offset");

	/* Complete transfer */
	err = dfu_target_stream_done(true);
	zassert_equal(err, 0, "Unexpected failure");

	/* Call _init again after calling "_done". This should NOT result
	 * in an error since the id should be reset, and the setting
	 * should be deleted.
	 */
	err = dfu_target_stream_init(TEST_ID_2, fdev, sbuf, sizeof(sbuf),
				     FLASH_BASE, 0, NULL);
	zassert_equal(err, 0, "Unexpected failure");

	/* Read out the data to ensure that it was written correctly */
	err = flash_read(fdev, FLASH_BASE, read_buf, BUF_LEN);
	zassert_equal(err, 0, "Unexpected failure");
	zassert_mem_equal(read_buf, write_buf, BUF_LEN, "Incorrect value");
	
}

static void test_dfu_target_stream_save_progress(void)
{
	int err;
	size_t first_offset;
	size_t second_offset;

	/* Reset state to avoid failure when initializing */
	err = dfu_target_stream_done(true);
	zassert_equal(err, 0, "Unexpected failure");

	err = dfu_target_stream_init(TEST_ID_1, fdev, sbuf, sizeof(sbuf),
				     FLASH_BASE, 0, NULL);
	zassert_equal(err, 0, "Unexpected failure");

	/* Perform write, and verify offset */
	err = dfu_target_stream_write(write_buf, sizeof(write_buf)/2);
	zassert_equal(err, 0, "Unexpected failure");
	
	err = dfu_target_stream_offset_get(&first_offset);
	zassert_equal(err, 0, "Unexpected failure");

	/* Complete transfer with failure */
	err = dfu_target_stream_done(false);
	zassert_equal(err, 0, "Unexpected failure");

	/* Re-initialize dfu target, don't perform any write operation, and
	 * verify that the offsets are the same.
	 */
	err = dfu_target_stream_init(TEST_ID_1, fdev, sbuf, sizeof(sbuf),
				     FLASH_BASE, 0, NULL);
	zassert_equal(err, 0, "Unexpected failure");

	err = dfu_target_stream_offset_get(&second_offset);
	zassert_equal(err, 0, "Unexpected failure");

	zassert_equal(first_offset, second_offset, "Offsets do not match");

	/* Complete transfer with success */
	err = dfu_target_stream_done(true);
	zassert_equal(err, 0, "Unexpected failure");

	/* Re-initialize dfu target, don't perform any write operation, and
	 * verify that the offset is now 0, since we had a succesfull 'done'.
	 */
	err = dfu_target_stream_init(TEST_ID_1, fdev, sbuf, sizeof(sbuf),
				     FLASH_BASE, 0, NULL);
	zassert_equal(err, 0, "Unexpected failure");

	err = dfu_target_stream_offset_get(&second_offset);
	zassert_equal(err, 0, "Unexpected failure");
	zassert_equal(0, second_offset, "Offsets do not match");

	/* Next we ensure that changing the target results in the offset
	 * being reset
	 */

	/* Complete transfer with success */
	err = dfu_target_stream_done(true);
	zassert_equal(err, 0, "Unexpected failure");

	/* Re-initialize dfu target, for 'TEST_ID_1' */
	err = dfu_target_stream_init(TEST_ID_1, fdev, sbuf, sizeof(sbuf),
				     FLASH_BASE, 0, NULL);
	zassert_equal(err, 0, "Unexpected failure");

	/* Verify that offset is 0 */
	err = dfu_target_stream_offset_get(&first_offset);
	zassert_equal(err, 0, "Unexpected failure");
	zassert_equal(0, first_offset, "Offsets do not match");

	/* Perform write, and verify that offset has changed*/
	err = dfu_target_stream_write(write_buf, sizeof(write_buf));
	zassert_equal(err, 0, "Unexpected failure");

	/* Verify that the offset is not 0 anymore */
	err = dfu_target_stream_offset_get(&second_offset);
	zassert_equal(err, 0, "Unexpected failure");
	zassert_not_equal(second_offset, first_offset, "Offset hasn't updated");

	/* Complete transfer with failure, this retains the non-0 offset */
	err = dfu_target_stream_done(false);
	zassert_equal(err, 0, "Unexpected failure");

	/* Re-initialize dfu target, for 'TEST_ID_2', verify that the offset
	 * loaded is 0 even though it was just retained an non-0 for TEST_ID_1
	 */
	err = dfu_target_stream_init(TEST_ID_2, fdev, sbuf, sizeof(sbuf),
				     FLASH_BASE, 0, NULL);
	zassert_equal(err, 0, "Unexpected failure");

	err = dfu_target_stream_offset_get(&first_offset);
	zassert_equal(err, 0, "Unexpected failure");
	zassert_equal(0, first_offset, "Offsets has not been reset");
}

void test_main(void)
{
	fdev = device_get_binding(FLASH_NAME);
	ztest_test_suite(lib_dfu_target_stream,
	     ztest_unit_test(test_dfu_target_stream),
	     ztest_unit_test(test_dfu_target_stream_save_progress)
	 );

	ztest_run_test_suite(lib_dfu_target_stream);
}
