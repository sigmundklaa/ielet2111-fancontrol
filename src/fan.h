
#ifndef FAN_H__
#define FAN_H__

#include <stdint.h>

/**
 * @brief Initialize fans
 */
void fan_init(void);

/**
 * @brief Check the speed of fan @p index. If the speed is not close to the
 * nominal speed determined by the output of the fan controller, an error
 * will be printed to the console.
 *
 * @param fan_index Index of fan
 */
void fan_check_speed(uint8_t fan_index);

/**
 * @brief Set the speed of fan @p index to one of "off", "low", "medium", "max"
 *
 * @param fan_index
 * @param speed
 */
void fan_set_speed(uint8_t fan_index, const char* speed);

/**
 * @brief Get the last measured speed for fan @p fan_index
 *
 * @param fan_index
 * @return uint16_t Speed in RPM
 */
uint16_t fan_get_speed(uint8_t fan_index);

#endif /* FAN_H__ */