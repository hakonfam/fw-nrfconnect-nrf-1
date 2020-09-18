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

#define FLASH_NAME DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL

#define BUF_LEN ((32*1024) + 10) /* Some pages, and then some */

#define WRITE_OFFSET 0x80000

/* Only 'a' */
static const uint8_t data[BUF_LEN] = {[0 ... BUF_LEN - 1] = 'a'};
static uint8_t read_buf[sizeof(data)];
static uint8_t write_buf[100];
static off_t write_buf_offset = (off_t)write_buf;

static void test_pcd_invalidate(void)
{
}


static void test_pcd_fw_copy(void)
{
}

static void test_pcd_fw_copy_status_get(void)
{
}

void test_main(void)
{
	ztest_test_suite(pcd_test,
			 ztest_unit_test(test_pcd_invalidate),
			 ztest_unit_test(test_pcd_fw_copy),
			 ztest_unit_test(test_pcd_fw_copy_status_get)
			 );

	ztest_run_test_suite(pcd_test);
}
