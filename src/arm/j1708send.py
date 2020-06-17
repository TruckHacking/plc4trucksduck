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

args = parser.parse_args()

SEND_PORT = 6969
if args.interface == 'j1708_2' or args.interface == 'plc':
	SEND_PORT = 6971

if __name__ == '__main__':
	hexinput = args.hexbytes.split(' ')[-1]
	hexinput = hexinput.replace(',','')
	hexinput = hexinput.replace('#','')
	message = bitstring.ConstBitArray(hex=hexinput)
	with socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM) as sock:
		sock.sendto(message.bytes, ('localhost', SEND_PORT))