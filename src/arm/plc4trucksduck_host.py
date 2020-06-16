#!/usr/bin/env python2
# PLC4TRUCKSDuck (c) 2020 National Motor Freight Traffic Association
#
# PLC4TRUCKSDuck is licensed under a
# Creative Commons Attribution-NonCommercial-NoDerivs 4.0 Unported License.
#
# You should have received a copy of the license along with this
# work.  If not, see <http://creativecommons.org/licenses/by-nc-nd/4.0/>.

from __future__ import print_function

import mmap
import pypruss  # available only in python 2
import select
import signal
import socket
import struct
import sys
import threading
import time
import bitstring

TARGET_PRU_FW = 'plc4trucksduck.bin'
TARGET_PRU_NO = 0
UDP_PORTS = (6971, 6972)

DDR_START = 0x10000000  # 256MiB
DDR_VADDR = 0x4a300000
DDR_SIZE = pypruss.ddr_size()
DDR_END = DDR_START + DDR_SIZE

if TARGET_PRU_NO == 0:
    TARGET_PRU_INTERRUPT = pypruss.PRU0_ARM_INTERRUPT
    TARGET_PRU_PRE_SIZE = 0
else:
    TARGET_PRU_INTERRUPT = pypruss.PRU1_ARM_INTERRUPT
    TARGET_PRU_PRE_SIZE = 8192

SHARED_ADDR = DDR_VADDR + TARGET_PRU_PRE_SIZE
SHARED_OFFSET = SHARED_ADDR - DDR_START
SHARED_FILELEN = DDR_SIZE + DDR_START

TX_FRAME_LEN = 42  # must match the same in plc4trucksduck.c
TX_FRAME_SIZE = TX_FRAME_LEN + 2 # must match the same in plc4trucksduck.c
TX_RING_BUFFER_LEN = 16  # must match the same in plc4trucksduck.c
TX_RING_BUFFER_CONSUME_OFFSET = 4  # must match the same in plc4trucksduck.c
TX_RING_BUFFER_FRAMES_OFFSET = 8  # must match the same in plc4trucksduck.c
TX_FRAME_BIT_LEN_OFFSET = 0  # must match the same in plc4trucksduck.c
TX_FRAME_PREAMBLE_OFFSET = 1  # must match the same in plc4trucksduck.c
TX_FRAME_PAYLOAD_OFFSET = 2  # must match the same in plc4trucksduck.c

RX_FRAME_LEN = 4  # must match the same in plc4trucksduck.c
RX_FRAME_SIZE = RX_FRAME_LEN + 1 # must match the same in plc4trucksduck.c
RX_RING_BUFFER_LEN = 4  # must match the same in plc4trucksduck.c
RX_RING_BUFFER_CONSUME_OFFSET = 4  # must match the same in plc4trucksduck.c
RX_RING_BUFFER_FRAMES_OFFSET = 8  # must match the same in plc4trucksduck.c

SHARED_RECEIVE_FRAME_LEN = RX_FRAME_LEN
SHARED_SEND_CIRC_BUF_SIZE = TX_RING_BUFFER_LEN
SHARED_RECEIVE_CIRC_BUF_SIZE = RX_RING_BUFFER_LEN

SHARED_RECEIVE_BUF_OFFSET = 0  # must match the same in plc4trucksduck.c
SHARED_RECEIVE_BUF_SIZE = 28  # must match the same in plc4trucksduck.c
SHARED_SEND_BUF_OFFSET = 28  # must match the same in plc4trucksduck.c

def get_checksum_bits(payload):
    msg = str(bitstring.ConstBitArray(bytes=payload).bin)
    checksum = 0
    for n in range(0,len(msg),8):
        checksum = checksum + int(msg[n:n+8],2)

    # Two's Complement (10)
    binint = int("{0:b}".format(checksum))             # Convert to binary (1010)
    flipped = ~binint                                  # Flip the bits (-1011)
    flipped += 1                                       # Add one (two's complement method) (-1010)
    intflipped=int(str(flipped),2)                     # Back to int (-10)
    intflipped = ((intflipped + (1 << 8)) % (1 << 8))  # Over to binary (246) <-- .uint
    intflipped = '{0:08b}'.format(intflipped)          # Format to one byte (11110110) <-- same as -10.bin

    checksum_bits = bitstring.BitArray(bin=intflipped)
    return checksum_bits

