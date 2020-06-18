import serial
import sliplib
from enum import Enum
import struct
import binascii
import math
from intelhex import IntelHex
import logging

MBR_SIZE                                = 4096
MBR_ADDRESS                             = 0
BOOTLOADER_SIZE                         = 4096
CHUNK_SIZE                              = 1024
RETRY_NUM                               = 3
UICR_START_ADDRESS                      = 0x10001000
UICR_APPROTECT_REGISTER_ADDRESS         = 0x10001208

class RecoveryBlCmds(Enum):
    RECOVERY_BL_CMD_RESPONSE             = 0
    RECOVERY_BL_CMD_DATA_WRITE           = 1
    RECOVERY_BL_CMD_DATA_READ            = 2
    RECOVERY_BL_CMD_PAGE_ERASE           = 3
    RECOVERY_BL_CMD_CRC_CHECK            = 4
    RECOVERY_BL_CMD_RESET                = 5
    RECOVERY_BL_CMD_UICR_WRITE           = 6
    RECOVERY_BL_CMD_UICR_ERASE           = 7
    RECOVERY_BL_CMD_UICR_READ            = 8
    RECOVERY_BL_CMD_VERSION_GET          = 9
    RECOVERY_BL_CMD_DEVICE_INFO_GET      = 10
    RECOVERY_BL_CMD_ERASE_ALL            = 11
    RECOVERY_BL_CMD_COUNT                = 12

class RecoveryBlAckReason(Enum):
    RECOVERY_BL_ACK                      = 0
    RECOVERY_BL_NACK_INVALID_CRC         = 1
    RECOVERY_BL_NACK_INVALID_CMD         = 2
    RECOVERY_BL_NACK_INVALID_MSG_SIZE    = 3
    RECOVERY_BL_NACK_PAGE_PROTECTED      = 4
    RECOVERY_BL_NACK_ADDRESS_NOT_ERASED  = 5
    RECOVERY_BL_NACK_DATA_NOT_ALIGNED    = 6
    RECOVERY_BL_NACK_INVALID_ADDRESS     = 7
    RECOVERY_BL_NACK_APPROTECT_ON        = 8
    RECOVERY_BL_NACK_COUNT               = 9

class RecoveryBlCommunicationError(Exception):
    def __init__(self, nack):
        self.nackResult = nack

class RecoveryBlTimeoutError(Exception):
    pass

class RecoveryBlVerificationError(Exception):
    pass

