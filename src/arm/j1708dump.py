#!/usr/bin/env python3
# PLC4TRUCKSDuck (c) 2020 National Motor Freight Traffic Association
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import bitstring
import argparse
import socket
import sys
import time
import functools
from scapy.all import UDP,AsyncSniffer
import queue

#don't buffer output
print = functools.partial(print, flush=True)

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
parser.add_argument('--promiscuous', '-P', action='store_true',
	help='perform j1708 capture in promiscuous mode, dumps without interfering with other j1708 consumers, requires privs')

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

sock=None
q=None
skip=0 # loopback interface duplicates packets, need to skip every second one
scapy_thread=None
def init_source():
	global sock
	global scapy_thread
	global q
	if args.promiscuous:
		q = queue.Queue()
		def doit(x):
			global skip
			if skip == 0:
				q.put(x.load)
				skip = 1
			else:
			    skip = 0

		scapy_thread = AsyncSniffer(iface='lo', filter="udp port %s" % READ_PORT, prn=doit)
		scapy_thread.start()
	else:
		sock = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
		sock.bind(('localhost', READ_PORT))

def get_one_message():
	global sock
	global q
	if args.promiscuous:
		return q.get()
	else:
		return sock.recv(256)

def main():
	init_source()
	while True:
		message = get_one_message()

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

		if messagebits is None:
			messagebits = bitstring.ConstBitArray(message)
		if messagebits.len < 8:
			sys.stderr.write('short frame "%s"\n' % messagebits)
			continue

		comment = ''
		checkbits = get_checksum_bits(message[:-1])
		if checkbits.bytes[0] != messagebits.bytes[-1]:
			if args.validate == 'true':
				continue
			else:
				comment = '; invalid checksum'

		if args.show_checksums == 'false':
			message = message[:-1]
		print("(%.6f) %s %s %s" % (time.monotonic(), args.interface, message.hex(), comment))


if __name__ == '__main__':
	try:
		main()
	except (KeyboardInterrupt):
		pass
	if not scapy_thread is None:
	    scapy_thread.stop()
	    scapy_thread = None
	sys.exit()
