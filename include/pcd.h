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
	u8_t *src;
	size_t src_len;
	u8_t area_id;
	u8_t hash[256/8];
};

int pcd_transfer_and_hash(struct pcd_cmd *cfg);

#endif /* PCD_H__ */

/**@} */
