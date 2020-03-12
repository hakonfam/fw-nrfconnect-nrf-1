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

#include <device.h>

#define PCD_CMD_MAGIC_COPY 0xb5b4b3b6
#define PCD_CMD_MAGIC_FAIL 0x25bafc15

struct pcd_cmd {
	u32_t magic;
	const void *src;
	size_t len;
	size_t offset;
	u8_t buf[CONFIG_PCD_BUF_SIZE];
};

struct pcd_cmd *pcd_get_cmd(void *addr);

int pcd_invalidate(struct pcd_cmd *cmd);

int pcd_transfer(struct pcd_cmd *cmd, struct device * fdev);

#endif /* PCD_H__ */

/**@} */
