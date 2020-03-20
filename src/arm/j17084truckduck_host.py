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

TARGET_PRU_FW = 'j17084truckduck.bin'
TARGET_PRU_NO = 0
TARGET_PRU_INTERRUPT = pypruss.PRU0_ARM_INTERRUPT
TARGET_PRU_PRE_SIZE = 0
UDP_PORTS = (6969, 6970)

DDR_START = 0x10000000  # 256MiB
DDR_VADDR = 0x4a300000
DDR_SIZE = pypruss.ddr_size()
DDR_END = DDR_START + DDR_SIZE

SHARED_ADDR = DDR_VADDR + TARGET_PRU_PRE_SIZE
SHARED_OFFSET = SHARED_ADDR - DDR_START
SHARED_FILELEN = DDR_SIZE + DDR_START

FRAME_LEN = 42  # must match the same in j17084truckduck.c
RING_BUFFER_LEN = 16  # must match the same in j17084truckduck.c
RING_BUFFER_CONSUME_OFFSET = 4  # must match the same in j17084truckduck.c
RING_BUFFER_FRAMES_OFFSET = 8  # must match the same in j17084truckduck.c

SHARED_SEND_FRAME_LEN = FRAME_LEN
SHARED_RECEIVE_FRAME_LEN = FRAME_LEN
SHARED_SEND_CIRC_BUF_SIZE = RING_BUFFER_LEN
SHARED_RECEIVE_CIRC_BUF_SIZE = RING_BUFFER_LEN

SHARED_RECEIVE_BUF_OFFSET = 0  # must match the same in j17084truckduck.c
SHARED_RECEIVE_BUF_LEN = 696  # must match the same in j17084truckduck.c
SHARED_SEND_BUF_OFFSET = 704  # must match the same in j17084truckduck.c


class PRU_read_thread(threading.Thread):
    def __init__(self, stopped, socket, ddr_mem):
        super(PRU_read_thread, self).__init__()
        self.ddr_mem = ddr_mem

        self.struct_start = DDR_START + SHARED_RECEIVE_BUF_OFFSET
        self.frames_base = self.struct_start + RING_BUFFER_FRAMES_OFFSET
        self.frames_ptr = self.frames_base

        self.calls = 0
        self.socket = socket

        self.stopped = stopped

    def kill_me(self):
        self.stopped.set()

    def join(self, timeout=None):
        super(PRU_read_thread, self).join(timeout)
        data = self.ddr_mem[DDR_START:DDR_START + SHARED_RECEIVE_BUF_LEN]
        msg = map(lambda x: "{:02x}".format(ord(x)), data)
        for i in range(8, len(msg), SHARED_RECEIVE_FRAME_LEN + 1):
            print(",".join(msg[i:i + SHARED_RECEIVE_FRAME_LEN + 1]))

    def run(self):
        old_consume = 0
        while not self.stopped.is_set():
            pypruss.wait_for_event(TARGET_PRU_NO)
            pypruss.clear_event(TARGET_PRU_NO, TARGET_PRU_INTERRUPT)
            self.calls += 1
            (produce, consume) = \
                struct.unpack("LL", self.ddr_mem[DDR_START:DDR_START +
                                                 RING_BUFFER_FRAMES_OFFSET])
            while consume != produce:
                length = struct.unpack("B", self.ddr_mem[self.frames_ptr])[0]
                frame = \
                    struct.unpack("B"*length,
                                  self.ddr_mem[self.frames_ptr+1:
                                               self.frames_ptr+1+length])
                self.socket.sendto(''.join(map(chr, frame)),
                                   ('localhost', UDP_PORTS[1]))
                consume = (consume + 1) % SHARED_RECEIVE_CIRC_BUF_SIZE
                self.frames_ptr = self.frames_base + \
                    (consume * (SHARED_RECEIVE_FRAME_LEN + 1))
            if old_consume != consume:
                self.ddr_mem[DDR_START + RING_BUFFER_CONSUME_OFFSET:
                             DDR_START + RING_BUFFER_FRAMES_OFFSET] = \
                             struct.pack('L', consume)
                old_consume = consume


class PRU_write_thread(threading.Thread):
    def __init__(self, stopped, socket, ddr_mem):
        super(PRU_write_thread, self).__init__()
        self.ddr_mem = ddr_mem

        self.struct_start = DDR_START + SHARED_SEND_BUF_OFFSET
        self.frames_base = self.struct_start + RING_BUFFER_FRAMES_OFFSET
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
                                           RING_BUFFER_FRAMES_OFFSET])
            if (produce + 1) % SHARED_SEND_CIRC_BUF_SIZE == consume:
                sys.stderr.write("buffer full\n")
                # the buffer is full so drop the frame
                continue
            if len(frame) > SHARED_SEND_FRAME_LEN:
                frame = frame[:SHARED_SEND_FRAME_LEN]

            len_byte = struct.pack('B', len(frame))
            self.ddr_mem[self.frames_ptr] = len_byte
            frame_offset = self.frames_ptr + 1
            self.ddr_mem[frame_offset:frame_offset + len(frame)] = \
                frame
            produce = (produce + 1) % SHARED_SEND_CIRC_BUF_SIZE
            self.frames_ptr = \
                self.frames_base + (produce * (SHARED_SEND_FRAME_LEN + 1))
            self.ddr_mem[self.struct_start:self.struct_start +
                         RING_BUFFER_CONSUME_OFFSET] = \
                struct.pack('L', produce)


pypruss.modprobe()

sock = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)

try:
    sock.bind(('localhost', UDP_PORTS[0]))
except OSError as e:
    print(e)
    sys.exit(-1)

f = open("/dev/mem", "r+b")
shared_mem = mmap.mmap(f.fileno(), SHARED_FILELEN, offset=SHARED_OFFSET)

if TARGET_PRU_NO == 0:
    pypruss.init()

pypruss.open(TARGET_PRU_NO)

if TARGET_PRU_NO == 0:
    pypruss.pruintc_init()

stopped = threading.Event()
stopped.clear()
pru_stop_thread = PRU_read_thread(stopped, sock, shared_mem)
pru_send_thread = PRU_write_thread(stopped, sock, shared_mem)
pru_stop_thread.start()
pru_send_thread.start()

pypruss.exec_program(TARGET_PRU_NO, TARGET_PRU_FW)


def signal_handler(signal, frame):
    pru_stop_thread.kill_me()
    pru_send_thread.kill_me()
    pru_stop_thread.join()
    pru_send_thread.join()


signal.signal(signal.SIGINT, signal_handler)

pru_stop_thread.join()
pru_send_thread.join()

pypruss.exit()
