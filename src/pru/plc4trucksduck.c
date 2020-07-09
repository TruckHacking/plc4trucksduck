/* PLC4TRUCKSDuck (c) 2020 National Motor Freight Traffic Association
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#include <stdint.h>

#include <pru_cfg.h>
#include <pru_intc.h>
#include <pru_ctrl.h>

#include <string.h>
#include <stddef.h>
#define compile_time_assert(cond, msg) typedef char msg[(cond) ? 1 : -1]

#define PRU_NO 0

#define RX_PAYLOAD_LEN 4
/* must match the same in plc4trucksduck_host.py */
compile_time_assert(RX_PAYLOAD_LEN == 4, rx_frame_len_must_be_4);

typedef struct  {
    uint8_t volatile length;
    uint8_t volatile payload[RX_PAYLOAD_LEN];
} rx_frame_t;

#define RX_RING_BUFFER_LEN 4
/* must match the same in plc4trucksduck_host.py */
compile_time_assert(RX_RING_BUFFER_LEN == 4, rx_ring_buffer_len_must_be_4);
typedef struct {
    uint32_t volatile produce;
    uint32_t volatile consume;
    rx_frame_t volatile frames[RX_RING_BUFFER_LEN];
} rx_ring_buffer_t;

#define RX_FRAME_SIZE 5
/* must match the same in plc4trucksduck_host.py */
compile_time_assert(RX_FRAME_SIZE == sizeof(rx_frame_t), rx_frame_size_must_be_5);
#define RX_RING_BUFFER_CONSUME_OFFSET 4
/* must match the same in plc4trucksduck_host.py */
compile_time_assert(RX_RING_BUFFER_CONSUME_OFFSET ==
                    offsetof(rx_ring_buffer_t, consume), rx_ring_buf_consume_offset_must_be_4);
#define RX_RING_BUFFER_FRAMES_OFFSET 8
/* must match the same in plc4trucksduck_host.py */
compile_time_assert(RX_RING_BUFFER_FRAMES_OFFSET ==
                    offsetof(rx_ring_buffer_t, frames), rx_ring_buf_frames_offset_must_be_8);

#define TX_PAYLOAD_LEN 321 // payload special bits size for 255 byte payload
/* must match the same in plc4trucksduck_host.py */
compile_time_assert(TX_PAYLOAD_LEN == 321, tx_frame_len_must_be_321);

typedef struct  {
    uint16_t volatile bit_length;
    uint8_t volatile preamble; /* all the bits, without prepended 00, 0 or appended 1*/
    uint8_t volatile payload[TX_PAYLOAD_LEN]; /* all the bits, including checksum, without 11111 prepended or 1111111 appended */
} tx_frame_t;

#define TX_RING_BUFFER_LEN 4
/* must match the same in plc4trucksduck_host.py */
compile_time_assert(TX_RING_BUFFER_LEN == 4, tx_ring_buffer_len_must_be_4);
typedef struct {
    uint32_t volatile produce;
    uint32_t volatile consume;
    tx_frame_t volatile frames[TX_RING_BUFFER_LEN];
} tx_ring_buffer_t;

#define TX_FRAME_SIZE 324
/* must match the same in plc4trucksduck_host.py */
compile_time_assert(TX_FRAME_SIZE == sizeof(tx_frame_t), tx_frame_size_must_be_324);
#define TX_RING_BUFFER_CONSUME_OFFSET 4
/* must match the same in plc4trucksduck_host.py */
compile_time_assert(TX_RING_BUFFER_CONSUME_OFFSET ==
                    offsetof(tx_ring_buffer_t, consume), tx_ring_buf_consume_offset_must_be_4);
#define TX_RING_BUFFER_FRAMES_OFFSET 8
/* must match the same in plc4trucksduck_host.py */
compile_time_assert(TX_RING_BUFFER_FRAMES_OFFSET ==
                    offsetof(tx_ring_buffer_t, frames), tx_ring_buf_frames_offset_must_be_8);
