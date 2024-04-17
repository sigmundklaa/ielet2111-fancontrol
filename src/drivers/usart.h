
#ifndef DRIVER_USART_H__
#define DRIVER_USART_H__

#include <stddef.h>
#include <stdint.h>

#include <avr/io.h>

/**
 * @brief Setup the main USART peripheral for use for stdout
 */
void usart_setup_stdout(void);

/**
 * @brief Set @p peri as the main USART peripheral, and configure it with
 * nominal baudrate @p baud and @p mode
 *
 * @param peri
 * @param baud
 */
void usart_init(volatile USART_t* const peri, uint32_t baud);

/**
 * @brief Write @p len bytes from @p str to main USART peripheral
 *
 * @param str
 * @param len
 */
void usart_write(const char* str, size_t len);

/**
 * @brief Read at most @p max bytes into @p buf from main USART peripheral
 *
 * @param buf
 * @param max
 * @return size_t Bytes read, at most @p max
 */
size_t usart_read(char* buf, size_t max);

#endif /* DRIVER_USART_H__ */
