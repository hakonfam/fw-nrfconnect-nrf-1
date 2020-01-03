/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file pcd.h
 *
 * @defgroup pcd Peripheral Core DFU
 * @{
 * @brief Driver for loading DFU files to peripheral core
 */

#ifndef PCD_H__
#define PCD_H__

#ifdef __cplusplus
extern "C" {
#endif

struct pcd_cmd {
	const void *const src;
	size_t src_len;
	u32_t dst_addr;
	u8_t buf[CONFIG_PCD_BUF_SIZE];
	u8_t hash[256/8];
};

int pcd_transfer_and_hash(struct pcd_cmd *config, const char *dev_name);

#endif /* PCD_H__ */

/**@} */