#define TX_FRAME_BIT_LEN_OFFSET 0
/* must match the same in plc4trucksduck_host.py */
compile_time_assert(TX_FRAME_BIT_LEN_OFFSET ==
                    offsetof(tx_frame_t, bit_length), tx_frame_bitlen_offset_must_be_0);
#define TX_FRAME_PREAMBLE_OFFSET 2
/* must match the same in plc4trucksduck_host.py */
compile_time_assert(TX_FRAME_PREAMBLE_OFFSET ==
                    offsetof(tx_frame_t, preamble), tx_frame_preamble_offset_must_be_2);
#define TX_FRAME_PAYLOAD_OFFSET 3
/* must match the same in plc4trucksduck_host.py */
compile_time_assert(TX_FRAME_PAYLOAD_OFFSET ==
                    offsetof(tx_frame_t, payload), tx_frame_payload_offset_must_be_3);

#define RX_RING_BUFFER_VADDR_OFFSET 0
/* must match the same in plc4trucksduck_host.py */
compile_time_assert(RX_RING_BUFFER_VADDR_OFFSET == 0, rx_buf_start_must_be_0);
#define RX_RING_BUFFER_SIZE sizeof(rx_ring_buffer_t)
/* must match the same in plc4trucksduck_host.py */
compile_time_assert(RX_RING_BUFFER_SIZE == 28 , rx_buf_len_must_be_28);
#define TX_RING_BUFFER_VADDR_OFFSET 28
/* must match the same in plc4trucksduck_host.py */
compile_time_assert(TX_RING_BUFFER_VADDR_OFFSET == 28, tx_buf_start_must_be_28);
/* must match the same in plc4trucksduck_host.py */
#define TX_RING_BUFFER_SIZE sizeof(tx_ring_buffer_t)
compile_time_assert(TX_RING_BUFFER_SIZE == 1304, tx_buf_len_must_be_1304);

/* check total size doesn't exceed available 8K */
compile_time_assert(RX_RING_BUFFER_SIZE + TX_RING_BUFFER_SIZE + 438 /*stack size*/ + 0 /*heap size*/ < 8192 /*PRU RAM size*/, bufs_must_be_less_than_ram);

int __inline signal_frame_received();

int __inline receive_frame() {
    int ret = 0;
    return ret;
}

/* returns non-zero if there is a pending frame to send */
int __inline is_frame_to_send() {
    tx_ring_buffer_t *tx_buf = (tx_ring_buffer_t*) TX_RING_BUFFER_VADDR_OFFSET;

    if (tx_buf->produce == tx_buf->consume) {
        return 0;
    }
    return 1;
}

int __inline hw_send_frame(volatile tx_frame_t *msg);

int __inline send_next_frame() {
    tx_ring_buffer_t *tx_buf = (tx_ring_buffer_t*) TX_RING_BUFFER_VADDR_OFFSET;
    int err = 0;

    if (!is_frame_to_send()) {
        return err;
    }

    err = hw_send_frame(tx_buf->frames + tx_buf->consume);

    if (!err) {
        tx_buf->consume = (tx_buf->consume + 1) % TX_RING_BUFFER_LEN;
    }

    return err;
}

volatile register uint32_t __R31;
#if (PRU_NO == 0)
#define MSG_RX_NUM 35
#else
#define MSG_RX_NUM 36
#endif

void __inline interrupt_init() {
    /* enable OCP master port */
    CT_CFG.SYSCFG_bit.STANDBY_INIT = 0;
    /* enable all interrupts */
    CT_INTC.GER = 1<<0;
    /* clear pending interrupts */
    __R31 = 0;
}

#define HOST_NUM 2
#define CHAN_NUM 2

void __inline map_host_interrupts() {
    CT_INTC.HIEISR |= HOST_NUM;
    CT_INTC.HMR0_bit.HINT_MAP_2 = HOST_NUM;
    CT_INTC.CMR4_bit.CH_MAP_19 = CHAN_NUM;
}

