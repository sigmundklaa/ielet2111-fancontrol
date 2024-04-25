
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

#include "../error.h"
#include "i2c.h"

/**
 * @brief Calculate the baud rate that can be used in the MBAUD register, based
 * on the nominal baud rate @p nominal and the desired mode @p mode
 *
 * See the AVR128DB48 datasheet, section 29.3.2.2.1 for details on configuration
 * of BAUD rate.
 *
 * @param nominal
 * @param mode One of `I2C_MODE_STANDARD`, `I2C_MODE_FAST`, `I2C_MODE_FAST_PLUS`
 * @return uint8_t
 */
static uint8_t baud_rate_(uint32_t nominal, enum i2c_mode mode)
{
        uint32_t base, tlow, tlow_mode;

        /* Derived from eq. 2 in 29.3.2.2.1, however TR has been set to 0 as
         * the resolution of TWIn.MBAUD is not high enough to be affected by
         * the impact of TR being non-zero. */
        base = F_CPU / nominal - 10;

        /* Multiplied by 1e7 to avoid floating point arithmetic, while not
         * risking an overflow. This is undone later, and is also the reason
         * tlow_mode has the last two digits removed */
        tlow = ((base + 5) * 10000000) / F_CPU;

        switch (mode) {
        case I2C_MODE_FAST:
                tlow_mode = 13;
                break;
        case I2C_MODE_FAST_PLUS:
                tlow_mode = 5;
                break;
        case I2C_MODE_STANDARD:
        default:
                tlow_mode = 47;
                break;
        }

        if (tlow < tlow_mode) {
                /* Like with TR, TOF has been set to 0 as the difference is
                 * minimal. See eq. 4 in 29.3.2.2.1. */
                base = ((F_CPU * (tlow_mode)) / 10000000) - 5;
        }

        return (uint8_t)base;
}

/**
 * @brief Helper function to give a better name for ack-checking, as
 * `TWIn.MSTATUS & TWI_RXACK_bm` can be confused for ACK, not NACK.
 *
 * @param status
 * @return bool
 * @retval true Status has NACK bit set
 * @retval false Status does not have NACK bit set
 */
static inline bool is_nack_(uint16_t status)
{
        return status & TWI_RXACK_bm;
}

/**
 * @brief Setup TWI master on TWI instance @p twi, using @p speed on mode @p
 * mode
 *
 * @param twi
 * @param speed Bus speed, in Hz
 * @param mode One of `MODE_STANDARD_`, `MODE_FAST_`, `MODE_FAST_PLUS_`
 */
static void msetup_(volatile TWI_t* twi, uint32_t speed, enum i2c_mode mode)
{
        twi->DBGCTRL = TWI_DBGRUN_bm;

        twi->MBAUD = baud_rate_(speed, mode);

        /* Force bus into idle, and enable the master */
        twi->MSTATUS = 0x1;
        twi->MCTRLA |= TWI_ENABLE_bm;
}

/**
 * @brief Helper function to wait for the TWI master to have acted on the
 * previous request
 *
 * @param twi
 */
static void mwait_(volatile TWI_t* twi)
{
        /* Case M1, M3 and M4 will set WIF. M1, M2 and M3 set CLKHOLD */
        while (!(twi->MSTATUS & (TWI_WIF_bm | TWI_CLKHOLD_bm))) {
        }
}

/**
 * @brief Set the address of the current transmission on @p twi to @ addr
 *
 * @param twi
 * @param addr
 * @param is_read Indicates direction
 * @return int
 * @retval -ENODEV No device with address @p addr acknowledged the request
 * @retval -EBUSY Error on bus
 * @retval 0 Success
 */
static int mset_addr_(volatile TWI_t* twi, uint8_t addr, bool is_read)
{
        twi->MADDR = (addr << 1) | is_read;
        mwait_(twi);

        if (is_nack_(twi->MSTATUS)) {
                /* Case M3: Not acknowledged, stop transfer */
                twi->MCTRLB |= TWI_MCMD_STOP_gc;

                return -E_NODEV;
        } else if (twi->MSTATUS & (TWI_BUSERR_bm | TWI_ARBLOST_bm)) {
                /* Case M4: Arbitration lost/bus error */

                return -E_BUSY;
        }

        return 0;
}

/**
 * @brief Send at most @p size bytes from @p data to @p addr, using @p twi
 *
 * @param twi
 * @param addr
 * @param data
 * @param size
 * @return ptrdiff_t
 * @retval -ENODEV No device with address @p addr acknowledged the request
 * @retval -EBUSY Error on bus
 * @retval >=0 Bytes written
 */
static ptrdiff_t
msend_(volatile TWI_t* twi, uint8_t addr, const uint8_t* data, size_t size)
{
        size_t sent;
        ptrdiff_t status;

        status = mset_addr_(twi, addr, false);
        if (status != 0) {
                return status;
        }

        for (sent = 0; sent < size; sent++) {
                twi->MDATA = data[sent];
                mwait_(twi);

                if (is_nack_(twi->MSTATUS)) {
                        twi->MCTRLB |= TWI_MCMD_STOP_gc;

                        return -E_BUSY;
                } else if (twi->MSTATUS & (TWI_BUSERR_bm | TWI_ARBLOST_bm)) {
                        return -E_BUSY;
                }
        }

        twi->MCTRLB |= TWI_MCMD_STOP_gc;

        return (ptrdiff_t)sent;
}

