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

#define PRU_NO 1

#define PAYLOAD_LEN 255
/* must match the same in j17084truckduck_host.py */
compile_time_assert(PAYLOAD_LEN == 255, frame_size_must_be_42);
typedef struct  {
    uint8_t volatile length;
    uint8_t volatile payload[PAYLOAD_LEN];
} frame_t;

#define FRAME_SIZE 256
/* must match the same in j17084truckduck_host.py */
compile_time_assert(FRAME_SIZE == sizeof(frame_t), frame_size_must_be_43);

#define RX_RING_BUFFER_LEN 16
typedef struct {
    uint32_t volatile produce;
    uint32_t volatile consume;
    frame_t volatile frames[RX_RING_BUFFER_LEN];
} rx_ring_buffer_t;

#define TX_RING_BUFFER_LEN 4
typedef struct {
    uint32_t volatile produce;
    uint32_t volatile consume;
    frame_t volatile frames[TX_RING_BUFFER_LEN];
} tx_ring_buffer_t;

#define RING_BUFFER_CONSUME_OFFSET 4
/* must match the same in j17084truckduck_host.py */
compile_time_assert(RING_BUFFER_CONSUME_OFFSET ==
                    offsetof(rx_ring_buffer_t, consume), rx_ring_buf_consume_offset_must_be_4);
compile_time_assert(RING_BUFFER_CONSUME_OFFSET ==
                    offsetof(tx_ring_buffer_t, consume), tx_ring_buf_consume_offset_must_be_4);

#define RING_BUFFER_FRAMES_OFFSET 8
/* must match the same in j17084truckduck_host.py */
compile_time_assert(RING_BUFFER_FRAMES_OFFSET ==
                    offsetof(rx_ring_buffer_t, frames), rx_ing_buf_frames_offset_must_be_8);
compile_time_assert(RING_BUFFER_FRAMES_OFFSET ==
                    offsetof(tx_ring_buffer_t, frames), tx_ing_buf_frames_offset_must_be_8);

#define RX_RING_BUFFER_VADDR_OFFSET 0
/* must match the same in j17084truckduck_host.py */
compile_time_assert(RX_RING_BUFFER_VADDR_OFFSET == 0, rx_buf_start_must_be_0);
#define RX_RING_BUFFER_SIZE sizeof(rx_ring_buffer_t)
/* must match the same in j17084truckduck_host.py */
compile_time_assert(RX_RING_BUFFER_SIZE == 4104 , rx_buf_len_must_be_696);
#define TX_RING_BUFFER_VADDR_OFFSET 4104
/* must match the same in j17084truckduck_host.py */
compile_time_assert(TX_RING_BUFFER_VADDR_OFFSET == 4104, tx_buf_start_must_be_704);

/* must match the same in j17084truckduck_host.py */
#define TX_RING_BUFFER_SIZE sizeof(tx_ring_buffer_t)
compile_time_assert(TX_RING_BUFFER_SIZE == 1032, tx_buf_len_must_be_696);

/* check total size doesn't exceed available 8K */
compile_time_assert(RX_RING_BUFFER_SIZE + TX_RING_BUFFER_SIZE + 438 /*stack size*/ + 0 /*heap size*/ < 8192 /*PRU RAM size*/, bufs_must_be_less_than_ram);

int __inline signal_frame_received();

void __inline hw_wait_for_bus();
int __inline hw_is_bus_available();
uint8_t __inline hw_wait_and_read_char();