int __inline signal_frame_received() {
    __R31 = MSG_RX_NUM;
    return 0;
}

void __inline hw_warmup() {
    return;
}

void __inline hw_init() {
    asm("	clr r30, r30, 1");
    return;
}

void emit_pos_symbol() {
    asm("        SUB      r2, r2, 8");
    asm("        SBBO     &r0, r2, 0, 4");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 240 ; sleep 484 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 232 ; sleep 467 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 224 ; sleep 452 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 217 ; sleep 437 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 211 ; sleep 426 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 205 ; sleep 414 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 200 ; sleep 403 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 195 ; sleep 393 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 190 ; sleep 384 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 186 ; sleep 375 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 182 ; sleep 368 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 178 ; sleep 360 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 175 ; sleep 353 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 171 ; sleep 346 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 168 ; sleep 340 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 165 ; sleep 334 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 162 ; sleep 328 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 160 ; sleep 323 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 157 ; sleep 318 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 154 ; sleep 312 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 153 ; sleep 309 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 150 ; sleep 303 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 148 ; sleep 300 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 146 ; sleep 295 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 144 ; sleep 291 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 142 ; sleep 288 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 140 ; sleep 284 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 138 ; sleep 280 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 137 ; sleep 277 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 135 ; sleep 274 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 133 ; sleep 270 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 132 ; sleep 268 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 130 ; sleep 264 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 129 ; sleep 262 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 128 ; sleep 259 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 126 ; sleep 256 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 125 ; sleep 254 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 123 ; sleep 249 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 161 ; sleep 325 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 269 ; sleep 542 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 294 ; sleep 592 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 289 ; sleep 582 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 285 ; sleep 573 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 280 ; sleep 563 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 276 ; sleep 555 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 271 ; sleep 546 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 267 ; sleep 538 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 264 ; sleep 531 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 260 ; sleep 523 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 257 ; sleep 517 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 253 ; sleep 509 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        CLR      r30, r30, 1");
    // caller is responsible for 504 cycle delay
    asm("        LBBO     &r0, r2, 0, 4");
    asm("        ADD      r2, r2, 8");
}

#define EMIT_POS_SYMBOL_FINAL_CYCLES 504

void emit_neg_symbol() {
    asm("        SUB      r2, r2, 8");
    asm("        SBBO     &r0, r2, 0, 4");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 240 ; sleep 484 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 232 ; sleep 467 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 224 ; sleep 452 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 217 ; sleep 437 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 211 ; sleep 426 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 205 ; sleep 414 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 200 ; sleep 403 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 195 ; sleep 393 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 190 ; sleep 384 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 186 ; sleep 375 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 182 ; sleep 368 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 178 ; sleep 360 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 175 ; sleep 353 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 171 ; sleep 346 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 168 ; sleep 340 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 165 ; sleep 334 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 162 ; sleep 328 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 160 ; sleep 323 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 157 ; sleep 318 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 154 ; sleep 312 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 153 ; sleep 309 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 150 ; sleep 303 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 148 ; sleep 300 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 146 ; sleep 295 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 144 ; sleep 291 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 142 ; sleep 288 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 140 ; sleep 284 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 138 ; sleep 280 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 137 ; sleep 277 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 135 ; sleep 274 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 133 ; sleep 270 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 132 ; sleep 268 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 130 ; sleep 264 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 129 ; sleep 262 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 128 ; sleep 259 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 126 ; sleep 256 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 125 ; sleep 254 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 123 ; sleep 249 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 161 ; sleep 325 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 269 ; sleep 542 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 294 ; sleep 592 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 289 ; sleep 582 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 285 ; sleep 573 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 280 ; sleep 563 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 276 ; sleep 555 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 271 ; sleep 546 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 267 ; sleep 538 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        XOR      r0, r0, r0 ; for even number of sleep cycles");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 264 ; sleep 531 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 260 ; sleep 523 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        SET      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 257 ; sleep 517 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        CLR      r30, r30, 1");
    asm("        .newblock");
    asm("        LDI32    r0, 253 ; sleep 509 cycles total");
    asm("$1:     SUB      r0, r0, 1");
    asm("        QBNE     $1, r0, 0");
    asm("        SET      r30, r30, 1");
    // caller is responsible for 504 cycle delay
    asm("        LBBO     &r0, r2, 0, 4");
    asm("        ADD      r2, r2, 8");
}

