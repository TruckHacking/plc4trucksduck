/* PLC4TRUCKSDuck (c) 2020 National Motor Freight Traffic Association
*
* PLC4TRUCKSDuck is licensed under a
* Creative Commons Attribution-NonCommercial-NoDerivs 4.0 Unported License.
*
* You should have received a copy of the license along with this
* work.  If not, see <http://creativecommons.org/licenses/by-nc-nd/4.0/>.
*/

#include <stdint.h>

#include <pru_cfg.h>
#include <pru_intc.h>
#include <pru_ctrl.h>

#include <string.h>
#include <stddef.h>
#define compile_time_assert(cond, msg) typedef char msg[(cond) ? 1 : -1]

#define PRU_NO 0

#define RX_FRAME_LEN 4
typedef struct  {
    uint8_t volatile length;
    uint8_t volatile payload[RX_FRAME_LEN];
} rx_frame_t;

#define RX_RING_BUFFER_LEN 4
typedef struct {
    uint32_t volatile produce;
    uint32_t volatile consume;
    rx_frame_t volatile frames[RX_RING_BUFFER_LEN];
} rx_ring_buffer_t;

#define TX_FRAME_LEN 42
/* must match the same in plc4trucksduck_host.py */
compile_time_assert(TX_FRAME_LEN == 42, tx_frame_len_must_be_42);

#define TX_FRAME_PREAMBLE_LEN 8

typedef struct  {
    uint8_t volatile bit_length;
    uint8_t volatile preamble; /* all the bits, without prepended 00, 0 or appended 1*/
    uint8_t volatile payload[TX_FRAME_LEN]; /* all the bits, including checksum, without 11111 prepended or 1111111 appended */
} tx_frame_t;

#define TX_RING_BUFFER_LEN 16
typedef struct {
    uint32_t volatile produce;
    uint32_t volatile consume;
    tx_frame_t volatile frames[TX_RING_BUFFER_LEN];
} tx_ring_buffer_t;

#define CHIRP_SLEEPS_LEN 52
typedef struct {
    uint8_t length;
    uint16_t sleeps[CHIRP_SLEEPS_LEN];
} chirp_sleeps_t;
#define CHIRP_SLEEPS_SIZE sizeof(chirp_sleeps_t)

#define RX_RING_BUFFER_CONSUME_OFFSET 4
/* must match the same in plc4trucksduck_host.py */
compile_time_assert(RX_RING_BUFFER_CONSUME_OFFSET ==
                    offsetof(rx_ring_buffer_t, consume), rx_ring_buf_consume_offset_must_be_4);

#define RX_RING_BUFFER_FRAMES_OFFSET 8
/* must match the same in plc4trucksduck_host.py */
compile_time_assert(RX_RING_BUFFER_FRAMES_OFFSET ==
                    offsetof(rx_ring_buffer_t, frames), rx_ring_buf_frames_offset_must_be_8);

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

#define TX_FRAME_PREAMBLE_OFFSET 1
/* must match the same in plc4trucksduck_host.py */
compile_time_assert(TX_FRAME_PREAMBLE_OFFSET ==
                    offsetof(tx_frame_t, preamble), tx_frame_preamble_offset_must_be_1);

#define TX_FRAME_PAYLOAD_OFFSET 2
/* must match the same in plc4trucksduck_host.py */
compile_time_assert(TX_FRAME_PAYLOAD_OFFSET ==
                    offsetof(tx_frame_t, payload), tx_frame_payload_offset_must_be_2);

#define SHARED_RECEIVE_BUF_OFFSET 0
/* must match the same in plc4trucksduck_host.py */
compile_time_assert(SHARED_RECEIVE_BUF_OFFSET == 0, rx_buf_start_must_be_0);
#define SHARED_RECEIVE_BUF_SIZE sizeof(rx_ring_buffer_t)
/* must match the same in plc4trucksduck_host.py */
compile_time_assert(SHARED_RECEIVE_BUF_SIZE == 28 , rx_buf_len_must_be_28);