int __inline receive_frame() {
    rx_ring_buffer_t *rx_buf = (rx_ring_buffer_t*) RX_RING_BUFFER_VADDR_OFFSET;
    int ret = 0;

    for (int i = 0; i < PAYLOAD_LEN; ++i) {
        uint32_t tmp_produce = rx_buf->produce;
        uint32_t tmp_consume = rx_buf->consume;

        if ((tmp_produce + 1) % RX_RING_BUFFER_LEN == tmp_consume) {
            hw_wait_for_bus();
            /* Buffer is full
             * TODO better error logging,
             * return non-zero here */
            break;
        }

        rx_buf->frames[tmp_produce].payload[i] = hw_wait_and_read_char();

        /* TODO: probably better to terminate a frame when the most recent
         * bytes match the checksum of the previous bytes instead of testing
         * for bus idleness here. Sometimes we receive extra bytes appended to
         * the received frames */
        if (hw_is_bus_available()) {
            rx_buf->frames[tmp_produce].length = i + 1;
            rx_buf->produce = (tmp_produce + 1) % RX_RING_BUFFER_LEN;

            signal_frame_received();
            break;
        }
    }

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

int __inline hw_send_frame(volatile frame_t *msg);

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

#define CM_PER_BASE ((volatile uint32_t *)(0x44E00000))
/* using UART2 */
#define UARTX_CLKCTRL (0x70 / 4)
#define CLKCTRL_ON 2
#define WARMUP_CYCLES 100

void __inline gpio_warmup();

void __inline hw_warmup() {
    volatile uint32_t *ptr_cm = CM_PER_BASE;

    ptr_cm[UARTX_CLKCTRL] = CLKCTRL_ON;
    while(ptr_cm[UARTX_CLKCTRL] & 0x00300000) {
        __delay_cycles(WARMUP_CYCLES);
    }

    gpio_warmup();
}

#define UART_BASE ((volatile uint16_t *)(0x48024000)) /* UART2 */
#define UART_SYSC (0x54 / 2)
#define UART_SYSS (0x58 / 2)
#define UART_LCR  (0x0C / 2)
#define UART_EFR  (0x08 / 2)
#define UART_MCR  (0x10 / 2)
#define UART_FCR  (0x08 / 2)
#define UART_TLR  (0x1C / 2)
#define UART_SCR  (0x40 / 2)
#define UART_MDR1 (0x20 / 2)
#define UART_IER  (0x04 / 2)
#define UART_DLL  (0x00 / 2)
#define UART_DLH  (0x04 / 2)
#define UART_LSR  (0x14 / 2)
#define UART_RHR  (0x00 / 2)
#define UART_THR  (0x00 / 2)

void __inline hw_init() {
    /* UART init as per spruh73q */
    volatile uint16_t *ptr_uart = UART_BASE;

    ptr_uart[UART_SYSC] = 0x1;

    while((ptr_uart[UART_SYSS] & 0x1) == 0) {
        __delay_cycles(10000);
    }
    ptr_uart[UART_LCR] |= 0x40;

    uint16_t saved_lcr = ptr_uart[UART_LCR];
    ptr_uart[UART_LCR] = 0x00BF;

    uint16_t saved_efr = ptr_uart[UART_EFR];
    ptr_uart[UART_EFR] = (saved_efr | 0x10);

    ptr_uart[UART_LCR] = 0x80;

    uint16_t saved_mcr = ptr_uart[UART_MCR];
    ptr_uart[UART_MCR] = (saved_mcr | 0x40);

    ptr_uart[UART_FCR] = 0x3;
    ptr_uart[UART_LCR] = 0xBF;
    ptr_uart[UART_TLR] = 0x88;
    ptr_uart[UART_SCR] = 0xC0;

    ptr_uart[UART_EFR] = saved_efr;
    ptr_uart[UART_LCR] = 0x80;
    ptr_uart[UART_MCR] = saved_mcr;
    ptr_uart[UART_LCR] = saved_lcr;

    uint16_t saved_reg = ptr_uart[UART_MDR1];
    ptr_uart[UART_MDR1] = (saved_reg & 0xFFF8) | 0x7;

    ptr_uart[UART_LCR] = 0x00BF;
    saved_efr = ptr_uart[UART_EFR];
    ptr_uart[UART_EFR] = (saved_efr | 0x10);

    ptr_uart[UART_LCR] = 0x00;
    ptr_uart[UART_IER] = 0x00;
    ptr_uart[UART_LCR] = 0xBF;

    ptr_uart[UART_DLL] = 0x38;
    ptr_uart[UART_DLH] = 0x01;

    ptr_uart[UART_LCR] = 0x00;
    ptr_uart[UART_IER] = 0x00;
    ptr_uart[UART_LCR] = 0xBF;

    ptr_uart[UART_EFR] = saved_efr;
    ptr_uart[UART_LCR] = 0x03;

    saved_reg = ptr_uart[UART_MDR1];
    ptr_uart[UART_MDR1] = (saved_reg & 0xFFF8);

    hw_wait_for_bus();

    /* drain RX FIFO */
    volatile uint8_t *ptr_rhr = (uint8_t*) UART_BASE + UART_RHR;
    volatile uint8_t rx;
    while (ptr_uart[UART_LSR] & (1 << 0)) {
        rx = *ptr_rhr;
    }
    rx |= 0xff; // avoid optimize-away

    return;
}

#define READ_CYCLES 60 /* anything fast: e.g. 300ns at 200MHz */

uint8_t __inline hw_wait_and_read_char() {
    volatile uint16_t *ptr_uart = UART_BASE;
    volatile uint8_t *ptr_rhr = (uint8_t*) UART_BASE + UART_RHR;

    while(!(ptr_uart[UART_LSR] & (1 << 0))) {
        __delay_cycles(READ_CYCLES);
    }
    return *ptr_rhr;
}

#define SEND_CYCLES 200 /*something fast for polling in a hotloop */

int __inline hw_send_frame(volatile frame_t *msg) {
    volatile uint16_t *ptr_uart = UART_BASE;
    int ret = 0;

    for (int i = 0; i < msg->length; ++i) {
        ptr_uart[UART_THR] = msg->payload[i];

        while(ptr_uart[UART_LSR] & (1 << 6)) { /* wait for tx complete */
            __delay_cycles(SEND_CYCLES);
        }

        uint8_t rx = hw_wait_and_read_char();
        if (rx != msg->payload[i]) {
            /* TODO: better error logging
             * and: implement arbitration
             * or return non-zero here */
            break;
        }
    }

    return ret;
}


#define GPIO_BASE ((volatile uint32_t *)(0x4804C000)) /* Using GPIO1 */
#define GPIO_CTRL (0x130 / 4)
#define GPIO_DATAIN (0x138 / 4)
#define GPIO_PIN (1 << 28)

#define GPIO1_CLKCTRL (0xAC / 4)

void __inline gpio_warmup() {
#if (PRU_NO == 1)
    volatile uint32_t *ptr_gpio = GPIO_BASE;
    volatile uint32_t *ptr_cm = CM_PER_BASE;

    ptr_cm[GPIO1_CLKCTRL] = CLKCTRL_ON;
    while(ptr_cm[GPIO1_CLKCTRL] & 0x00300000) {
        __delay_cycles(WARMUP_CYCLES);
    }

    ptr_gpio[GPIO_CTRL] = 0; /* gating enabled, no divs */
#endif
}

int __inline read_gpio_pin() {
    volatile uint32_t *ptr_gpio = GPIO_BASE;

    return ptr_gpio[GPIO_DATAIN] & GPIO_PIN;
}

#define AVAILABLE_CYCLES 10400 /* 52us at 200MHz */
#define AVAILABLE_ATTEMPTS 20

/* returns non-zero if the PLC bus is available at present */
int __inline hw_is_bus_available() {
    for(int i =0; i < AVAILABLE_ATTEMPTS; ++i) {
        if (read_gpio_pin() == 0) {
            return 0;
        } else {
            __delay_cycles(AVAILABLE_CYCLES);
        }
    }
    return 1;
}

#define WAIT_CYCLES 10400 /* 52us at 200MHz */
#define WAIT_ATTEMPTS 13 /* lucky wait number */

void __inline hw_wait_for_bus() {
    for(int i = 1; i < WAIT_ATTEMPTS; ++i) {
        if (!read_gpio_pin()) {
            i = 1;
            continue;
        }
        __delay_cycles(WAIT_CYCLES);
    }
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
        if (hw_is_bus_available()) {
            if (send_next_frame()) {
                /* TODO error handling better than 'die' */
                break;
            }
        } else {
            if (receive_frame()) {
                /* TODO error handling better than 'die' */
                break;
            }
        }
    }

    __halt();
}
