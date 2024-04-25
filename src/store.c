
#include <string.h>

#include <avr/eeprom.h>

#include "store.h"

static struct store store_ = {
    /* Default values, will be overwritten */
    .i2c_slave_addr = 9,
    .i2c_temp_addr = 5,
};

/**
 * @brief Get the real address of the field member located @p addr_offset into
 * the store
 *
 * @param addr_offset
 * @return void*
 */
static void* store_addr_(uint16_t addr_offset)
{
        return ((uint8_t*)&store_) + addr_offset;
}

/**
 * @brief Initialize the EEPROM store, reading from EEPROM if there is any
 * data saved there.
 */
void store_init(void)
{
        uint8_t byte = eeprom_read_byte((void*)EEPROM_START);

        /* If the first byte is not 0x1, then the store has not been saved yet
         * and we should instead use the default values. */
        if (byte == 0x1) {
                void* addr = (void*)(EEPROM_START + 1);

                eeprom_read_block(&store_, addr, sizeof(store_));
        }
}

void store_update__(uint16_t addr_offset, const void* value, size_t size)
{
        (void)memcpy(store_addr_(addr_offset), value, size);

        void* eeprom_addr = (void*)(EEPROM_START + 1 + addr_offset);

        eeprom_update_block(value, eeprom_addr, size);
        eeprom_update_byte((void*)EEPROM_START, 0x1);
}

void store_read__(uint16_t addr_offset, void* buf, size_t size)
{
        /* This function exists for consistency, and does not need to read from
         * EEPROM as that should already be handled by any call to
         * update/init */
        (void)memcpy(buf, store_addr_(addr_offset), size);
}

void* store_get__(uint16_t addr_offset)
{
        return store_addr_(addr_offset);
}