#define EMIT_NEG_SYMBOL_FINAL_CYCLES 504

int __inline return_debug_info() {
    rx_ring_buffer_t *rx_buf = (rx_ring_buffer_t*) RX_RING_BUFFER_VADDR_OFFSET;
    int ret = 0;
    uint8_t rx_placebo;

    uint32_t tmp_produce = rx_buf->produce;
    uint32_t tmp_consume = rx_buf->consume;

    rx_placebo = 99;
    if ((tmp_produce + 1) % RX_RING_BUFFER_LEN == tmp_consume) {
        /* Buffer is full
         * TODO return non-zero here */
        rx_placebo = 17;
        ret = 0;
    }

    rx_buf->frames[tmp_produce].payload[0] = rx_placebo;
    rx_buf->frames[tmp_produce].length = 0 + 1;
    rx_buf->produce = (tmp_produce + 1) % RX_RING_BUFFER_LEN;

    signal_frame_received();

    return ret;
}

#define TX_FRAME_PREAMBLE_LEN 8
#define PREAMBLE_EXTRA_CYCLES 2800 // 14us at 200MHz
#define PREAMBLE_TOTAL_CYCLES 22800 // 114us at 200MHz

#define BUS_ACCESS_IDLE_CYCLES (12 * PREAMBLE_TOTAL_CYCLES)

//NB: heavy use of __delay_cycles() in this function results in a large stack save/restore operation
int hw_send_preamble(volatile tx_frame_t *msg) {
    //emit negative preamble symbol
    emit_pos_symbol();
    __delay_cycles(EMIT_POS_SYMBOL_FINAL_CYCLES
                    - 6 // overhead of emit_pos_symbol() call
                    + PREAMBLE_EXTRA_CYCLES // silence time for preamble symbols only
                    );
    //emit negative preamble symbol
    emit_pos_symbol();
    __delay_cycles(EMIT_POS_SYMBOL_FINAL_CYCLES
                    - 6 // overhead of emit_pos_symbol() call
                    + PREAMBLE_EXTRA_CYCLES // silence time for preamble symbols only
                    );
    //emit negative preamble symbol
    emit_pos_symbol();
    __delay_cycles(EMIT_POS_SYMBOL_FINAL_CYCLES
                    - 6 // overhead of emit_pos_symbol() call
                    + PREAMBLE_EXTRA_CYCLES // silence time for preamble symbols only
                    - 8 // overhead of loop initialization and test below
                    );

    for(int i=0; i < TX_FRAME_PREAMBLE_LEN; ++i) {
        if(msg->preamble & (1 << i)) {
            //emit positive preamble symbol
            asm("   clr r30, r30, 1");
            __delay_cycles(PREAMBLE_TOTAL_CYCLES
                            - 14 // loop and test overhead
                            );
        } else {
            //emit negative preamble symbol
            emit_pos_symbol();
            __delay_cycles(EMIT_POS_SYMBOL_FINAL_CYCLES
                            - 6 // overhead of emit_pos_symbol() call
                            + PREAMBLE_EXTRA_CYCLES // silence time for preamble symbols only
                            - 14 // loop and test overhead
                            );
        }
    }

    __delay_cycles(8 // loop and test overhead not executed in last pass
                    );
    //emit positive preamble symbol
    asm("   clr r30, r30, 1");
    //caller is responsible for delay of PREAMBLE_TOTAL_CYCLES
    return 0;
}