#define SHARED_SEND_BUF_OFFSET 28
/* must start after the receive buffer */
compile_time_assert(SHARED_SEND_BUF_OFFSET >= SHARED_RECEIVE_BUF_SIZE, tx_buf_must_be_after_rx_buf);
/* must match the same in plc4trucksduck_host.py */
compile_time_assert(SHARED_SEND_BUF_OFFSET == 28, tx_buf_start_must_be_28);
/* must match the same in plc4trucksduck_host.py */
#define SHARED_SEND_BUF_SIZE sizeof(tx_ring_buffer_t)
compile_time_assert(SHARED_SEND_BUF_SIZE == 712, tx_buf_len_must_be_696);

compile_time_assert(SHARED_RECEIVE_BUF_SIZE + SHARED_SEND_BUF_SIZE + CHIRP_SLEEPS_SIZE + 256 /*stack size*/ + 0 /*heap size*/ < 8192 /*PRU RAM size*/, bufs_must_be_less_than_ram);

int __inline signal_frame_received();

int __inline receive_frame() {
    int ret = 0;
    return ret;
}

/* returns non-zero if there is a pending frame to send */
int __inline is_frame_to_send() {
    tx_ring_buffer_t *tx_buf = (tx_ring_buffer_t*) SHARED_SEND_BUF_OFFSET;

    if (tx_buf->produce == tx_buf->consume) {
        return 0;
    }
    return 1;
}

int __inline hw_send_frame(volatile tx_frame_t *msg);

