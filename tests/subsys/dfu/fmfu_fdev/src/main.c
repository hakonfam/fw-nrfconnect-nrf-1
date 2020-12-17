/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <string.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <ztest.h>
#include <device.h>
#include <drivers/flash.h>
#include <dfu/fmfu_fdev.h>

#define FLASH_NAME DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL

static const struct device *fdev;
static uint8_t buf[128];

static void test_fmfu_fdev_load_null(void)
{
	int err;

	err = fmfu_fdev_load(NULL, sizeof(buf), fdev, 0);
	zassert_true(err < 0, "Expected negative error code for NULL argument");

	err = fmfu_fdev_load(buf, sizeof(buf), NULL, 0);
	zassert_true(err < 0, "Expected negative error code for NULL argument");
}

static void test_fmfu_fdev_load_invalid_offset(void)
{
	int err;

	err = fmfu_fdev_load(buf, sizeof(buf), fdev, 0x12345678);
	zassert_true(err < 0,
		     "Expected negative error code for invalid offset");

	err = fmfu_fdev_load(buf, sizeof(buf), fdev, 0x10000);
	zassert_true(err < 0,
		     "Expected negative error code for invalid offset");
}

void test_main(void)
{
	fdev = device_get_binding(FLASH_NAME);

	ztest_test_suite(lib_fmfu_fdev_test,
	     ztest_unit_test(test_fmfu_fdev_load_null),
	     ztest_unit_test(test_fmfu_fdev_load_invalid_offset)
	 );

	ztest_run_test_suite(lib_fmfu_fdev_test);
}
