/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "recovery_bootloader.h"
#include <sys/crc.h>
#include <nrfx_nvmc.h>
#include <string.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(recovery_bootloader, CONFIG_PRODUCTION_BOOTLOADER_LOG_LEVEL);

#define ACK_RESP_CRC_LEN (sizeof(p_resp->hdr))                        /**< Length of the CRC data for ACK response message. */
#define ACK_RESP_MSG_LEN (ACK_RESP_CRC_LEN + sizeof(p_resp->msg_crc)) /**< Length of the data packet for ACK response message. */

static uint32_t last_resp_len = 0;                                  /**< Length of the last response for the valid request. */
static uint8_t  seq_no        = 0xFF;                               /**< Sequence number of the last sended response. */
static bool     reset_flag    = false;                              /**< Flag used to trigger device reset after response transmission end. */


/* TODO kconfig these */
#define FW_VERSION_MAJOR 1
#define FW_VERSION_MINOR 1
#define FW_VERSION_BUGFIX 1

/**
 * @brief   Function for creating response ACK message.
 *
 * @param[out] p_resp      Pointer to the response message.
 * @param[in]  req_command Request command.
 * @param[in]  result      Command execution result.
 *
 * @return  Length of the response message.
 */
static uint32_t ack_resp_msg_prepare(recovery_bl_msg_resp_t *p_resp,
                                     uint8_t req_command,
                                     recovery_bl_response_result_t result)
{
    __ASSERT_NO_MSG(p_resp != NULL);
    __ASSERT_NO_MSG(req_command < RECOVERY_BL_CMD_COUNT);
    __ASSERT_NO_MSG(result      < RECOVERY_BL_NACK_COUNT);

    /* Setup ACK response fields and compute packet CRC. */
    p_resp->hdr.command     = RECOVERY_BL_CMD_RESPONSE;
    p_resp->hdr.req_command = req_command;
    p_resp->hdr.result      = result;
    p_resp->msg_crc         = crc32_ieee((uint8_t *)(&p_resp->hdr), ACK_RESP_CRC_LEN);

    /* ACK response command has fixed lenght. */
    return ACK_RESP_MSG_LEN;
}

/**
 * @brief   Function for checking if given value is
 *          inside given area.
 *
 * @param[in] value Value to be checked.
 * @param[in] left  Left area boundary.
 * @param[in] right Right area boundary.
 *
 * @return  True if value is inside area, false otherwise.
 */
static bool is_value_in_range(uint32_t value, uint32_t left, uint32_t right)
{
    return (value >= left) && (value < right);
}

/**
 * @brief   Function for checking if area is inside
 *          given boundaries.
 *
 * @param[in] area_begin Start of area to be checked.
 * @param[in] area_end   End of area to be checked.
 * @param[in] left       Left area boundary.
 * @param[in] right      Right area boundary.
 *
 * @return  True if area is inside boundaries, false otherwise.
 */
static bool is_area_in_range(uint32_t area_begin,
                             uint32_t area_end,
                             uint32_t left,
                             uint32_t right)
{
    return is_value_in_range(area_begin, left, right) &&
           is_value_in_range(area_end - 1, left, right);
}

/**
 * @brief   Function for checking if given flash area is erased.
 *
 * @brief   Checked area should be word aligned.
 *
 * @param[in] address Start of area to be checked.
 * @param[in] length  End of area to be checked.
 *
 * @return  True if area is erased, false otherwise.
 */
static bool is_area_erased(uint32_t address, uint32_t length)
{
    uint32_t * p_data_ptr = (uint32_t *)address;

    for (uint32_t i = 0; i < length / sizeof(uint32_t); i++)
    {
        if (p_data_ptr[i] != 0xFFFFFFFF)
        {
            return false;
        }
    }

    return true;
}

/**
 * @brief Function for erasing UICR page.
 */
