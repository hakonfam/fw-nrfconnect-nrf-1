import serial
import sliplib
from recovery_bl import *
from intelhex import IntelHex
import time

RblDriver = RecoveryBl(port='COM44', baudrate=115200, argTimeout=1)

UICR_START_ADDRESS = 0x10001000
PSELRESET = 0x10001000 + 0x200

RblDriver.flash_uicr([21, 0,  0, 0, 21, 0, 0, 0], PSELRESET)
