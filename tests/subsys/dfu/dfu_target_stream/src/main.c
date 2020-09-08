/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
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

static struct device *fdev;
static uint8_t buf[1024];
static uint8_t write_buf[1024];

static void test_dfu_target_stream(void)
{
	/*
	 * To test:
	 *  - Parameter checks
	 *  - Complete write which is not aligned.
	 *  - Start update, cancel it, start different update
	 *  - Start update, finish it, start different update
	 *  - Start update, start other update, verify failure on second,
	 *    and that the first is able to finish.
	 */
	int err;
	size_t offset;

	/* Null checks */
	err = dfu_target_stream_init(NULL, fdev, buf, sizeof(buf),
				     FLASH_BASE, 0, NULL);
	zassert_true(err < 0, "Unexpected success");

	err = dfu_target_stream_init(TEST_ID_1, NULL, buf, sizeof(buf),
				     FLASH_BASE, 0, NULL);
	zassert_true(err < 0, "Unexpected success");

	err = dfu_target_stream_init(TEST_ID_1, fdev, NULL, sizeof(buf),
				     FLASH_BASE, 0, NULL);
	zassert_true(err < 0, "Unexpected success");

	/* Expected successful call */
	err = dfu_target_stream_init(TEST_ID_1, fdev, buf, sizeof(buf),
				     FLASH_BASE, 0, NULL);
	zassert_equal(err, 0, "Unexpected failure");

	/* Call _init again without calling "complete". This should result
	 * in an error since only one id is supported simultaneously
	 */
	err = dfu_target_stream_init(TEST_ID_2, fdev, buf, sizeof(buf),
				     FLASH_BASE, 0, NULL);
	zassert_true(err < 0, "Unexpected success");

	/* Perform write, and verify offset */
	err = dfu_target_stream_write(write_buf, sizeof(write_buf));
	zassert_equal(err, 0, "Unexpected failure");

	err = dfu_target_stream_offset_get(&offset);
	zassert_equal(err, 0, "Unexpected failure");

	zassert_equal(offset, sizeof(write_buf), "Invalid offset");

	/* Complete transfer */
	err = dfu_target_stream_done(true);
	zassert_equal(err, 0, "Unexpected failure");

	/* Call _init again after calling "_done". This should NOT result
	 * in an error since the id should be reset.
	 */
	err = dfu_target_stream_init(TEST_ID_2, fdev, buf, sizeof(buf),
				     FLASH_BASE, 0, NULL);
	zassert_equal(err, 0, "Unexpected failure");

}

#ifdef CONFIG_DFU_TARGET_STREAM_SAVE_PROGRESS
static void test_dfu_target_stream_save_progress(void)
{
}

#else
static void test_dfu_target_stream_save_progress(void)
{
	ztest_test_skip();
}


#endif /* CONFIG_DFU_TARGET_STREAM_SAVE_PROGRESS */

void test_main(void)
{
	fdev = device_get_binding(FLASH_NAME);
	ztest_test_suite(lib_dfu_target_stream,
	     ztest_unit_test(test_dfu_target_stream),
	     ztest_unit_test(test_dfu_target_stream_save_progress)
	 );

	ztest_run_test_suite(lib_dfu_target_stream);
}
