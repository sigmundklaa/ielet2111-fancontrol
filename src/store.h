
#ifndef STORE_H__
#define STORE_H__

struct store {
        uint8_t i2c_slave_addr;
};

/**
 * @brief Initialize the EEPROM store, reading from EEPROM if there is any
 * data saved there.
 */
void store_init(void);

/**
 * @brief Update the value of the store field @p value to @p value
 *
 * @param field
 * @param value
 */
#define store_update(field, value)                                             \
        store_update__(offsetof(struct store, field), value, sizeof(*value))

/**
 * @brief Read the contents of store field @p field into @p buffer
 *
 * @param field
 * @param buffer
 */
#define store_read(field, buffer)                                              \
        store_read__(offsetof(struct store, field), buffer, sizeof(*buffer))

/**
 * @brief Get the type of the store field @p field
 *
 * @param field
 */
#define store_type(field) (typeof(((struct store*)NULL)->field))

/**
 * @brief Get the value stored at store field @p field
 *
 * @param field
 */
#define store_get(field)                                                       \
        (*((typeof(((struct store*)NULL)->field                                \
        )*)store_get__(offsetof(struct store, field))))

/**
 * @brief Update the value at offset @p addr_offset in the store, writing
 * @p size bytes from @p value
 *
 * NOTE: This function should not be used directly, see `store_update` instead
 *
 * @param addr_offset
 * @param value
 * @param size
 */
void store_update__(uint16_t addr_offset, const void* value, size_t size);

/**
 * @brief Read the value at @p addr_offset into @p buf
 *
 * NOTE: This function should not be used directly, see `store_read` instead
 *
 * @param addr_offset
 * @param buf
 * @param Max size of buffer
 */
void store_read__(uint16_t addr_offset, void* buf, size_t size);

/**
 * @brief Get the store address pointed to by @p addr_offset
 *
 * NOTE: This function should not be used directly, see `store_get` instead
 *
 * @param addr_offset
 * @return void*
 */
void* store_get__(uint16_t addr_offset);

#endif /* STORE_H__ */