int __inline send_next_frame() {
    tx_ring_buffer_t *tx_buf = (tx_ring_buffer_t*) SHARED_SEND_BUF_OFFSET;
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

void __inline emit_pos_symbol() {
    asm("   set r30, r30, 1");
    __delay_cycles(484);
    asm("   clr r30, r30, 1");
    __delay_cycles(467);
    asm("   set r30, r30, 1");
    __delay_cycles(452);
    asm("   clr r30, r30, 1");
    __delay_cycles(437);
    asm("   set r30, r30, 1");
    __delay_cycles(426);
    asm("   clr r30, r30, 1");
    __delay_cycles(414);
    asm("   set r30, r30, 1");
    __delay_cycles(403);
    asm("   clr r30, r30, 1");
    __delay_cycles(393);
    asm("   set r30, r30, 1");
    __delay_cycles(384);
    asm("   clr r30, r30, 1");
    __delay_cycles(375);
    asm("   set r30, r30, 1");
    __delay_cycles(368);
    asm("   clr r30, r30, 1");
    __delay_cycles(360);
    asm("   set r30, r30, 1");
    __delay_cycles(353);
    asm("   clr r30, r30, 1");
    __delay_cycles(346);
    asm("   set r30, r30, 1");
    __delay_cycles(340);
    asm("   clr r30, r30, 1");
    __delay_cycles(334);
    asm("   set r30, r30, 1");
    __delay_cycles(328);
    asm("   clr r30, r30, 1");
    __delay_cycles(323);
    asm("   set r30, r30, 1");
    __delay_cycles(318);
    asm("   clr r30, r30, 1");
    __delay_cycles(312);
    asm("   set r30, r30, 1");
    __delay_cycles(309);
    asm("   clr r30, r30, 1");
    __delay_cycles(303);
    asm("   set r30, r30, 1");
    __delay_cycles(300);
    asm("   clr r30, r30, 1");
    __delay_cycles(295);
    asm("   set r30, r30, 1");
    __delay_cycles(291);
    asm("   clr r30, r30, 1");
    __delay_cycles(288);
    asm("   set r30, r30, 1");
    __delay_cycles(284);
    asm("   clr r30, r30, 1");
    __delay_cycles(280);
    asm("   set r30, r30, 1");
    __delay_cycles(277);
    asm("   clr r30, r30, 1");
    __delay_cycles(274);
    asm("   set r30, r30, 1");
    __delay_cycles(270);
    asm("   clr r30, r30, 1");
    __delay_cycles(268);
    asm("   set r30, r30, 1");
    __delay_cycles(264);
    asm("   clr r30, r30, 1");
    __delay_cycles(262);
    asm("   set r30, r30, 1");
    __delay_cycles(259);
    asm("   clr r30, r30, 1");
    __delay_cycles(256);
    asm("   set r30, r30, 1");
    __delay_cycles(254);
    asm("   clr r30, r30, 1");
    __delay_cycles(249);
    asm("   set r30, r30, 1");
    __delay_cycles(325);
    asm("   clr r30, r30, 1");
    __delay_cycles(542);
    asm("   set r30, r30, 1");
    __delay_cycles(592);
    asm("   clr r30, r30, 1");
    __delay_cycles(582);
    asm("   set r30, r30, 1");
    __delay_cycles(573);
    asm("   clr r30, r30, 1");
    __delay_cycles(563);
    asm("   set r30, r30, 1");
    __delay_cycles(555);
    asm("   clr r30, r30, 1");
    __delay_cycles(546);
    asm("   set r30, r30, 1");
    __delay_cycles(538);
    asm("   clr r30, r30, 1");
    __delay_cycles(531);
    asm("   set r30, r30, 1");
    __delay_cycles(523);
    asm("   clr r30, r30, 1");
    __delay_cycles(517);
    asm("   set r30, r30, 1");
    __delay_cycles(509);
    asm("   clr r30, r30, 1");
    __delay_cycles(504);
}

void __inline emit_neg_symbol() {
    asm("   clr r30, r30, 1");
    __delay_cycles(484);
    asm("   set r30, r30, 1");
    __delay_cycles(467);
    asm("   clr r30, r30, 1");
    __delay_cycles(452);
    asm("   set r30, r30, 1");
    __delay_cycles(437);
    asm("   clr r30, r30, 1");
    __delay_cycles(426);
    asm("   set r30, r30, 1");
    __delay_cycles(414);
    asm("   clr r30, r30, 1");
    __delay_cycles(403);
    asm("   set r30, r30, 1");
    __delay_cycles(393);
    asm("   clr r30, r30, 1");
    __delay_cycles(384);
    asm("   set r30, r30, 1");
    __delay_cycles(375);
    asm("   clr r30, r30, 1");
    __delay_cycles(368);
    asm("   set r30, r30, 1");
    __delay_cycles(360);
    asm("   clr r30, r30, 1");
    __delay_cycles(353);
    asm("   set r30, r30, 1");
    __delay_cycles(346);
    asm("   clr r30, r30, 1");
    __delay_cycles(340);
    asm("   set r30, r30, 1");
    __delay_cycles(334);
    asm("   clr r30, r30, 1");
    __delay_cycles(328);
    asm("   set r30, r30, 1");
    __delay_cycles(323);
    asm("   clr r30, r30, 1");
    __delay_cycles(318);
    asm("   set r30, r30, 1");
    __delay_cycles(312);
    asm("   clr r30, r30, 1");
    __delay_cycles(309);
    asm("   set r30, r30, 1");
    __delay_cycles(303);
    asm("   clr r30, r30, 1");
    __delay_cycles(300);
    asm("   set r30, r30, 1");
    __delay_cycles(295);
    asm("   clr r30, r30, 1");
    __delay_cycles(291);
    asm("   set r30, r30, 1");
    __delay_cycles(288);
    asm("   clr r30, r30, 1");
    __delay_cycles(284);
    asm("   set r30, r30, 1");
    __delay_cycles(280);
    asm("   clr r30, r30, 1");
    __delay_cycles(277);
    asm("   set r30, r30, 1");
    __delay_cycles(274);
    asm("   clr r30, r30, 1");
    __delay_cycles(270);
    asm("   set r30, r30, 1");
    __delay_cycles(268);
    asm("   clr r30, r30, 1");
    __delay_cycles(264);
    asm("   set r30, r30, 1");
    __delay_cycles(262);
    asm("   clr r30, r30, 1");
    __delay_cycles(259);
    asm("   set r30, r30, 1");
    __delay_cycles(256);
    asm("   clr r30, r30, 1");
    __delay_cycles(254);
    asm("   set r30, r30, 1");
    __delay_cycles(249);
    asm("   clr r30, r30, 1");
    __delay_cycles(325);
    asm("   set r30, r30, 1");
    __delay_cycles(542);
    asm("   clr r30, r30, 1");
    __delay_cycles(592);
    asm("   set r30, r30, 1");
    __delay_cycles(582);
    asm("   clr r30, r30, 1");
    __delay_cycles(573);
    asm("   set r30, r30, 1");
    __delay_cycles(563);
    asm("   clr r30, r30, 1");
    __delay_cycles(555);
    asm("   set r30, r30, 1");
    __delay_cycles(546);
    asm("   clr r30, r30, 1");
    __delay_cycles(538);
    asm("   set r30, r30, 1");
    __delay_cycles(531);
    asm("   clr r30, r30, 1");
    __delay_cycles(523);
    asm("   set r30, r30, 1");
    __delay_cycles(517);
    asm("   clr r30, r30, 1");
    __delay_cycles(509);
    asm("   set r30, r30, 1");
    __delay_cycles(504);
}

int __inline return_debug_info() {
    rx_ring_buffer_t *rx_buf = (rx_ring_buffer_t*) SHARED_RECEIVE_BUF_OFFSET;
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

#define PREAMBLE_EXTRA_CYCLES 2800 // 14us at 200MHz
#define PREAMBLE_TOTAL_CYCLES 22800 // 114us at 200MHz

void __inline emit_preamble_neg() {
    emit_pos_symbol();
    __delay_cycles(PREAMBLE_EXTRA_CYCLES);
}

void __inline emit_preamble_pos() {
    asm("   clr r30, r30, 1");
    __delay_cycles(PREAMBLE_TOTAL_CYCLES);
}

#define FAST_GPIO_BIT (1 << 1)
#define BUS_ACCESS_IDLE_CYCLES (12 * PREAMBLE_TOTAL_CYCLES)

//TODO tune the delays to account for the loop overhead
int __inline hw_send_preamble(volatile tx_frame_t *msg) {
    emit_preamble_neg();
    emit_preamble_neg();
    emit_preamble_neg();
    for(int i=0; i < TX_FRAME_PREAMBLE_LEN; ++i) {
        if(msg->preamble & (1 << i)) {
            emit_preamble_pos();
        } else {
            emit_preamble_neg();
        }
    }
    emit_preamble_pos();

    return 0;
}

//TODO tune the delays to account for the loop overhead
int __inline hw_send_payload(volatile tx_frame_t *msg) {
    for(int i=0; i < 5; ++i) {
        emit_pos_symbol();
    }
    for(int i=0; i < msg->bit_length; ++i) {
        if(msg->payload[ ((int) i / 8) ] & (i << (i % 8))) {
            emit_pos_symbol();
        } else {
            emit_neg_symbol();
        }
    }
    for(int i=0; i < 7; ++i) {
        emit_pos_symbol();
    }

    return 0;
}

int __inline hw_send_frame(volatile tx_frame_t *msg) {
    // TODO: remove this fixed lamp on message
    // msg->preamble = 0x0a;
    // msg->bit_length = 30;
    // msg->payload[0] = 0xfb;
    // msg->payload[1] = 0x20;
    // msg->payload[2] = 0x08;
    // msg->payload[3] = 0x50;

    // TODO: we might need to mask interrupts in this area.
    // TX gets unexpectedly long sometimes
    // send preamble
    hw_send_preamble(msg);
    // send payload (and checksum)
    hw_send_payload(msg);
    // sleep for bus access idle time
    asm("   clr r30, r30, 1");
    __delay_cycles(BUS_ACCESS_IDLE_CYCLES);

    return return_debug_info();
}

void ringbuff_init() {
    rx_ring_buffer_t *rx_buf = (rx_ring_buffer_t*) SHARED_RECEIVE_BUF_OFFSET;
    tx_ring_buffer_t *tx_buf = (tx_ring_buffer_t*) SHARED_SEND_BUF_OFFSET;

    memset((void *) rx_buf, 0, SHARED_RECEIVE_BUF_SIZE);
    memset((void *) tx_buf, 0, SHARED_SEND_BUF_SIZE);

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
