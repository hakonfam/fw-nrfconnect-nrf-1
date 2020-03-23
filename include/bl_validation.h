/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef BL_VALIDATION_H__
#define BL_VALIDATION_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <fw_info.h>

/** @defgroup bl_validation Bootloader firmware validation
 * @{
 */

struct __packed fw_validation_info {
	/* Magic value to verify that the struct has the correct type. */
	u32_t magic[MAGIC_LEN_WORDS];

	/* The address of the start (vector table) of the firmware. */
	u32_t address;

	/* The hash of the firmware.*/
	u8_t  hash[CONFIG_SB_HASH_LEN];

	/* Public key to be used for signature verification. This must be
	 * checked against a trusted hash.
	 */
	u8_t  public_key[CONFIG_SB_PUBLIC_KEY_LEN];

	/* Signature over the firmware as represented by the address and size in
	 * the firmware_info.
	 */
	u8_t  signature[CONFIG_SB_SIGNATURE_LEN];
};


/** Function for validating firmware.
 *
 * @details This will run a series of checks on the fw_info contents, then
 *          locate the validation info and check the signature of the image.
 *
 * @note If this function returns false, the fw_info struct can be invalidated
 *       by modifying its 'valid' field, causing any subsequent call to this
 *       function to quickly return false.
 *
 * @param[in] fw_address  Address the firmware is currently at.
 * @param[in] fwinfo      Firmware info structure belonging to the image.
 *
 * @retval  true   if the image is valid
 * @retval  false  if the image is invalid
 */
bool bl_validate_firmware(u32_t fw_dst_address, u32_t fw_src_address);

/* Typedef for use in EXT_API declaration */
typedef
bool (*bl_validate_firmware_t)(u32_t fw_dst_address, u32_t fw_src_address);

/** Whether bl_validate_firmware() is available.
 *
 * @details This is only relevant when CONFIG_BL_VALIDATE_FW_EXT_API_OPTIONAL is
 *          set.
 *
 * @retval true  bl_validate_firmware() can be called and should work correctly.
 * @retval false bl_validate_firmware() is unavailable and will always return
 *               false because the undelying EXT_API is unavailable.
 */
bool bl_validate_firmware_available(void);

/** Function for validating firmware in place.
 *
 * @note This function is only available to the bootloader.
 *
 * @details See @ref bl_validate_firmware for more details.
 */
bool bl_validate_firmware_local(u32_t fw_address,
				const struct fw_info *fwinfo);


/**
 * @brief Structure describing the BL_VALIDATE_FW EXT_API.
 */
struct bl_validate_fw_ext_api {
	bl_validate_firmware_t bl_validate_firmware;
};

/**
 * @brief Find the validation_info at the end of the firmware.
 */
const struct fw_validation_info *
bl_validation_info_find(u32_t start_address, u32_t search_distance);

  /** @} */

#ifdef __cplusplus
}
#endif

#endif /* BL_VALIDATION_H__ */
