#include <ztest.h>
#include <stdio.h>
#include <string.h>
#include <kernel.h>
#include <ccm_mac.h>

static void test_something(void)
{
	struct ccm_mac_ctx ctx;
	int rc = ccm_mac_init(&ctx);
	zassert_equal(rc, 0, NULL);
}

void test_main(void)
{
	ztest_test_suite(ccm_mac,
			ztest_unit_test(test_something)
			);

	ztest_run_test_suite(ccm_mac);
}