class RecoveryBl:
    def __init__(self, port, baudrate, argTimeout, log_level=logging.WARNING):
        logging.basicConfig(level=log_level)
        self._slip_driver = sliplib.Driver()
        self._serial = serial.Serial(port, baudrate, parity=serial.PARITY_NONE, timeout=argTimeout, rtscts=0) # TODO timeout=argTimeout instead of 0
        self._tx_buff = bytearray()
        self._seq_no = 0

        # Get device HW info
        dev_info = self.read_device_info()
        if dev_info != None:
            self.device_part    = struct.unpack("<I", dev_info[0:4])[0]
            self.device_variant = struct.unpack("<I", dev_info[4:8])[0]
            self.flash_size     = struct.unpack("<I", dev_info[8:12])[0]
            self.page_size      = struct.unpack("<I", dev_info[12:16])[0]
            self.uicr_size      = struct.unpack("<I", dev_info[16:20])[0]
        else:
            raise RecoveryBlTimeoutError()

        # Get S-BL firmware version info
        version_info = self.get_version_info()
        if version_info != None:
            self.protocol_version  = struct.unpack("<I", version_info[0:4])[0]
            self.fw_version_string = str(struct.unpack("<B", version_info[6:7])[0]) + "." + \
                                     str(struct.unpack("<B", version_info[5:6])[0]) + "." + \
                                     str(struct.unpack("<B", version_info[4:5])[0])
        else:
            raise RecoveryBlTimeoutError()

    def array_4B_padding(self, data):
        if len(data) % 4:
            padding = 4 - len(data) % 4
            for i in range(0, padding):
                data.append(255)

            logging.info('Unalinged data, adding padding')
        return data

    def build_write_msg(self, address, data):        
        self._tx_buff  = bytearray(0)
        self._tx_buff += struct.pack("B", self._seq_no)
        self._tx_buff += struct.pack("B", RecoveryBlCmds.RECOVERY_BL_CMD_DATA_WRITE.value)
        self._tx_buff += struct.pack("<I", address)
        self._tx_buff += struct.pack("<I", len(data))

        for byte in data:
            self._tx_buff += struct.pack("B", byte)
        
        self._tx_buff  = struct.pack("<I", binascii.crc32(self._tx_buff, 0)) + self._tx_buff

    def build_erase_page_msg(self, page_address):
        self._tx_buff  = bytearray(0)
        self._tx_buff += struct.pack("B", self._seq_no)
        self._tx_buff += struct.pack("B", RecoveryBlCmds.RECOVERY_BL_CMD_PAGE_ERASE.value)
        self._tx_buff += struct.pack("<I", page_address)
        self._tx_buff  = struct.pack("<I", binascii.crc32(self._tx_buff, 0)) + self._tx_buff

    def build_verify_msg(self, address, length):
        self._tx_buff  = bytearray(0)
        self._tx_buff += struct.pack("B", self._seq_no)
        self._tx_buff += struct.pack("B", RecoveryBlCmds.RECOVERY_BL_CMD_CRC_CHECK.value)
        self._tx_buff += struct.pack("<I", address)
        self._tx_buff += struct.pack("<I", length)
        self._tx_buff  = struct.pack("<I", binascii.crc32(self._tx_buff, 0)) + self._tx_buff

    def build_reset_msg(self):
        self._tx_buff  = bytearray(0)
        self._tx_buff += struct.pack("B", self._seq_no)
        self._tx_buff += struct.pack("B", RecoveryBlCmds.RECOVERY_BL_CMD_RESET.value)
        self._tx_buff  = struct.pack("<I", binascii.crc32(self._tx_buff, 0)) + self._tx_buff

    def build_read_uicr_msg(self, address, length):
        self._tx_buff  = bytearray(0)
        self._tx_buff += struct.pack("B", self._seq_no)
        self._tx_buff += struct.pack("B", RecoveryBlCmds.RECOVERY_BL_CMD_UICR_READ.value)
        self._tx_buff += struct.pack("<I", address)
        self._tx_buff += struct.pack("<I", length)
        self._tx_buff  = struct.pack("<I", binascii.crc32(self._tx_buff, 0)) + self._tx_buff

    def build_erase_uicr_msg(self):
        self._tx_buff  = bytearray(0)
        self._tx_buff += struct.pack("B", self._seq_no)
        self._tx_buff += struct.pack("B", RecoveryBlCmds.RECOVERY_BL_CMD_UICR_ERASE.value)
        self._tx_buff  = struct.pack("<I", binascii.crc32(self._tx_buff, 0)) + self._tx_buff

    def build_write_uicr_msg(self, address, data):
        self._tx_buff  = bytearray(0)
        self._tx_buff += struct.pack("B", self._seq_no)
        self._tx_buff += struct.pack("B", RecoveryBlCmds.RECOVERY_BL_CMD_UICR_WRITE.value)
        self._tx_buff += struct.pack("<I", address)
        self._tx_buff += struct.pack("<I", len(data))

        for byte in data:
            self._tx_buff += struct.pack("B", byte)

        self._tx_buff  = struct.pack("<I", binascii.crc32(self._tx_buff, 0)) + self._tx_buff

    def build_version_get_msg(self):
        self._tx_buff  = bytearray(0)
        self._tx_buff += struct.pack("B", self._seq_no)
        self._tx_buff += struct.pack("B", RecoveryBlCmds.RECOVERY_BL_CMD_VERSION_GET.value)
        self._tx_buff  = struct.pack("<I", binascii.crc32(self._tx_buff, 0)) + self._tx_buff

    def build_device_info_get_msg(self):
        self._tx_buff  = bytearray(0)
        self._tx_buff += struct.pack("B", self._seq_no)
        self._tx_buff += struct.pack("B", RecoveryBlCmds.RECOVERY_BL_CMD_DEVICE_INFO_GET.value)
        self._tx_buff  = struct.pack("<I", binascii.crc32(self._tx_buff, 0)) + self._tx_buff

    def build_erase_all_msg(self):
        self._tx_buff  = bytearray(0)
        self._tx_buff += struct.pack("B", self._seq_no)
        self._tx_buff += struct.pack("B", RecoveryBlCmds.RECOVERY_BL_CMD_ERASE_ALL.value)
        self._tx_buff  = struct.pack("<I", binascii.crc32(self._tx_buff, 0)) + self._tx_buff

    def verify_hex(self, hexPath):
        ih = IntelHex(hexPath)
        for s in ih.segments():
            start_addr = s[0]
            end_addr   = s[1]
            bin = ih.tobinarray(start=start_addr, size = (end_addr - start_addr))
            data = bin

            # Ignore MBR section
            if start_addr < (MBR_SIZE + MBR_ADDRESS):
                if len(data) + start_addr < (MBR_SIZE + MBR_ADDRESS):
                    continue
                data = data[(MBR_SIZE + MBR_ADDRESS - start_addr):]
                start_addr = MBR_SIZE + MBR_ADDRESS

            if start_addr > UICR_START_ADDRESS:
                uicrData = self.read_uicr(start_addr, len(data))
                if bytearray(uicrData) != bytearray(data):
                    raise RecoveryBlVerificationError()
            else:
                flashDataCrc = self.flash_data_crc_get(start_addr, len(data))
                if flashDataCrc == None or flashDataCrc != binascii.crc32(data, 0):
                    raise RecoveryBlVerificationError()
        return True

    def device_reset(self):
        # Bump up packet sequence number
        self.seq_no_inc()

        self.build_reset_msg()
        # Send packet with retries
        for i in range(0, RETRY_NUM):
            self._serial.write(self._slip_driver.send(self._tx_buff))
            resp = self.read_response()
            if resp != None and self.packet_crc_check(resp):
                if self.is_protocol_error(resp):
                    raise RecoveryBlCommunicationError(RecoveryBlAckReason(self.response_result_get(resp)))
                elif self.is_crc_error(resp):
                    continue
                return
        raise RecoveryBlTimeoutError()

    def read_response(self):
        while True:
            try:
                s = self._serial.read()
            except serial.SerialException as e:
                print(e)
                return None
            if not s:
                return None
            resp = self._slip_driver.receive(s)
            if resp:
                return resp[0]

    def flash_data(self, data, address):
        data = self.array_4B_padding(data)
        # Ignore MBR section
        if address < (MBR_SIZE + MBR_ADDRESS):
            if len(data) + address < (MBR_SIZE + MBR_ADDRESS):
                return
            data = data[(MBR_SIZE + MBR_ADDRESS - address):]
            address = MBR_SIZE + MBR_ADDRESS

        page_num = int(math.ceil(len(data)/self.page_size))
        start_page = int(address/self.page_size)

        logging.debug('flashing data, address: %x, bytes:%d, start_page:%d, pages:%d', address, len(data), start_page, page_num)
            
        for chunk in range(0, len(data), CHUNK_SIZE):
            logging.debug("Writing chunk addr:0X%08x", address+chunk)
            if ((address + chunk) % self.page_size == 0):
                #Erase page if chunk is at the start of page.
                self.build_erase_page_msg(address + chunk)
                if self.send_packet() == None:
                    raise RecoveryBlTimeoutError()

            self.build_write_msg(address+chunk, data[chunk: chunk+CHUNK_SIZE])
            if self.send_packet() == None:
                raise RecoveryBlTimeoutError()
        packetCrc = self.flash_data_crc_get(address, len(data))
        if packetCrc == None:
            raise RecoveryBlTimeoutError()
        if packetCrc != binascii.crc32(data, 0):
            raise RecoveryBlVerificationError()

    def flash_uicr(self, data, address):
        # Split data in chunks that fits packet MTU
        for chunk in range(0, len(data), CHUNK_SIZE):
            self.build_write_uicr_msg(address+chunk, data[chunk: chunk+CHUNK_SIZE])
            if self.send_packet() == None:
                raise RecoveryBlTimeoutError()
        # Verify programmed data
        try:
            read_data = self.read_uicr(address, len(data))
            if bytearray(read_data) != bytearray(data):
                raise RecoveryBlVerificationError()
        except RecoveryBlCommunicationError as err:
            if err.nackResult.value == RecoveryBlAckReason.RECOVERY_BL_NACK_APPROTECT_ON.value and \
                address == UICR_APPROTECT_REGISTER_ADDRESS:
                pass
            else:
                raise

    def flash_hex(self, hexPath):
        ih = IntelHex(hexPath)
        for s in ih.segments():
            start_addr = s[0]
            end_addr   = s[1]
            bin = ih.tobinarray(start=start_addr, size = (end_addr - start_addr))
            # Determine if segment is in UICR or FLASH area
            if start_addr > UICR_START_ADDRESS:
                self.flash_uicr(bin, start_addr)
            else:
                self.flash_data(bin, start_addr)

    def packet_crc_check(self, packet):
        crc = struct.unpack("<I", packet[0:4])[0]
        return crc ==  binascii.crc32(packet[4:], 0)

    # Send packet + wait for ACK response
    def send_packet(self):
        # Bump up packet sequence number
        self.seq_no_inc()

        # Send packet with retries
        for i in range(0, RETRY_NUM):
            self._serial.write(self._slip_driver.send(self._tx_buff))
            # Wait for response
            resp = self.read_response()
            if resp != None and self.packet_crc_check(resp):
                if self.is_protocol_error(resp):
                    raise RecoveryBlCommunicationError(RecoveryBlAckReason(self.response_result_get(resp)))
                elif self.is_crc_error(resp):
                    continue
                return resp
        return None

    # Get CRC of flashed data from device
    def flash_data_crc_get(self, address, length):
        # Bump up packet sequence number
        self.seq_no_inc()

        self.build_verify_msg(address, length)
        # Send packet with retries
        for i in range(0, RETRY_NUM):
            self._serial.write(self._slip_driver.send(self._tx_buff))
            # Wait for response
            resp = self.read_response()
            if resp != None and self.packet_crc_check(resp):
                if self.is_protocol_error(resp):
                    raise RecoveryBlCommunicationError(RecoveryBlAckReason(self.response_result_get(resp)))
                elif self.is_crc_error(resp):
                    continue
                crc = struct.unpack("<I", resp[8:])[0]
                return crc
        return None

    def is_crc_error(self, resp):
        return self.response_result_get(resp) == RecoveryBlAckReason.RECOVERY_BL_NACK_INVALID_CRC.value

    def is_protocol_error(self, resp):
        return self.response_result_get(resp) != RecoveryBlAckReason.RECOVERY_BL_ACK.value and            \
                self.response_result_get(resp) != RecoveryBlAckReason.RECOVERY_BL_NACK_INVALID_CRC.value

    def response_result_get(self, data):
        return data[7]

    def seq_no_inc(self):
        self._seq_no += 1
        if self._seq_no >= 255:
            self._seq_no = 0

    def read_uicr(self, address, length):
        # Bump up packet sequence number
        self.seq_no_inc()

        # Build packet message
        self.build_read_uicr_msg(address, length)
        # Send packet with retries
        for i in range(0, RETRY_NUM):
            self._serial.write(self._slip_driver.send(self._tx_buff))
            # Wait for response
            resp = self.read_response()
            if resp != None and self.packet_crc_check(resp):
                if self.is_protocol_error(resp):
                    raise RecoveryBlCommunicationError(RecoveryBlAckReason(self.response_result_get(resp)))
                elif self.is_crc_error(resp):
                    continue
                data = resp[16:]
                return data
        return None

    def read_device_info(self):
        # Bump up packet sequence number
        self.seq_no_inc()

        # Build packet message
        self.build_device_info_get_msg()

        # Send packet with retries
        for i in range(0, RETRY_NUM):
            to_send = self._slip_driver.send(self._tx_buff)
            self._serial.write(to_send)
            # Wait for response
            resp = self.read_response()
            if resp != None and self.packet_crc_check(resp):
                if self.is_protocol_error(resp):
                    raise RecoveryBlCommunicationError(RecoveryBlAckReason(self.response_result_get(resp)))
                elif self.is_crc_error(resp):
                    continue
                data = resp[8:]
                return data
        return None

    def chip_flash_erase(self):
        start_page = int((MBR_ADDRESS + MBR_SIZE)/self.page_size)
        page_num = int(math.ceil((self.flash_size - MBR_SIZE - BOOTLOADER_SIZE)/self.page_size))
        for page in range(start_page, (start_page + page_num)):
            self.build_erase_page_msg(page * self.page_size)
            resp = self.send_packet() 
            if resp != None and self.packet_crc_check(resp):
                if self.is_protocol_error(resp):
                    raise RecoveryBlCommunicationError(RecoveryBlAckReason(self.response_result_get(resp)))
                elif self.is_crc_error(resp):
                    continue
                else:
                    return
        raise RecoveryBlTimeoutError()

    def erase_all(self):
        # Bump up packet sequence number
        self.seq_no_inc()

        self.build_erase_all_msg()
        # Send packet with retries
        for i in range(0, RETRY_NUM):
            self._serial.write(self._slip_driver.send(self._tx_buff))
            resp = self.read_response()
            if resp != None and self.packet_crc_check(resp):
                if self.is_protocol_error(resp):
                    raise RecoveryBlCommunicationError(RecoveryBlAckReason(self.response_result_get(resp)))
                elif self.is_crc_error(resp):
                    continue
                else:
                    return
        raise RecoveryBlTimeoutError()

    def erase_uicr(self):
        # Bump up packet sequence number
        self.seq_no_inc()

        self.build_erase_uicr_msg()
        # Send packet with retries
        for i in range(0, RETRY_NUM):
            self._serial.write(self._slip_driver.send(self._tx_buff))
            resp = self.read_response()
            if resp != None and self.packet_crc_check(resp):
                if self.is_protocol_error(resp):
                    raise RecoveryBlCommunicationError(RecoveryBlAckReason(self.response_result_get(resp)))
                elif self.is_crc_error(resp):
                    continue
                else:
                    return
        raise RecoveryBlTimeoutError()

    def protect_application(self):
        try:
            self.flash_uicr([0, 0xFF, 0xFF , 0xFF], UICR_APPROTECT_REGISTER_ADDRESS)
        except RecoveryBlCommunicationError as err:
            if err.nackResult.value == RecoveryBlAckReason.RECOVERY_BL_NACK_APPROTECT_ON.value:
                pass
            else:
                raise

    def erase_page(self, page_address):
        # Bump up packet sequence number
        self.seq_no_inc()

        self.build_erase_page_msg(page_address)
        # Send packet with retries
        for i in range(0, RETRY_NUM):
            self._serial.write(self._slip_driver.send(self._tx_buff))
            resp = self.read_response()
            if resp != None and self.packet_crc_check(resp):
                if self.is_protocol_error(resp):
                    raise RecoveryBlCommunicationError(RecoveryBlAckReason(self.response_result_get(resp)))
                elif self.is_crc_error(resp):
                    continue
                else:
                    return
        raise RecoveryBlTimeoutError()

    def get_version_info(self):
        # Bump up packet sequence number
        self.seq_no_inc()

        # Build packet message
        self.build_version_get_msg()
        # Send packet with retries
        for i in range(0, RETRY_NUM):
            self._serial.write(self._slip_driver.send(self._tx_buff))
            # Wait for response
            resp = self.read_response()
            if resp != None and self.packet_crc_check(resp):
                if self.is_protocol_error(resp):
                    raise RecoveryBlCommunicationError(RecoveryBlAckReason(self.response_result_get(resp)))
                elif self.is_crc_error(resp):
                    continue
                data = resp[8:]
                return data
        return None

    def get_fw_version(self):
        return self.fw_version_string
