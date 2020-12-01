/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

int update_sample_init(void (*finished_cb)(void), const char *(*host_get)(void),
		       const char *(file_get)(void), bool (*two_leds)(void));
