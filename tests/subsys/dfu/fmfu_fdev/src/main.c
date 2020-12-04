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

static void test_fmfu_fdev_prevalidate(void)
{
	zassert_true(true, NULL);

}

void test_main(void)
{
	ztest_test_suite(lib_fmfu_fdev_test,
	     ztest_unit_test(test_fmfu_fdev_prevalidate)
	 );

	ztest_run_test_suite(lib_fmfu_fdev_test);
}
