import sys, os, argparse, intelhex, re, struct, binascii, collections, hashlib

HEADER_MAGIC_VALUE = 0x7fdeb13c
TYPE_MODEM_UNSIGNED_PKG = 0x01
TYPE_MODEM_ECDSA_SIGNED_PKG = 0x02
TYPE_MODEM_BL_SEGMENT = 0x03
TYPE_MODEM_FW_SEGMENT = 0x04
TYPE_MODEM_FW_SEGMENT_W_DIGEST = 0x05

def bl_segment_encode(filename):
        digest = re.findall(r'([A-Fa-f0-9]+).ipc_dfu.*ihex', filename)
        if len(digest) != 1:
            raise Exception('Root digest not found in bootloader filename')

        digest = int(digest[0], 16)
        ih = intelhex.IntelHex(filename)
        if len(ih.segments()) > 1:
            raise NotImplementedError('Only one bootloader segment expected')

        start_address = ih.segments()[0][0]
        data_length = ih.segments()[0][1] - ih.segments()[0][0]

        header = bytes()
        header += struct.pack('<I', TYPE_MODEM_BL_SEGMENT)
        header += struct.pack('<I', data_length)
        header += struct.pack('<I', start_address)
        header += struct.pack('<I', digest)

        log = ''
        log += 'Encoded BL segment ({}):\n'.format(filename)
        log += '(header length: {} bytes)\n'.format(len(header))
        log += '  Type: {}\n'.format(TYPE_MODEM_BL_SEGMENT)
        log += '  Data length: {} bytes\n'.format(data_length)
        log += '  Data address: 0x{:08X}\n'.format(start_address)
        log += '  Digest: 0x{:08X}\n'.format(digest)

        return header + ih.tobinstr(), log

def fw_segment_encode(filename_hex, filename_digest):
    digests = {}
    if filename_digest is not None:
        # Read all digests from text file and map to hex address range
        with open(filename_digest, 'r') as fin:
            p = re.compile(r'Range.*(0x[0-9A-Fa-f]+)[-]+(0x[0-9A-Fa-f]+).*SHA256:[ ]*([0-9A-Fa-f]+)')
            for line in fin:
                if re.match(p, line):
                    seg_start, seg_end, digest = re.findall(p, line)[0]
                    digests[(int(seg_start, 16), int(seg_end, 16) + 1)] = binascii.unhexlify(digest)
    
    ih = intelhex.IntelHex(filename_hex)

    # Verify that address range for digests (if any) match segments of hex file
    if len(digests.keys()) != 0 and len(digests.keys()) != len(ih.segments()):
        raise Exception('{} does not cover all segments in {}'.format(filename_digest, filename_hex))
    
    if len(digests.keys()) != 0 and set(digests.keys()) != set(ih.segments()):
        raise Exception('{} does not contain the same segments as {}'.format(filename_digest, filename_hex))

    encoded = bytes()
    log = ''

    for segment in ih.segments():
        segment_len = segment[1] - segment[0]

        if segment in digests:
            header_type = TYPE_MODEM_FW_SEGMENT_W_DIGEST
            # Convert endianness to comply with modem bootloader format
            digest_be = struct.unpack('>IIIIIIII', digests[segment])
            digest = struct.pack('<IIIIIIII', *digest_be)
        else:
            header_type = TYPE_MODEM_FW_SEGMENT
            digest = bytes()

        header = bytes()
        header += struct.pack('<I', header_type)
        header += struct.pack('<I', segment_len)
        header += struct.pack('<I', segment[0])
        header += digest

        encoded += header
        encoded += ih.tobinstr(start=segment[0], size=segment_len)

        log += 'Encoded FW segment ({}):\n'.format(filename_hex)
        log += '(header length: {} bytes)\n'.format(len(header))
        log += '  Type: {}\n'.format(header_type)
        log += '  Data length: {} bytes\n'.format(segment_len)
        log += '  Address: 0x{:08X}\n'.format(segment[0])
        log += '  Digest (endianness converted): {}\n'.format('None' if len(digest) == 0 else binascii.hexlify(digest).decode('utf-8'))

    return encoded, log

def pkg_header_encode(encoded_data, sk):
    if sk is None:
        pkg_type = TYPE_MODEM_UNSIGNED_PKG
    else:
        pkg_type = TYPE_MODEM_ECDSA_SIGNED_PKG

    header = bytes()
    header += struct.pack('<I', HEADER_MAGIC_VALUE)
    header += struct.pack('<I', pkg_type)
    header += struct.pack('<I', len(encoded_data))
    # SHA256 is generated for package header (minus hash itself) + encoded data
    pkg_hash = hashlib.sha256(header + encoded_data).digest()
    header += pkg_hash

    if sk is not None:
        signature = sk.sign(header, hashfunc=hashlib.sha256, sigencode=ecdsa.keys.sigencode_string)
        header += signature
    else:
        signature = bytes()

    log = ''
    log += 'Encoded package header:\n'.format(len(header))
    log += '(header length: {} bytes)\n'.format(len(header))
    log += '  Type: {}\n'.format(pkg_type)
    log += '  Package length: {} bytes\n'.format(len(encoded_data))
    log += '  SHA256: {}\n'.format(binascii.hexlify(pkg_hash).decode('utf-8'))
    log += '  ECDSA: {}\n'.format(binascii.hexlify(signature).decode('utf-8'))

    return header, log

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Modem FOTA package generator.')

    parser.add_argument('-b',
        '--bootloader',
        required=True,
        dest='bootloader',
        type=str,
        default=None,
        help='Bootloader ihex file.')

    parser.add_argument('-s',
        '--segments',
        dest='segments',
        type=str,
        default=None,
        nargs='+',
        help='List of segment hex files and digest files. E.g. --segments segment0.hex segment1.hex,digest1.txt')

    parser.add_argument('-sk',
        '--private-key',
        required=False,
        dest='sk',
        type=str,
        default=None,
        help='Pem file containing private key used to sign package header (optional).')

    parser.add_argument('-o',
        '--output',
        required=True,
        dest='output',
        type=str,
        default=None,
        help='Name of binary output file.')

    args = parser.parse_args()

    privkey = None
    if args.sk is not None:
        try:
            import ecdsa
        except Exception:
            print("Failed to import ecdsa, cannot do signing")
            exit(-1)

        with open(args.sk, 'r') as fin:
            privkey = ecdsa.SigningKey.from_pem(fin.read())

    if privkey is None:
        print('===================================================')
        print('WARNING: Unsigned update package is being generated')
        print('===================================================')

    packed_update = bytes()
    content_description = ''

    # Encode bootloader
    segment, desc = bl_segment_encode(args.bootloader)
    packed_update += segment
    content_description += desc

    # Encode segment hex files
    for i, hexdigest_pair in enumerate(args.segments):
        try:
            filename_hex, filename_digest = hexdigest_pair.split(',')
        except ValueError:
            filename_hex = hexdigest_pair
            filename_digest = None
        segment, desc = fw_segment_encode(filename_hex, filename_digest)
        packed_update += segment
        content_description += desc
    
    # Encode header (done last due to sha digest calculation)
    update_header, header_desc = pkg_header_encode(packed_update, sk=privkey)

    with open(args.output, 'wb') as fout:
        fout.write(update_header)
        fout.write(packed_update)

    with open(args.output + '.txt', 'w') as fout:
        s = '{} contains the following elements:\n'.format(args.output)
        fout.write(s)
        fout.write('=' * len(s) + '\n')
        fout.write(header_desc)
        fout.write(content_description)

    print('Wrote {} bytes to {}'.format(len(update_header) + len(packed_update), args.output))
