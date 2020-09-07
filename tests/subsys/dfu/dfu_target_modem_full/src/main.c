/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <string.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <ztest.h>
#include <dfu_target_modem_full.h>

static void test_something1(void)
{
}

static void test_something2(void)
{
}

void test_main(void)
{
	ztest_test_suite(lib_dfu_target_modem_full,
	     ztest_unit_test(test_something2),
	     ztest_unit_test(test_something1)
	 );

	ztest_run_test_suite(lib_dfu_target_modem_full);
}
