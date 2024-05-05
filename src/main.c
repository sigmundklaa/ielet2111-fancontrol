
#include <stdio.h>

#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

#include "cmd.h"
#include "drivers/i2c.h"
#include "drivers/usart.h"
#include "fan.h"
#include "store.h"

extern void shell_tick(void);
extern void fan_tick(void);

int main(void)
{
        /* Setup I2C controller pins */
        PORTA.DIRSET = PIN2_bm | PIN3_bm;
        PORTA.PINCONFIG = PORT_PULLUPEN_bm;
        PORTA.PINCTRLUPD = PIN2_bm | PIN3_bm;

        /* Setup USART3 controller */
        PORTB.DIR = 1 << 0;

        store_init();

        usart_init(&USART3, 9600);
        usart_setup_stdout();

        fan_init();

        i2c_master_init(100000, I2C_MODE_STANDARD);

        i2c_slave_init(store_get(i2c_slave_addr));
        sei();

        while (1) {
                shell_tick();
                cmd_tick();
                fan_tick();
        }
}