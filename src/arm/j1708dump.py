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
parser.add_argument('--show-checksums', default='false', const='true',
	nargs='?', choices=['true', 'false'],
	help='show frame checksums')
parser.add_argument('--validate', default='true', const='true',
	nargs='?', choices=['true', 'false'],
	help='discard frames with invalid checksums')
parser.add_argument('--show', nargs='?', action='append',
	help='specify a candump-like filter; frames matching this filter will be shown. Processed before hide filters. e.g. "ac:ff" to show only MID 0xAC frames')
parser.add_argument('--hide', nargs='?', action='append',
	help='specify a candump-like filter; frames matching this filter will be hidden. e.g. "89:ff" to hide MID 0x89 frames')

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

def get_filter_val_and_mask(phil):
	philsplit = phil.split(':')
	val = philsplit[0]
	if len(philsplit) < 2:
		mask = 'f' * len(val)
	else:
		mask = philsplit[1]
	return (val, mask)

def is_filter_applies(phil, messagebits):
	(val, mask) = get_filter_val_and_mask(phil)
	mask = bitstring.ConstBitArray(hex=mask)
	masklen = mask.len
	testmessage = messagebits[:masklen]
	return (testmessage & mask[:testmessage.len]).hex == val

if __name__ == '__main__':
	with socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM) as sock:
		sock.bind(('localhost', READ_PORT))
		while True:
			message = sock.recv(256)

			skip_this_message = False
			messagebits = None

			if not args.show is None:
				skip_this_message = True

				if messagebits is None:
					messagebits = bitstring.ConstBitArray(message)
				for phil in args.show:
					if is_filter_applies(phil, messagebits):
						skip_this_message = False
						break

			if not args.hide is None:
				if messagebits is None:
					messagebits = bitstring.ConstBitArray(message)
				for phil in args.hide:
					if is_filter_applies(phil, messagebits):
						skip_this_message = True
						break

			if skip_this_message:
				continue

			if args.validate == 'true':
				if messagebits is None:
					messagebits = bitstring.ConstBitArray(message)
				checkbits = get_checksum_bits(message[:-1])
				if checkbits.bytes[0] != messagebits.bytes[-1]:
					continue

			if args.show_checksums == 'false':
				message = message[:-1]
			print("(%.6f) %s %s" % (time.monotonic(), args.interface, message.hex()))