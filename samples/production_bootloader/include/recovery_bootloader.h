/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**@file
 *
 * @defgroup recovery_bl_protocol Recovery Bootloader(R-BL) protocol
 * @{
 * @ingroup  sdk_recovery_bl
 * @brief    Recovery bootloader commands processing.
 *
 */
 
#ifndef RECOVERY_BOOTLOADER_H__
#define RECOVERY_BOOTLOADER_H__

#include <stdbool.h>
#include <kernel.h>

#define RECOVERY_BL_PROTOCOL_VERSION 1                          /**< Recovery Bootloader protocol version. */

/* Common platform dependent defines. */
#define PAGE_SIZE                 (4096)                        /**< Flash page size. */
/* TODO Get this from DTS */
#define FLASH_SIZE                0x100000 /**< Flash size in bytes. */
#define BOOTLOADER_START_ADDRESS  (FLASH_SIZE - PAGE_SIZE)      /**< Address of the bootloader(last flash page). */
#define BOOTLOADER_SIZE           (PAGE_SIZE)                   /**< Size of the bootloader in bytes. */
#define MBR_START_ADDRESS         0                             /**< Address of the MBR(first flash page). */
#define MBR_SIZE                  (PAGE_SIZE)                   /**< Size of the MBR in bytes. */
#define UICR_SIZE                 (sizeof(NRF_UICR_Type))       /**< UICR registers size. */

/* Transport configuration defines. */
#define RECOVERY_BL_TRANSPORT_MTU 4200                          /**< Max packet size(before encoding to the SLIP). */

/**
 * @brief Recovery Bootloader commands numbers.
 */
typedef enum {
    RECOVERY_BL_CMD_RESPONSE,            /**< Response message ID. */
    RECOVERY_BL_CMD_DATA_WRITE,          /**< Write data to flash. */
    RECOVERY_BL_CMD_DATA_READ,           /**< Read data from flash. */
    RECOVERY_BL_CMD_PAGE_ERASE,          /**< Erase flash page. */
    RECOVERY_BL_CMD_CRC_CHECK,           /**< Compute CRC of given flash area. */
    RECOVERY_BL_CMD_RESET,               /**< Reset device. */
    RECOVERY_BL_CMD_UICR_WRITE,          /**< Write data to UICR registers. */
    RECOVERY_BL_CMD_UICR_ERASE,          /**< Erase UICR page. */
    RECOVERY_BL_CMD_UICR_READ,           /**< Read data from UICR. */
    RECOVERY_BL_CMD_VERSION_GET,         /**< Request protocol version. */
    RECOVERY_BL_CMD_DEVICE_INFO_GET,     /**< Request info about flash and UICR size. */
    RECOVERY_BL_CMD_ERASE_ALL,           /**< Erase all available flash and UICR. */
    RECOVERY_BL_CMD_COUNT,               /**< Number of available commands. */
} recovery_bl_cmd_t;

/**
 * @brief Recovery Bootloader response result codes.
 */
typedef enum {
    RECOVERY_BL_ACK,                     /**< Command executed properly. */
    RECOVERY_BL_NACK_INVALID_CRC,        /**< Received packet CRC is invalid. */
    RECOVERY_BL_NACK_INVALID_CMD,        /**< Requested command is not supported. */
    RECOVERY_BL_NACK_INVALID_MSG_SIZE,   /**< Request or response message is longer than MTU. */
    RECOVERY_BL_NACK_PAGE_PROTECTED,     /**< Reqested address is protected against erase and write. */
    RECOVERY_BL_NACK_ADDRESS_NOT_ERASED, /**< Write address is not erased. */
    RECOVERY_BL_NACK_DATA_NOT_ALIGNED,   /**< Write data and address are not word aligned. */
    RECOVERY_BL_NACK_INVALID_ADDRESS,    /**< Given address exceeds flash address space. */
    RECOVERY_BL_NACK_APPROTECT_ON,       /**< Operation is invalid when application is protected. */
    RECOVERY_BL_NACK_COUNT,              /**< Number of available result codes. */
} recovery_bl_response_result_t;

/**
 * @brief Request message header.
 */
typedef struct __packed {
    uint8_t sequence_no;                 /**< Packet sequence number. */
    uint8_t command;                     /**< Request command. */
} recovery_bl_msg_req_hdr_t;

/**
 * @brief Response message header.
 */
typedef struct __packed {
    uint8_t sequence_no;                 /**< Packet sequence number. */
    uint8_t command;                     /**< Response command. */
    uint8_t req_command;                 /**< Requested command. */
    uint8_t result;                      /**< Result of request execution. */
} recovery_bl_msg_resp_hdr_t;

/**
 * @brief @ref RECOVERY_BL_CMD_DATA_WRITE request command arguments.
 */
typedef struct __packed {
    uint32_t address;                    /**< Start address of write area. */
    uint32_t length;                     /**< Length of data to be written. */
    uint8_t  data[1];                    /**< Variable length table of data to be stored in flash. */
} recovery_bl_cmd_data_write_req_t;

/**
 * @brief @ref RECOVERY_BL_CMD_DATA_READ request command arguments.
 */
typedef struct __packed {
    uint32_t address;                    /**< Start address of read area. */
    uint32_t length;                     /**< Length of data to be read. */
} recovery_bl_cmd_data_read_req_t;

/**
 * @brief @ref RECOVERY_BL_CMD_DATA_READ response command arguments.
 */
typedef struct __packed {
    uint32_t address;                    /**< Start address of requested read area. */
    uint32_t length;                     /**< Length of requested data to be read. */
    uint8_t  data[1];                    /**< Variable length table storing requested data. */
} recovery_bl_cmd_data_read_resp_t;

/**
 * @brief @ref RECOVERY_BL_CMD_PAGE_ERASE request command arguments.
 */
typedef struct __packed {
    uint32_t address;                    /**< Address of the page to be erased. */
} recovery_bl_cmd_page_erase_req_t;

/**
 * @brief @ref RECOVERY_BL_CMD_CRC_CHECK request command arguments.
 */
typedef struct __packed {
    uint32_t address;                    /**< Start address of CRC compute area. */
    uint32_t length;                     /**< Length of CRC compute area. */
} recovery_bl_cmd_crc_check_req_t;

/**
 * @brief @ref RECOVERY_BL_CMD_CRC_CHECK response command arguments.
 */
typedef struct __packed {
    uint32_t crc;                        /**< Requested CRC value. */
} recovery_bl_cmd_crc_check_resp_t;

/**
 * @brief @ref RECOVERY_BL_CMD_UICR_WRITE request command arguments.
 */
typedef struct __packed {
    uint32_t address;                    /**< Start address of UICR write area. */
    uint32_t length;                     /**< Length of UICR write area. */
    uint8_t  data[1];                    /**< Variable length table of data to be stored in UICR registers. */
} recovery_bl_cmd_uicr_write_req_t;

/**
 * @brief @ref RECOVERY_BL_CMD_UICR_READ request command arguments.
 */
typedef struct __packed {
    uint32_t address;                    /**< Start address of UICR read area. */
    uint32_t length;                     /**< Length of UICR data to be read. */
} recovery_bl_cmd_uicr_read_req_t;

/**
 * @brief @ref RECOVERY_BL_CMD_UICR_READ response command arguments.
 */
typedef struct __packed {
    uint32_t address;                    /**< Start address of requested UICR read area. */
    uint32_t length;                     /**< Length of requested UICR data to be read. */
    uint8_t  data[1];                    /**< Variable length table storing requested UICR data. */
} recovery_bl_cmd_uicr_read_resp_t;

/**
 * @brief @ref RECOVERY_BL_CMD_VERSION_GET response command arguments.
 */
typedef struct __packed {
    uint32_t protocol_version;           /**< Requested protocol version number. */
    uint32_t fw_version;                 /**< Requested firmware version number. */
} recovery_bl_cmd_version_get_resp_t;

/**
 * @brief @ref RECOVERY_BL_CMD_DEVICE_INFO_GET response command arguments.
 */
typedef struct __packed {
    uint32_t ficr_part;                  /**< Device part code(FICR.INFO.PART). */
    uint32_t ficr_variant;               /**< Device part variant(FICR.INFO.VARIANT). */
    uint32_t flash_size;                 /**< Device flash size. */
    uint32_t page_size;                  /**< Device page size. */
    uint32_t uicr_size;                  /**< Device UICR suze. */
} recovery_bl_cmd_dev_info_get_resp_t;

/**
 * @brief Request message structure.
 */
typedef struct __packed {
    uint32_t                  msg_crc;                    /**< Packet CRC. */
    recovery_bl_msg_req_hdr_t hdr;                        /**< Request message header. */
    union {                                      /**< Union of request commands specific data. */
        recovery_bl_cmd_data_write_req_t write;           /**< Write request command data. */
        recovery_bl_cmd_data_read_req_t  read;            /**< Read request command data. */
        recovery_bl_cmd_page_erase_req_t erase;           /**< Page erase request command data. */
        recovery_bl_cmd_crc_check_req_t  crc_check;       /**< CRC check request command data. */
        recovery_bl_cmd_uicr_write_req_t uicr_write;      /**< UICR write request command data. */
        recovery_bl_cmd_uicr_read_req_t  uicr_read;       /**< UICR read request command data. */
    } __packed;
} recovery_bl_msg_req_t;

/**
 * @brief Response message structure.
 */
typedef struct __packed { /* TODO make all these __packed at the end as is done elsewhere */
    uint32_t                   msg_crc;                   /**< Packet CRC. */
    recovery_bl_msg_resp_hdr_t hdr;                       /**< Response messsage header. */
    union {                                      /**< Union of response commands specific data. */
        recovery_bl_cmd_data_read_resp_t    read;         /**< Read response command data. */
        recovery_bl_cmd_crc_check_resp_t    crc_check;    /**< CRC check response command data. */
        recovery_bl_cmd_uicr_read_resp_t    uicr_read;    /**< UICR read response command data. */
        recovery_bl_cmd_version_get_resp_t  version_get;  /**< Get version response command data. */
        recovery_bl_cmd_dev_info_get_resp_t dev_info_get; /**< Obtain info about device flash and UICR. */
    } __packed;
} recovery_bl_msg_resp_t;

/**
 * @brief   Callback to be called on response transmission end.
 *
 * @details It is used to reset device after sending response
 *          to the @ref RECOVERY_BL_CMD_RESET request.
 */
void recovery_bl_tx_end_callback(void);

/**
 * @brief   Function for R-BL protocol messages processing.
 *
 * @details Executes received request and forms response
 *          message.
 *
 * @param[in]  p_msg  Pointer to the received message.
 * @param[out] p_resp Pointer to the response message.
 * @param[in]  length Received packet length.
 *
 * @return  Length of the response message.
 */
uint32_t recovery_bl_msg_process(recovery_bl_msg_req_t  * p_msg,
                                 recovery_bl_msg_resp_t * p_resp,
                                 uint32_t                 length);

#endif // RECOVERY_BOOTLOADER_H__
