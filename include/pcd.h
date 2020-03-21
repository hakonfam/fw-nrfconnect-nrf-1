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

#define PCD_CMD_MAGIC 0xb5b4b3b6

struct pcd_cmd {
	u32_t magic;
	const void *src;
	size_t len; /* Size of image to copy in bytes */
	void *dst;
	u8_t buf[CONFIG_PCD_BUF_SIZE];
	u8_t hash[256/8]; /* TODO CONFIG_SB_HASH_LEN? */
};

int pcd_transfer_and_hash(struct pcd_cmd *cmd, struct device * fdev);

#endif /* PCD_H__ */

/**@} */
