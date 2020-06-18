import argparse
import recovery_bl
import struct
import logging

CLI_TOOL_VERSION = "0.8.3"
LOG_FILENAME     = 'RecoveryBootloader.log'

# create log instance
logger = logging.getLogger('R-BL CLI v' + CLI_TOOL_VERSION)
logger.setLevel(logging.INFO)

# create console handler
ch = logging.StreamHandler()
ch.setLevel(logging.INFO)

# add the handlers to the logger
logger.addHandler(ch)

def main():
    parser = argparse.ArgumentParser()

    #
    # Commands
    #
    parser.add_argument('--program', help='Flash specified hex. Additional parameter describe type of erase - default erase type is the --sectorerase.')
    parser.add_argument('--reset', help='Reset device.', action='store_true')
    parser.add_argument('--eraseall', help='Erase all pages except MBR and bootloader, erase UICR page.', action='store_true')
    parser.add_argument('--erasepage', help='Erase given page (Will not accept UICR nor page used by MBR & R-BL).')
    parser.add_argument('--verify', help='Verify if given hex file is stored in flash memory.')
    parser.add_argument('--readuicr', help='Read UICR page and store it to the specified file.')
    parser.add_argument('--writeuicr', help='Write value to the UICR register.', nargs='*')
    parser.add_argument('--protectapp', help='Turns on application read protection by clearing APPROTECT UICR register.', action='store_true')
    parser.add_argument('--fw_version', help='Prints device R-BL firmware version.', action='store_true')
    parser.add_argument('--log', help='Turns on logging to the ' + LOG_FILENAME + ' file.', action='store_true')

    # Group of erase options for --program command
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--chipflasherase', help='Can be added to --program command. '
                                                'If --chipflasherase is given, all the available user '
                                                'flash memory will be erased before programming.', action='store_true')
    group.add_argument('--chiperaseall', help='Can be added to --program command. '
                                              'If --chiperaseall is given, all the available user '
                                              'flash memory and UCIR page will be erased before programming.', action='store_true')
    group.add_argument('--sectorerase', help='Can be added to --program command. '
                                             'If --sectorerase is given, only the targeted non-volatile '
                                             'memory pages excluding UICR will be erased.', action='store_true')

    #
    # Parameters
    #
    parser.add_argument('-p', help='COM port to be used for transmission, e.g. "COM1"', required=True, type=str)
#    parser.add_argument('-b', help='Baudrate', type=int, default=115200)
#    parser.add_argument('-fc', help='To enable flow control, set this flag to 1', type=int, default=0)
#    parser.add_argument('-mtu', help='Max message size', type=int, default=1024)
    parser.add_argument('-t', help='Timeout in seconds', type=int, default=1)

    args = parser.parse_args()
    logger.debug('Arguments: ' + str(args))

    # Turn on file logger if requested.
    log_lvl = logging.WARNING
    if args.log:
        log_lvl = logging.INFO
        # create file handler
        fh = logging.FileHandler(LOG_FILENAME)
        fh.setLevel(log_lvl)
        # configure log formatter
        formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
        fh.setFormatter(formatter)
        logger.addHandler(fh)

    logger.info('Recovery Bootloader CLI version ' + CLI_TOOL_VERSION)

    # Check if erasing flash type is given only with '--program' command.
    if not args.program and (args.chipflasherase or args.chiperaseall or args.sectorerase):
        parser.error('wrong argument combinations: --program command missing but erase type specified')

    try:
        RblDriver = recovery_bl.RecoveryBl(port=args.p, baudrate=115200, argTimeout=args.t, log_level=log_lvl)

        if args.fw_version:
            logger.info("Device R-BL firmware version: " + RblDriver.get_fw_version())

        if args.readuicr:
            filename = args.readuicr
            uicr_addr = recovery_bl.UICR_START_ADDRESS
            uicr_len = RblDriver.uicr_size
            uicr = RblDriver.read_uicr(uicr_addr, uicr_len)
            # Save UICR to file
            with open(filename, 'wb') as f:
                f.write(uicr)
            logger.info('UICR saved to ' + filename)
            return

        if args.eraseall:
            logger.info('Performing erase all...')
            RblDriver.erase_all()
            logger.info('Success.')

        if args.erasepage:
            logger.info('Erasing page ' + args.erasepage)
            page_address = int(args.erasepage, 16)
            RblDriver.erase_page(page_address)
            logger.info('Success.')

        if args.program:
            logger.info('Flashing device with ' + args.program + ' firmware')

            if args.chipflasherase:
                logger.info('Performing all available flash erase...')
                RblDriver.chip_flash_erase()
                logger.info('Done.')

            elif args.chiperaseall:
                logger.info('Performing erase all...')
                RblDriver.erase_all()
                logger.info('Done.')

            elif args.sectorerase:
                logger.info('Performing sector erase...')
                # Sector erasing is default flashing type, no need to do anything here.
                logger.info('Done.')

            logger.info('Programming...')
            RblDriver.flash_hex(args.program)
            logger.info('Done.')

        if args.verify:
            logger.info('Verifying if flashed firmware is the same as ' + args.verify)
            RblDriver.verify_hex(args.verify)
            logger.info('Success.')

        if args.writeuicr:
            logger.info('Writing value ' + args.writeuicr[1] + ' to the UICR register ' + args.writeuicr[0])
            value = struct.pack("<I", int(args.writeuicr[1], 16))
            RblDriver.flash_uicr(value, int(args.writeuicr[0], 16))
            logger.info('Success.')

        if args.protectapp:
            logger.info('Turning on application protection...')
            RblDriver.protect_application()
            logger.info('Success.')

        # after all other commands, optional reset
        if args.reset:
            logger.info('Applying system reset...')
            RblDriver.device_reset()
            logger.info('Success.')

    # Exceptions handling
    except recovery_bl.RecoveryBlCommunicationError as err:
        logger.error("ERROR: Protocol error: " + err.nackResult.name)

    except recovery_bl.RecoveryBlTimeoutError:
        logger.error("ERROR: Communication timeout!")

    except recovery_bl.RecoveryBlVerificationError as err:
        logger.error("ERROR: Verification failed!")

if __name__ == "__main__":
    main()