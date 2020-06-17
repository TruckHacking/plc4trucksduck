#!/usr/bin/env python3

import bitstring
import argparse
import socket
import sys
import time

default_interface = 'plc' #TODO make this based on exec name

parser = argparse.ArgumentParser(description='frame dumping utility for J1708 and PLC on truckducks')
parser.add_argument('--interface', default=default_interface, const=default_interface,
	nargs='?', choices=['j1708', 'j1708_2', 'plc'],
	help='choose the interface to dump from')
parser.add_argument('--show-checksum', default=False, nargs='?', const=True, help='show frame checksums')
parser.add_argument('--validate', default=True, nargs='?', const=True, help='discard frames with invalid checksums')
parser.add_argument('--hide', nargs='?', action='append', help='specify a candump-like filter; frames matching this filter will be hidden. e.g. "ac:ff" to show only MID 0xAC frames')

args = parser.parse_args()

READ_PORT = 6970
if args.interface == 'j1708_2' or args.interface == 'plc':
	READ_PORT = 6972

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
	with socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM) as sock:
		sock.bind(('localhost', READ_PORT))
		while True:
			message = sock.recv(256)

			if not args.hide is None:
				messagebits = bitstring.ConstBitArray(message)
				skip_this_message = False
				for phil in args.hide:
					(val,mask) = phil.split(":")
					mask = bitstring.ConstBitArray(hex=mask)
					masklen = mask.len
					testmessage = messagebits[:masklen]
					if ( testmessage & mask[:testmessage.len]).hex == val:
						skip_this_message = True
						break

			if skip_this_message:
				continue

			if args.validate:
				if not messagebits is None:
					messagebits = bitstring.ConstBitArray(message)
				checkbits = get_checksum_bits(message[:-1])
				if checkbits.bytes[0] != messagebits.bytes[-1]:
					continue

			if not args.show_checksum:
				message = message[:-1]
			print("(%.6f) %s %s" % (time.monotonic(), args.interface, message.hex()))