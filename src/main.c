
#include <stdio.h>

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/eeprom.h>
#include <util/delay.h>

#include "drivers/i2c.h"
#include "drivers/usart.h"
#include "store.h"

extern void shell_tick(void);

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

        i2c_master_init(100000, I2C_MODE_STANDARD);
        i2c_slave_init(store_get(i2c_slave_addr));
        sei();

        uint8_t buf[100];
        uint32_t ticks = 0;

        while (1) {
                if (ticks >= 100000) {
                        ticks = 0;
                        i2c_master_send(4, (uint8_t*)"Hello World", 12);
                        i2c_master_recv(4, buf, 5);
                        buf[5] = 0;
                        (void)printf("Receive: %s\r\n", (const char*)buf);
                }

                ticks++;

                shell_tick();
        }
}