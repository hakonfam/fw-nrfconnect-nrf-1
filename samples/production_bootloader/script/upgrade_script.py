import serial
import sliplib
from recovery_bl import *
from intelhex import IntelHex
import time


RblDriver = RecoveryBl('COM44', 115200, 1)

start = time.time()
RblDriver.flash_hex("main.hex")
RblDriver.verify_hex("main.hex")
RblDriver.device_reset()

end = time.time()
print(end - start)