static void erase_uicr(void)
{
    /* Enable erase. */
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Een;
    __ISB();
    __DSB();

    /* Erase the UICR page. */
    NRF_NVMC->ERASEUICR |= 1;

    /* Wait for erase operation end. */
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {;}

    /* Turn on erase protection. */
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren;
    __ISB();
    __DSB();
}

void recovery_bl_tx_end_callback(void)
{
    /* Reset device if responded to reset request command. */
    if (reset_flag)
    {
        NVIC_SystemReset();
    }
}

uint32_t recovery_bl_msg_process(recovery_bl_msg_req_t  * p_msg,
                                 recovery_bl_msg_resp_t * p_resp,
                                 uint32_t                 length)
{
    __ASSERT_NO_MSG(p_msg  != NULL);
    __ASSERT_NO_MSG(p_resp != NULL);
    __ASSERT_NO_MSG(length != 0);

    /* Length of response message. */
    uint32_t resp_len = 0;

    /* CRC is calculated starting from the packet header. */
    uint32_t crc      = crc32_ieee((uint8_t *)(&p_msg->hdr),
                                       length - sizeof(p_msg->msg_crc));

    /* Check packet integrity. */
    if (crc != p_msg->msg_crc)
    {
        /* Invalid CRC - send NACK response. */
        return ack_resp_msg_prepare(p_resp, p_msg->hdr.command, RECOVERY_BL_NACK_INVALID_CRC);
    }

    /* Check received packet length. */
    if (length >= RECOVERY_BL_TRANSPORT_MTU)
    {
        /* Send NACK if received packet is too long. */
        return ack_resp_msg_prepare(p_resp, p_msg->hdr.command, RECOVERY_BL_NACK_INVALID_MSG_SIZE);
    }

    /* Check sequence number to detect request command retransmission. */
    if ((p_msg->hdr.sequence_no == seq_no) &&
        (last_resp_len        != 0)        &&
        (seq_no               != 0xFF))
    {
        /* Retransmit last response message - request will not be */
        /* executed again as it has already been done first time. */
        return last_resp_len;
    }

    /* Copy the request sequence number into the response message. */
    p_resp->hdr.sequence_no = p_msg->hdr.sequence_no;

    /* Check if command is disabled due to APPROTECT turned on. */
    if (((NRF_UICR->APPROTECT & UICR_APPROTECT_PALL_Msk) != UICR_APPROTECT_PALL_Disabled) &&
        ((p_msg->hdr.command == RECOVERY_BL_CMD_DATA_WRITE) ||
         (p_msg->hdr.command == RECOVERY_BL_CMD_DATA_READ)  ||
         (p_msg->hdr.command == RECOVERY_BL_CMD_PAGE_ERASE) ||
         (p_msg->hdr.command == RECOVERY_BL_CMD_CRC_CHECK)  ||
         (p_msg->hdr.command == RECOVERY_BL_CMD_UICR_WRITE) ||
         (p_msg->hdr.command == RECOVERY_BL_CMD_UICR_ERASE) ||
         (p_msg->hdr.command == RECOVERY_BL_CMD_UICR_READ)))
    {
        /* Send NACK if APPROTECT is on and requested command is disabled. */
        return ack_resp_msg_prepare(p_resp, p_msg->hdr.command, RECOVERY_BL_NACK_APPROTECT_ON);
    }

    /* Process command. */
    switch (p_msg->hdr.command)
    {
        case RECOVERY_BL_CMD_PAGE_ERASE:
        {
            /* Check if page is not outside flash. */
            if (!(p_msg->erase.address < FLASH_SIZE))
            {
                /* Send NACK if address exceeds flash size. */
                return ack_resp_msg_prepare(p_resp,
                                            p_msg->hdr.command,
                                            RECOVERY_BL_NACK_INVALID_ADDRESS);
            }

            /* Check if address is page aligned. */
            if (p_msg->erase.address % PAGE_SIZE)
            {
                /* Send NACK if erase address is not page aligned. */
                return ack_resp_msg_prepare(p_resp,
                                            p_msg->hdr.command,
                                            RECOVERY_BL_NACK_DATA_NOT_ALIGNED);
            }

            /* Check against protected pages - serial Bootloader and MBR. */
            if (!is_area_in_range(p_msg->erase.address,
                                  p_msg->erase.address + PAGE_SIZE,
                                  MBR_START_ADDRESS + MBR_SIZE,
                                  BOOTLOADER_START_ADDRESS))
            {
                /* Send NACK if page is write protected. */
                return ack_resp_msg_prepare(p_resp,
                                            p_msg->hdr.command,
                                            RECOVERY_BL_NACK_PAGE_PROTECTED);
            }

            /* Perform page erase. */
            nrfx_nvmc_page_erase(p_msg->erase.address);

            /* Verify if page has been correctly erased. */
            if (!is_area_erased(p_msg->erase.address, PAGE_SIZE))
            {
                /* Send NACK if page is not erased properly. */
                return ack_resp_msg_prepare(p_resp,
                                            p_msg->hdr.command,
                                            RECOVERY_BL_NACK_ADDRESS_NOT_ERASED);
            }

            /* Send ACK response. */
            resp_len = ack_resp_msg_prepare(p_resp, p_msg->hdr.command, RECOVERY_BL_ACK);
        } break;

        case RECOVERY_BL_CMD_DATA_WRITE:
        {
            /* Check if message size is correct. */
            if ((length + 1) != (p_msg->write.length  +
                                 sizeof(p_msg->write) +
                                 sizeof(p_msg->hdr)   +
                                 sizeof(p_msg->msg_crc)))
            {
                /* Send NACK if message is invalid. */
                return ack_resp_msg_prepare(p_resp,
                                            p_msg->hdr.command,
                                            RECOVERY_BL_NACK_INVALID_MSG_SIZE);
            }

            /* Check if write area is not outside flash. */
            if (!is_value_in_range(p_msg->write.address + p_msg->write.length,
                                   0,
                                   FLASH_SIZE))
            {
                /* Send NACK if write area exceeds flash size. */
                return ack_resp_msg_prepare(p_resp,
                                            p_msg->hdr.command,
                                            RECOVERY_BL_NACK_INVALID_ADDRESS);
            }

            /* Check against protected pages - serial Bootloader and MBR. */
            if (!is_area_in_range(p_msg->write.address,
                                  p_msg->write.address + p_msg->write.length,
                                  MBR_START_ADDRESS + MBR_SIZE,
                                  BOOTLOADER_START_ADDRESS))
            {
                /* Send NACK if write area is protected. */
                return ack_resp_msg_prepare(p_resp,
                                            p_msg->hdr.command,
                                            RECOVERY_BL_NACK_PAGE_PROTECTED);
            }

            /* Check if data is word aligned. */
            if ((p_msg->write.address % sizeof(uint32_t)) ||
                (p_msg->write.length  % sizeof(uint32_t)))
            {
                /* Send NACK if data or address are not word aligned. */
                return ack_resp_msg_prepare(p_resp,
                                            p_msg->hdr.command,
                                            RECOVERY_BL_NACK_DATA_NOT_ALIGNED);
            }

            /* Check if write area is erased. */
            if (!is_area_erased(p_msg->write.address, p_msg->write.length))
            {
                /* Send NACK if write area is not erased. */
                return ack_resp_msg_prepare(p_resp,
                                            p_msg->hdr.command,
                                            RECOVERY_BL_NACK_ADDRESS_NOT_ERASED);
            }

            /* Write data to flash. */
            nrfx_nvmc_bytes_write(p_msg->write.address,
                                 (uint32_t *)p_msg->write.data,
                                 p_msg->write.length);

            /* Send ACK response. */
            resp_len = ack_resp_msg_prepare(p_resp,
                                            p_msg->hdr.command,
                                            RECOVERY_BL_ACK);
        } break;

        case RECOVERY_BL_CMD_CRC_CHECK:
        {
            /* Check if CRC area is inside flash space. */
            if (!is_area_in_range(p_msg->crc_check.address,
                                  p_msg->crc_check.address + p_msg->crc_check.length,
                                  0,
                                  FLASH_SIZE))
            {
                /* Send NACK if address exceeds flash size. */
                return ack_resp_msg_prepare(p_resp,
                                            p_msg->hdr.command,
                                            RECOVERY_BL_NACK_INVALID_ADDRESS);
            }

            /* Form response message with CRC computed for a given area. */
            uint32_t packet_crc_len = sizeof(p_resp->crc_check) + sizeof(p_resp->hdr);
            resp_len                = packet_crc_len + sizeof(p_resp->msg_crc);
            p_resp->hdr.req_command = RECOVERY_BL_CMD_CRC_CHECK;
            p_resp->hdr.result      = RECOVERY_BL_ACK;
            p_resp->hdr.command     = RECOVERY_BL_CMD_RESPONSE;
            p_resp->crc_check.crc   = crc32_ieee((uint8_t *)p_msg->crc_check.address,
                                                               p_msg->crc_check.length);

            /* Compute response packet CRC. */
            p_resp->msg_crc         = crc32_ieee((uint8_t *)(&p_resp->hdr),
                                                     packet_crc_len);
        } break;

        case RECOVERY_BL_CMD_RESET:
        {
            /* Jump to the application through reset and MBR - command */
            /* will be executed after ACK response transmit ends. */
            reset_flag = true;

            /* Send ACK response. */
            resp_len     = ack_resp_msg_prepare(p_resp, p_msg->hdr.command, RECOVERY_BL_ACK);
        } break;

        case RECOVERY_BL_CMD_UICR_WRITE:
        {
            /* Check if received message size is correct. */
            if ((length + 1) != (p_msg->uicr_write.length  +
                                 sizeof(p_msg->uicr_write) +
                                 sizeof(p_msg->hdr)        +
                                 sizeof(p_msg->msg_crc)))
            {
                /* Send NACK if received message size is invalid. */
                return ack_resp_msg_prepare(p_resp,
                                            p_msg->hdr.command,
                                            RECOVERY_BL_NACK_INVALID_MSG_SIZE);
            }

            /* Check if write area is inside UICR address space. */
            if (!is_area_in_range(p_msg->uicr_write.address,
                                  p_msg->uicr_write.address + p_msg->uicr_write.length,
                                  (uint32_t)NRF_UICR,
                                  ((uint32_t)NRF_UICR) + sizeof(NRF_UICR_Type)))
            {
                /* Send NACK if write area exceeds UICR address space. */
                return ack_resp_msg_prepare(p_resp,
                                            p_msg->hdr.command,
                                            RECOVERY_BL_NACK_INVALID_ADDRESS);
            }

            /* Check if data is word aligned. */
            if ((p_msg->uicr_write.address % sizeof(uint32_t)) ||
                (p_msg->uicr_write.length  % sizeof(uint32_t)))
            {
                /* Send NACK if data is not word aligned. */
                return ack_resp_msg_prepare(p_resp,
                                            p_msg->hdr.command,
                                            RECOVERY_BL_NACK_DATA_NOT_ALIGNED);
            }

            /* Verify if area may be programmed without erase. */
            uint32_t * p_uicr_reg = (uint32_t *)p_msg->uicr_write.address;
            uint32_t * p_data     = (uint32_t *)p_msg->uicr_write.data;
            for (uint32_t i = 0; i < p_msg->uicr_write.length / sizeof(uint32_t); i++)
            {
                if ((p_uicr_reg[i] & p_data[i]) != p_data[i])
                {
                    /* Send NACK if register may not be properly flashed without erase. */
                    return ack_resp_msg_prepare(p_resp,
                                                p_msg->hdr.command,
                                                RECOVERY_BL_NACK_ADDRESS_NOT_ERASED);
                }
            }

            /* Write data to UICR registers. */
            nrfx_nvmc_bytes_write(p_msg->uicr_write.address,
                                 (uint32_t *)p_msg->uicr_write.data,
                                 p_msg->uicr_write.length);

            /* Send ACK response. */
            resp_len = ack_resp_msg_prepare(p_resp, p_msg->hdr.command, RECOVERY_BL_ACK);
        } break;

        case RECOVERY_BL_CMD_UICR_ERASE:
        {
            erase_uicr();

            /* Send ACK response. */
            resp_len = ack_resp_msg_prepare(p_resp, p_msg->hdr.command, RECOVERY_BL_ACK);
        } break;

        case RECOVERY_BL_CMD_DATA_READ:
        {
            /* Check if read area is inside flash address space. */
            if (!((p_msg->read.address + p_msg->read.length) < FLASH_SIZE))
            {
                /* Send NACK if read area exceeds flash size. */
                return ack_resp_msg_prepare(p_resp,
                                            p_msg->hdr.command,
                                            RECOVERY_BL_NACK_INVALID_ADDRESS);
            }

            /* Check if response command length is less than packet MTU. */
            if (!((p_msg->read.length         +
                   sizeof(p_resp->msg_crc)    +
                   (sizeof(p_resp->read) - 1) +
                   sizeof(p_resp->hdr))       < RECOVERY_BL_TRANSPORT_MTU))
            {
                /* Send NACK if requested command response exceeds packet MTU. */
                return ack_resp_msg_prepare(p_resp,
                                            p_msg->hdr.command,
                                            RECOVERY_BL_NACK_INVALID_MSG_SIZE);
            }

            /* Copy requested data. */
            uint8_t * p_data = (uint8_t *)(p_msg->read.address);
            memcpy(p_resp->read.data, p_data, p_msg->read.length);

            /* Form response message */
            uint32_t packet_crc_len = p_msg->read.length         +
                                      sizeof(p_resp->hdr)        +
                                      (sizeof(p_resp->read) - 1);
            resp_len                = packet_crc_len + sizeof(p_resp->msg_crc);
            p_resp->read.address    = p_msg->read.address;
            p_resp->read.length     = p_msg->read.length;
            p_resp->hdr.req_command = RECOVERY_BL_CMD_DATA_READ;
            p_resp->hdr.result      = RECOVERY_BL_ACK;
            p_resp->hdr.command     = RECOVERY_BL_CMD_RESPONSE;

            /* Compute response packet CRC. */
            p_resp->msg_crc         = crc32_ieee((uint8_t *)(&p_resp->hdr),
                                                     packet_crc_len);
        } break;

        case RECOVERY_BL_CMD_UICR_READ:
        {
            /* Check if read area is inside UICR address space. */
            if (!is_area_in_range(p_msg->uicr_read.address,
                                  p_msg->uicr_read.address + p_msg->uicr_read.length,
                                  (uint32_t)NRF_UICR,
                                  ((uint32_t)NRF_UICR) + sizeof(NRF_UICR_Type)))
            {
                /* Send NACK if read area exceeds UICR registers area. */
                return ack_resp_msg_prepare(p_resp,
                                            p_msg->hdr.command,
                                            RECOVERY_BL_NACK_INVALID_ADDRESS);
            }

            /* Check if response command length is less than MTU. */
            if (!((p_msg->uicr_read.length         +
                   sizeof(p_resp->msg_crc)         +
                   (sizeof(p_resp->uicr_read) - 1) +
                   sizeof(p_resp->hdr))            < RECOVERY_BL_TRANSPORT_MTU))
            {
                /* Send NACK if requested command response exceeds packet MTU. */
                return ack_resp_msg_prepare(p_resp,
                                            p_msg->hdr.command,
                                            RECOVERY_BL_NACK_INVALID_MSG_SIZE);
            }

            /* Copy requested data. */
            uint8_t * p_data = (uint8_t *)(p_msg->uicr_read.address);
            memcpy(p_resp->uicr_read.data, p_data, p_msg->uicr_read.length);

            /* Form response message. */
            uint32_t packet_crc_len   = p_msg->uicr_read.length         +
                                        sizeof(p_resp->hdr)             +
                                        (sizeof(p_resp->uicr_read) - 1);
            resp_len                  = packet_crc_len + sizeof(p_resp->msg_crc);
            p_resp->uicr_read.address = p_msg->uicr_read.address;
            p_resp->uicr_read.length  = p_msg->uicr_read.length;
            p_resp->hdr.req_command   = RECOVERY_BL_CMD_UICR_READ;
            p_resp->hdr.result        = RECOVERY_BL_ACK;
            p_resp->hdr.command       = RECOVERY_BL_CMD_RESPONSE;

            /* Compute response packet CRC. */
            p_resp->msg_crc           = crc32_ieee((uint8_t *)(&p_resp->hdr),
                                                       packet_crc_len);
        } break;

        case RECOVERY_BL_CMD_VERSION_GET:
        {
            /* Form response message with protocol version. */
            uint32_t packet_crc_len              = sizeof(p_resp->version_get) +
                                                   sizeof(p_resp->hdr);
            resp_len                             = packet_crc_len + sizeof(p_resp->msg_crc);
            p_resp->version_get.protocol_version = RECOVERY_BL_PROTOCOL_VERSION;
            p_resp->version_get.fw_version       = (FW_VERSION_MAJOR << 16) |
                                                   (FW_VERSION_MINOR << 8)  |
                                                    FW_VERSION_BUGFIX;
            p_resp->hdr.req_command              = RECOVERY_BL_CMD_VERSION_GET;
            p_resp->hdr.result                   = RECOVERY_BL_ACK;
            p_resp->hdr.command                  = RECOVERY_BL_CMD_RESPONSE;

            /* Compute response packet CRC. */
            p_resp->msg_crc                      = crc32_ieee((uint8_t *)(&p_resp->hdr),
                                                                  packet_crc_len);
        } break;

        case RECOVERY_BL_CMD_DEVICE_INFO_GET:
        {
            /* Form response message with device info. */
            uint32_t packet_crc_len           = sizeof(p_resp->dev_info_get) +
                                                sizeof(p_resp->hdr);
            resp_len                          = packet_crc_len + sizeof(p_resp->msg_crc);
            p_resp->dev_info_get.ficr_part    = NRF_FICR->INFO.PART;
            p_resp->dev_info_get.ficr_variant = NRF_FICR->INFO.VARIANT;
            p_resp->dev_info_get.flash_size   = FLASH_SIZE;
            p_resp->dev_info_get.page_size    = PAGE_SIZE;
            p_resp->dev_info_get.uicr_size    = UICR_SIZE;
            p_resp->hdr.req_command           = RECOVERY_BL_CMD_DEVICE_INFO_GET;
            p_resp->hdr.result                = RECOVERY_BL_ACK;
            p_resp->hdr.command               = RECOVERY_BL_CMD_RESPONSE;

            /* Compute response packet CRC. */
            p_resp->msg_crc                   = crc32_ieee((uint8_t *)(&p_resp->hdr),
                                                               packet_crc_len);
        } break;

        case RECOVERY_BL_CMD_ERASE_ALL:
        {
            /* Erase all flash except MBR and R-BL. */
            uint32_t start_address = MBR_START_ADDRESS + MBR_SIZE;
            uint32_t page_num      = (FLASH_SIZE - MBR_SIZE - BOOTLOADER_SIZE) / PAGE_SIZE;

            for (uint32_t i = 0; i < page_num; i++)
            {
                nrfx_nvmc_page_erase(start_address + (i * PAGE_SIZE));
            }

            /* Erase UICR. */
            erase_uicr();

            resp_len = ack_resp_msg_prepare(p_resp, p_msg->hdr.command, RECOVERY_BL_ACK);
        } break;

        default:
        {
            /* Send NACK if command is not supported. */
            return ack_resp_msg_prepare(p_resp, p_msg->hdr.command, RECOVERY_BL_NACK_INVALID_CMD);
        }
    }

    /* Save message length in case of retransmission. */
    last_resp_len = resp_len;

    return resp_len;
}
