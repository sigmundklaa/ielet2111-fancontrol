
#ifndef DRIVER_I2C_H__
#define DRIVER_I2C_H__

#include <stddef.h>
#include <stdint.h>

enum i2c_mode {
        I2C_MODE_STANDARD = 0,
        I2C_MODE_FAST,
        I2C_MODE_FAST_PLUS,
};

/**
 * @brief Initialize the I2C master
 *
 * @param speed Speed of I2C bus
 * @param mode
 */
void i2c_master_init(uint32_t speed, enum i2c_mode mode);

/**
 * @brief Send @p bytes from @p to I2C device with address @p addr
 *
 * @param addr
 * @param data
 * @param size
 * @return ptrdiff_t
 * @retval -ENODEV No device with address @p addr acknowledged the request
 * @retval -EBUSY Error on bus
 * @retval >=0 Bytes written
 */
ptrdiff_t i2c_master_send(uint8_t addr, const uint8_t* data, size_t size);

/**
 * @brief Receive at most @p max bytes from @p addr into @p buf
 *
 * @param addr
 * @param buf
 * @param max
 * @return ptrdiff_t
 * @retval -ENODEV No device with address @p addr acknowledged the request
 * @retval -EBUSY Error on bus
 * @retval >=0 Bytes received
 */
ptrdiff_t i2c_master_recv(uint8_t addr, uint8_t* buf, size_t size);

/**
 * @brief Initialize I2C slave with address @p addr
 *
 * @param addr
 */
void i2c_slave_init(uint8_t addr);

/**
 * @brief Set the slave address to @p addr
 *
 * @param addr
 */
void i2c_slave_set_addr(uint8_t addr);

/**
 * @brief Send @p size bytes from @p data as a slave
 *
 * @param data
 * @param size
 * @return size_t Bytes written
 */
size_t i2c_slave_send(const uint8_t* data, size_t size);

/**
 * @brief Receive at most @p size bytes into @p buf from master device
 *
 * @param buf
 * @param size
 * @return size_t Bytes received
 */
size_t i2c_slave_recv(uint8_t* buf, size_t size);

#endif /* DRIVER_I2C_H__ */