/**
 * @brief Receive at most @p max bytes from @p addr into @p buf, using @p twi
 *
 * @param twi
 * @param addr
 * @param buf
 * @param max
 * @return ptrdiff_t
 * @retval -ENODEV No device with address @p addr acknowledged the request
 * @retval -EBUSY Error on bus
 * @retval >=0 Bytes received
 */
static ptrdiff_t
mrecv_(volatile TWI_t* twi, uint8_t addr, uint8_t* buf, size_t max)
{
        size_t bytes = 0;
        ptrdiff_t status;

        status = mset_addr_(twi, addr, true);
        if (status != 0) {
                return status;
        }

        while (bytes < max) {
                /* On receive, RIF and CLKHOLD are set to 1. On error, WIF is
                 * set to 1*/
                while (!(twi->MSTATUS & (TWI_CLKHOLD_bm | TWI_WIF_bm))) {
                }

                if (twi->MSTATUS & TWI_WIF_bm) {
                        return -E_IO;
                }

                buf[bytes++] = twi->MDATA;

                if (bytes < max) {
                        twi->MCTRLB |= TWI_MCMD_RECVTRANS_gc;
                }
        }

        twi->MCTRLB |= TWI_MCMD_STOP_gc;

        return (ptrdiff_t)bytes;
}

/**
 * @brief Set the slave address of @p twi to @p addr
 *
 * @param twi
 * @param addr
 */
static inline void sset_addr_(volatile TWI_t* twi, uint8_t addr)
{
        twi->SADDR = addr << 1;
}

/**
 * @brief Setup TWI slave with addr @p addr
 *
 * @param twi TWIn instance to use
 * @param addr Slave address
 */
static void ssetup_(volatile TWI_t* twi, uint8_t addr)
{
        sset_addr_(twi, addr);
        twi->SCTRLA |= TWI_PIEN_bm | TWI_APIEN_bm | TWI_DIEN_bm | TWI_ENABLE_bm;

        (void)twi->SSTATUS;
}

struct ringbuf_ {
        uint8_t buf[100];
        size_t head;
        size_t length;
};

static struct ringbuf_ tx_buf_, rx_buf_;

/**
 * @brief Write @p c to ringbuffer @p rbuf
 *
 * NOTE: This is not ISR safe on its own
 *
 * @param rbuf
 * @param c
 */
static int rbuf_write_(struct ringbuf_* rbuf, uint8_t c)
{
        if (rbuf->length >= sizeof(rbuf->buf)) {
                return -E_NOMEM;
        }

        size_t index = (rbuf->head + rbuf->length) % sizeof(rbuf->buf);
        rbuf->buf[index] = c;
        rbuf->length++;

        return 0;
}

/**
 * @brief Read one byte into @p c from ringbuffer @p rbuf
 *
 * @param rbuf
 * @param c
 * @return int
 * @retval -ENODATA No data in buffer
 * @retval 0 Byte read successfully
 */
static int rbuf_read_(struct ringbuf_* rbuf, volatile uint8_t* c)
{
        if (rbuf->length == 0) {
                return -E_NODATA;
        }

        *c = rbuf->buf[rbuf->head];
        rbuf->head = (rbuf->head + 1) % sizeof(rbuf->buf);
        rbuf->length--;

        return 0;
}

/**
 * @brief Handle slave interrupt for TWI instance @p twi
 *
 * @param twi
 */
static void sisr_handle_(volatile TWI_t* twi)
{
        if (twi->SSTATUS & TWI_DIF_bm) {
                int status;
                if (twi->SSTATUS & TWI_DIR_bm) {
                        /* Transmit direction */
                        status = rbuf_read_(&tx_buf_, &twi->SDATA);
                } else {
                        /* Receive direction */
                        status = rbuf_write_(&rx_buf_, twi->SDATA);
                }

                if (status != 0) {
                        twi->SCTRLB |= TWI_ACKACT_NACK_gc;
                }

                twi->SCTRLB |= TWI_SCMD_RESPONSE_gc;
        }

        if (twi->SSTATUS & TWI_APIF_bm) {
                if (twi->SSTATUS & TWI_AP_ADR_gc) {
                        twi->SCTRLB = TWI_ACKACT_ACK_gc | TWI_SCMD_RESPONSE_gc;
                } else {
                        twi->SCTRLB = TWI_ACKACT_NACK_gc;
                        twi->SCTRLB |= TWI_SCMD_COMPTRANS_gc;
                }
        }
}

ISR(TWI0_TWIS_vect)
{
        sisr_handle_(&TWI0);
}

void i2c_master_init(uint32_t speed, enum i2c_mode mode)
{
        msetup_(&TWI0, speed, mode);
}

ptrdiff_t i2c_master_send(uint8_t addr, const uint8_t* data, size_t size)
{
        return msend_(&TWI0, addr, data, size);
}

ptrdiff_t i2c_master_recv(uint8_t addr, uint8_t* buf, size_t size)
{
        return mrecv_(&TWI0, addr, buf, size);
}

void i2c_slave_init(uint8_t addr)
{
        ssetup_(&TWI0, addr);
}

void i2c_slave_set_addr(uint8_t addr)
{
        sset_addr_(&TWI0, addr);
}

size_t i2c_slave_send(const uint8_t* data, size_t size)
{
        size_t i = 0;

        cli();

        for (; rbuf_write_(&tx_buf_, data[i]) == 0 && --size; i++) {
        }

        sei();

        return i;
}

size_t i2c_slave_recv(uint8_t* buf, size_t size)
{
        size_t i = 0;

        cli();

        for (; rbuf_read_(&rx_buf_, buf + i) == 0 && --size; i++) {
        }

        sei();

        return i;
}