def get_special_preamble_bits(preamble_mid):
    mid_bits = bitstring.BitArray(bytes=preamble_mid)
    return mid_bits

def get_special_payload_bits(payload):
    payload_bits = bitstring.BitArray()

    for b_int in bytes(payload):
        b_bits = bitstring.BitArray(bytes=b_int)
        b_bits.reverse()

        payload_bits.append(bitstring.ConstBitArray(bin='0')) # start bit
        payload_bits.append(b_bits) # bit-reversed byte
        payload_bits.append(bitstring.ConstBitArray(bin='1')) # stop bit

    checksum_bits = get_checksum_bits(payload)
    checksum_bits.reverse()

    payload_bits.append(bitstring.ConstBitArray(bin='0'))
    payload_bits.append(checksum_bits)
    payload_bits.append(bitstring.ConstBitArray(bin='1'))
    return payload_bits

class PRU_read_thread(threading.Thread):
    def __init__(self, stopped, socket, ddr_mem):
        super(PRU_read_thread, self).__init__()
        self.ddr_mem = ddr_mem

        self.struct_start = DDR_START + SHARED_RECEIVE_BUF_OFFSET
        self.frames_base = self.struct_start + RX_RING_BUFFER_FRAMES_OFFSET
        self.frames_ptr = self.frames_base

        self.calls = 0
        self.socket = socket

        self.stopped = stopped

    def kill_me(self):
        self.stopped.set()

    def join(self, timeout=None):
        super(PRU_read_thread, self).join(timeout)
        data = self.ddr_mem[DDR_START:DDR_START + SHARED_RECEIVE_BUF_SIZE]
        msg = map(lambda x: "{:02x}".format(ord(x)), data)
        for i in range(8, len(msg), RX_FRAME_SIZE):
            print(",".join(msg[i:i + RX_FRAME_SIZE]))

    def run(self):
        old_consume = 0
        while not self.stopped.is_set():
            pypruss.wait_for_event(TARGET_PRU_NO)
            pypruss.clear_event(TARGET_PRU_NO, TARGET_PRU_INTERRUPT)
            self.calls += 1
            (produce, consume) = \
                struct.unpack("LL", self.ddr_mem[DDR_START:DDR_START +
                                                 RX_RING_BUFFER_FRAMES_OFFSET])
            while consume != produce:
                length = struct.unpack("B", self.ddr_mem[self.frames_ptr])[0]
                frame = \
                    struct.unpack("B"*length,
                                  self.ddr_mem[self.frames_ptr+1:
                                               self.frames_ptr+1+length])

                #TODO remote debug prints
                sys.stderr.write('rx ' + str(frame) + '\n')
                self.socket.sendto(''.join(map(chr, frame)),
                                   ('localhost', UDP_PORTS[1]))

                consume = (consume + 1) % SHARED_RECEIVE_CIRC_BUF_SIZE
                self.frames_ptr = self.frames_base + \
                    (consume * RX_FRAME_SIZE)
            if old_consume != consume:
                self.ddr_mem[DDR_START + RX_RING_BUFFER_CONSUME_OFFSET:
                             DDR_START + RX_RING_BUFFER_FRAMES_OFFSET] = \
                             struct.pack('L', consume)
                old_consume = consume


