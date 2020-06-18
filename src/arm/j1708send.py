#!/usr/bin/env python3

import bitstring
import argparse
import socket
import sys

default_interface = 'plc' #TODO make this based on exec name

parser = argparse.ArgumentParser(description='frame sending utility for J1708 and PLC on truckducks')
parser.add_argument('hexbytes', help="a j1708 or plc message to send e.g. '0a00' or '0a,00' or '0a#00' or '(123.123) j1708 0a#00'")
parser.add_argument('--interface', default=default_interface, const=default_interface,
	nargs='?', choices=['j1708', 'j1708_2', 'plc'],
	help='choose the interface to send frames to')
parser.add_argument('--checksums', default='true', const='true',
	nargs='?', choices=['true','false'],
	help="add checksums to frames sent")

args = parser.parse_args()

SEND_PORT = 6969
if args.interface == 'j1708_2' or args.interface == 'plc':
	SEND_PORT = 6971


def get_checksum_bits(payload):
    msg = str(bitstring.ConstBitArray(bytes=payload).bin)
    checksum = 0
    for n in range(0,len(msg),8):
        checksum = checksum + int(msg[n:n+8],2)

    # Two's Complement (10)
    binint = int("{0:b}".format(checksum))             # Convert to binary (1010)
    flipped = ~binint                                  # Flip the bits (-1011)
    flipped += 1                                       # Add one (two's complement method) (-1010)
    intflipped = int(str(flipped),2)                     # Back to int (-10)
    intflipped = ((intflipped + (1 << 8)) % (1 << 8))  # Over to binary (246) <-- .uint
    intflipped = '{0:08b}'.format(intflipped)          # Format to one byte (11110110) <-- same as -10.bin

    checksum_bits = bitstring.ConstBitArray(bin=intflipped)
    return checksum_bits


if __name__ == '__main__':
	hexinput = args.hexbytes.split(' ')[-1]
	hexinput = hexinput.replace(',','')
	hexinput = hexinput.replace('#','')
	message = bitstring.BitArray(hex=hexinput)
	if args.checksums == 'true':
		checksum = get_checksum_bits(message.bytes)
		message.append(checksum)
	with socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM) as sock:
		sock.sendto(message.bytes, ('localhost', SEND_PORT))