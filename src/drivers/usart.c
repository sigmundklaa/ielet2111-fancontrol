
#include <stdio.h>

#define F_CPU 4000000UL
#include <avr/interrupt.h>
#include <avr/io.h>

#include "../error.h"
#include "usart.h"

/**
 * @brief Main USART peripheral
 */
static volatile USART_t* usart_peri_;

static volatile struct {
        char mem[64];
        uint8_t head;
        uint8_t len;
} isr_buf_;

/**
 * @brief Helper function to ensure that @p cur is safe to use as an index into
 * the isr buffer, even in the case of an overflow
 *
 * @param cur Current value. This may be more than the maximum length of the
 * buffer
 * @return unsigned int New value, that is within the bounds of the buffer
 */
static inline unsigned int overflow_safe_(unsigned int cur)
{
        return cur % sizeof(isr_buf_.mem);
}

/**
 * @brief Send one char @p c to peripherial
 *
 * @param c
 */
static void send_one_(char c)
{
        while (!(usart_peri_->STATUS & USART_DREIF_bm)) {
        }

        usart_peri_->TXDATAL = c;
}

/**
 * @brief Process incoming char @p c from a USART peripheral. This is designed
 * to use in an interrupt context
 *
 * @param c
 */
static void process_incoming_(char c)
{
        uint8_t head = isr_buf_.head;
        uint8_t len = isr_buf_.len;
        unsigned int newlen;

        isr_buf_.mem[overflow_safe_(head + len)] = c;

        newlen = overflow_safe_(len + 1);

        /* If the new length is less than the old length, it means that it
         * overflowed. To maintain parts of the integrity of the buffer even
         * though we have exceeded the limit, we start manually popping bytes
         * off the front*/
        if (newlen < len) {
                isr_buf_.head = overflow_safe_(head + 1);
        } else {
                isr_buf_.len = newlen;
        }
}

/* Each vector is implemented, as only those that have interrupts configured
 * will be called. */
ISR(USART4_RXC_vect)
{
        process_incoming_(USART4.RXDATAL);
}

ISR(USART3_RXC_vect)
{
        process_incoming_(USART3.RXDATAL);
}

ISR(USART2_RXC_vect)
{
        process_incoming_(USART2.RXDATAL);
}

ISR(USART1_RXC_vect)
{
        process_incoming_(USART1.RXDATAL);
}

ISR(USART0_RXC_vect)
{
        process_incoming_(USART0.RXDATAL);
}

/**
 * @brief Read a byte from the ISR buffer into the address pointed to by @p c
 *
 * @param c
 * @return int
 * @retval 0 Read successful
 * @retval -ENODATA No data in the buffer
 */
static int recv_one_(char* c)
{
        cli();
        if (isr_buf_.len == 0) {
                sei();
                return -E_NODATA;
        }

        uint8_t head = isr_buf_.head;
        *c = isr_buf_.mem[head];

        isr_buf_.head = overflow_safe_(head + 1);
        isr_buf_.len--;

        sei();
        return 0;
}

/**
 * @brief Calculate the baud settings for a USART peripheral for nominal baud
 * rate @p baud
 *
 * This is derived from the below equation, however it is changed to not require
 * floating point arithmetic:
 * ((64 * F_CPU)/(16 * baud)) + 0.5
 *
 * @param baud
 * @return uint32_t Converted rate that can be assigned to a USART peripherial
 */
static inline uint32_t baud_rate_(uint32_t baud)
{
        return ((8 * F_CPU + baud) / (2 * baud));
}

/**
 * @brief Write a char @p c to USART peripherial `USART3`. This is used by the
 * stdout stream
 *
 * @param c
 * @param stream Unused
 * @return int Always 0, successful
 */
static int stream_write__(char c, FILE* stream)
{
        (void)stream;
        send_one_(c);

        return 0;
}

void usart_setup_stdout(void)
{
        static FILE stream =
            FDEV_SETUP_STREAM(stream_write__, NULL, _FDEV_SETUP_WRITE);

        stdout = &stream;
}

void usart_init(volatile USART_t* const peri, uint32_t baud)
{
        peri->BAUD = baud_rate_(baud);
        peri->CTRLB |= USART_TXEN_bm | USART_RXEN_bm;
        peri->CTRLA = USART_RXCIE_bm;

        usart_peri_ = peri;
}

void usart_write(const char* str, size_t len)
{
        while (len--) {
                send_one_(*str++);
        }
}

size_t usart_read(char* buf, size_t max)
{
        size_t i = 0;

        for (; i < max && recv_one_(&buf[i]) == 0; i++) {
        }

        return i;
}