void hw_send_payload(volatile tx_frame_t *msg) {
    register uint16_t bit_length = msg->bit_length;

    for(uint8_t i=0; i < 4; ++i) {
        emit_pos_symbol();
        __delay_cycles(EMIT_POS_SYMBOL_FINAL_CYCLES
                        - 2 // loop overhead
                        - 6 // overhead of next emit_pos_symbol() call
                        );
    }
    __delay_cycles(2 /*loop overhead not executed*/);
    emit_pos_symbol();
    __delay_cycles(EMIT_POS_SYMBOL_FINAL_CYCLES
                    - 6 // overhead of emit_pos_symbol() call
                    - 17 // loop and test overhead below
                    );

    for(uint16_t i=0; i < bit_length; ++i) {
        if( msg->payload[ i / 8 ] & (1 << (7-(i % 8))) ) {
            emit_pos_symbol();
            __delay_cycles(EMIT_POS_SYMBOL_FINAL_CYCLES
                - 14 // loop and test overhead
                - 6 // overhead of next emit_pos_symbol() call
                );
        } else {
            emit_neg_symbol();
            __delay_cycles(EMIT_POS_SYMBOL_FINAL_CYCLES
                - 15 // loop and test overhead
                - 6 // overhead of next emit_pos_symbol() call
                );
        }
    }
    // loop exit above is only 2 cycles max, we will ignore

    emit_pos_symbol();
    __delay_cycles(EMIT_POS_SYMBOL_FINAL_CYCLES
                    - 1 // loop setup overhead
                    - 6 // overhead of next emit_pos_symbol() call
                    - 2 // loop exit overhead above adjust
                    );
    for(uint8_t i=0; i < 5; ++i) {
        emit_pos_symbol();
        __delay_cycles(EMIT_POS_SYMBOL_FINAL_CYCLES
                        - 2 // loop overhead
                        - 6 // overhead of next emit_pos_symbol() call
                        );
    }
    emit_pos_symbol();
    //caller is responsible for delay of EMIT_POS_SYMBOL_FINAL_CYCLES
}

int __inline hw_send_frame(volatile tx_frame_t *msg) {
    tx_frame_t local_msg;
    // copy to a local SRAM buffer to avoid non-deterministic and long waits on DDR access during send
    memcpy((void *) &local_msg, (void *) msg, sizeof(tx_frame_t));

    // send preamble
    hw_send_preamble(&local_msg);
    __delay_cycles(PREAMBLE_TOTAL_CYCLES
                - 2 // return from hw_send_preamble overhead
                - 2 // hw_send_payload() call overhead below
                - 1 // first loop setup overhead in hw_send_payload()
                + 40 // custom fudge
                );

    // send payload (and checksum)
    hw_send_payload(&local_msg);
    __delay_cycles(EMIT_POS_SYMBOL_FINAL_CYCLES
                - 2 // return from hw_send_payload overhead
                );
    // sleep for bus access idle time
    asm("   clr r30, r30, 1");
    __delay_cycles(BUS_ACCESS_IDLE_CYCLES);

    return return_debug_info();
}

void ringbuff_init() {
    rx_ring_buffer_t *rx_buf = (rx_ring_buffer_t*) RX_RING_BUFFER_VADDR_OFFSET;
    tx_ring_buffer_t *tx_buf = (tx_ring_buffer_t*) TX_RING_BUFFER_VADDR_OFFSET;

    memset((void *) rx_buf, 0, RX_RING_BUFFER_SIZE);
    memset((void *) tx_buf, 0, TX_RING_BUFFER_SIZE);

    return;
}

void main() {
    ringbuff_init();

    interrupt_init();

    hw_warmup();

    map_host_interrupts();

    hw_init();

    while (1) {
        if (send_next_frame()) {
            break;
        }
    }

    __halt();
}