class PRU_write_thread(threading.Thread):
    def __init__(self, stopped, socket, ddr_mem):
        super(PRU_write_thread, self).__init__()
        self.ddr_mem = ddr_mem

        self.struct_start = DDR_START + SHARED_SEND_BUF_OFFSET
        self.frames_base = self.struct_start + TX_RING_BUFFER_FRAMES_OFFSET
        self.frames_ptr = self.frames_base

        self.socket = socket

        self.stopped = stopped

    def kill_me(self):
        self.stopped.set()

    def join(self, timeout=None):
        super(PRU_write_thread, self).join(timeout)

    def run(self):
        while not self.stopped.is_set():
            ready = select.select([self.socket], [], [], 0.5)[0]
            if ready == []:
                continue

            frame = self.socket.recv(256)
            (produce, consume) = \
                struct.unpack('LL',
                              self.ddr_mem[self.struct_start:
                                           self.struct_start +
                                           TX_RING_BUFFER_FRAMES_OFFSET])

            while (produce + 1) % SHARED_SEND_CIRC_BUF_SIZE == consume:
                sys.stderr.write("buffer full, waiting\n")
                time.sleep(0.003)
                (produce, consume) = \
                    struct.unpack('LL',
                                  self.ddr_mem[self.struct_start:
                                               self.struct_start +
                                               TX_RING_BUFFER_FRAMES_OFFSET])
            if len(frame) > TX_FRAME_LEN:
                frame = frame[:TX_FRAME_LEN]

            preamble_bits = get_special_preamble_bits(frame[0])
            preamble_byte = preamble_bits.tobytes()[0]
            payload_bits = get_special_payload_bits(frame)
            payload_bytes = payload_bits.tobytes()

            len_byte = struct.pack('B', payload_bits.len)
            self.ddr_mem[self.frames_ptr + TX_FRAME_BIT_LEN_OFFSET] = len_byte
            self.ddr_mem[self.frames_ptr + TX_FRAME_PREAMBLE_OFFSET] = preamble_byte
            frame_offset = self.frames_ptr + TX_FRAME_PAYLOAD_OFFSET
            self.ddr_mem[frame_offset:frame_offset + len(payload_bytes)] = \
                payload_bytes

            produce = (produce + 1) % SHARED_SEND_CIRC_BUF_SIZE
            self.frames_ptr = \
                self.frames_base + (produce * TX_FRAME_SIZE)
            self.ddr_mem[self.struct_start:self.struct_start +
                         TX_RING_BUFFER_CONSUME_OFFSET] = \
                struct.pack('L', produce)
            #TODO remove debug prints
            sys.stderr.write("tx preamble:%s payload:%s bit_length:%d\n" % (preamble_bits, payload_bits, ord(len_byte)))

class PRU_pump(threading.Thread):
    def __init__(self, stopped, socket):
        super(PRU_pump, self).__init__()
        self.socket = socket

        self.stopped = stopped

    def kill_me(self):
        self.stopped.set()

    def join(self, timeout=None):
        super(PRU_pump, self).join(timeout)

    def run(self):
        while not self.stopped.is_set():
            time.sleep(2.0)
            self.socket.sendto(b'\x0a\x00', ('localhost', UDP_PORTS[0]))


pypruss.modprobe()

sock = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)

try:
    sock.bind(('localhost', UDP_PORTS[0]))
except OSError as e:
    print(e)
    sys.exit(-1)

f = open("/dev/mem", "r+b")
shared_mem = mmap.mmap(f.fileno(), SHARED_FILELEN, offset=SHARED_OFFSET)

if TARGET_PRU_NO == 1:
    pypruss.init()

pypruss.open(TARGET_PRU_NO)

if TARGET_PRU_NO == 1:
    pypruss.pruintc_init()

stopped = threading.Event()
stopped.clear()
pru_stop_thread = PRU_read_thread(stopped, sock, shared_mem)
pru_send_thread = PRU_write_thread(stopped, sock, shared_mem)
pru_stop_thread.start()
pru_send_thread.start()

pru_pump_thread = PRU_pump(stopped, sock)
pru_pump_thread.start()

pypruss.exec_program(TARGET_PRU_NO, TARGET_PRU_FW)


def signal_handler(signal, frame):
    pru_stop_thread.kill_me()
    pru_send_thread.kill_me()
    pru_pump_thread.kill_me()
    pru_stop_thread.join()
    pru_send_thread.join()
    pru_pump_thread.join()


signal.signal(signal.SIGINT, signal_handler)

pru_stop_thread.join()
pru_send_thread.join()
pru_pump_thread.join()

pypruss.exit()
