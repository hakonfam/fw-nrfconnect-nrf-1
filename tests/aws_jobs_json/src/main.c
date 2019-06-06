/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <ztest.h>
#include <aws_fota_json.h>

static void test_notify_next(void)
{
	char encoded[] = "{\"timestamp\":1559808907,\"execution\":{\"jobId\":\"9b5caac6-3e8a-45dd-9273-c1b995762f4a\",\"status\":\"QUEUED\",\"queuedAt\":1559808906,\"lastUpdatedAt\":1559808906,\"versionNumber\":1,\"executionNumber\":1,\"jobDocument\":{\"operation\":\"app_fw_update\",\"fwversion\":\"2\",\"size\":181124,\"location\":{\"protocol\":\"https:\",\"host\":\"fota-update-bucket.s3.eu-central-1.amazonaws.com\",\"path\":\"/update.bin?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=AKIAWXEL53DXIU7W72AE%2F20190606%2Feu-central-1%2Fs3%2Faws4_request&X-Amz-Date=20190606T081505Z&X-Amz-Expires=604800&X-Amz-Signature=913e00b97efe5565a901df4ff0b87e4878a406941d711f59d45915035989adcc&X-Amz-SignedHeaders=host\"}}}}";
	int ret;
	char buf1[100];
	char buf2[100];
	char buf3[100];

	ret = aws_fota_parse_notify_next_document(encoded, sizeof(encoded) - 1,
			buf1, buf2, buf3);

	zassert_equal(ret, 1,
		     "All fields decoded correctly");

}

static void test_timestamp_only(void)
{
	int ret;
	char buf1[100];
	char buf2[100];
	char buf3[100];
	char encoded[] = "{\"timestamp\":1559808907}";

	ret = aws_fota_parse_notify_next_document(encoded, sizeof(encoded) - 1,
			buf1, buf2, buf3);

	zassert_equal(ret, 1,
		     "All fields decoded correctly");
}

static void test_update_job_exec_rsp_minimal(void)
{
	char encoded[] = "{\"timestamp\":4096,\"clientToken\":\"token\"}";
	int ret;

	ret = aws_fota_parse_update_job_exec_state_rsp(encoded,
			sizeof(encoded) - 1);

	/* Only two last fields are set */
	zassert_equal(ret,0b1100, "All fields decoded correctly");
}

void test_main(void)
{
	ztest_test_suite(lib_json_test,
			 ztest_unit_test(test_update_job_exec_rsp_minimal),
			 ztest_unit_test(test_timestamp_only),
			 ztest_unit_test(test_notify_next)
			 );

	ztest_run_test_suite(lib_json_test);
}
