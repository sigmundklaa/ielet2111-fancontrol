
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <avr/interrupt.h>
#include <avr/io.h>

#include "drivers/i2c.h"
#include "drivers/usart.h"
#include "error.h"
#include "store.h"
#include "fan.h"

#define BUF_SIZE_ (64)
#define TERM_CHAR_ ('\r')
#define DEL_CHAR_ (127)

#define ARR_LEN_(x) (sizeof(x) / sizeof(x[0]))

static struct {
        char tmpbuf[BUF_SIZE_];
        char* args[5];
        uint8_t tmpidx;
} sh_buf_;

static int say_hello(int argc, char** argv)
{
        (void)printf("Hello %s\r\n", argv[1]);

        return 0;
}

static int i2c_addr_set_(int argc, char** argv)
{
        if (argc < 2) {
                (void)printf("Expected 2 arguments, got %i\r\n", argc);
                return E_INVAL;
        }

        uint8_t addr = atoi(argv[1]);
        store_update(i2c_slave_addr, &addr);

        i2c_slave_set_addr(addr);

        (void)printf("I2C slave address set to %i\r\n", (int)addr);

        return 0;
}

static int i2c_addr_get_(int argc, char** argv)
{
        (void)printf("%i\r\n", (int)store_get(i2c_slave_addr));
        return 0;
}

static int i2c_temp_addr_set_(int argc, char** argv)
{
        if (argc < 2) {
                (void)printf("Expected 2 arguments, got %i\r\n", argc);
                return E_INVAL;
        }

        uint8_t addr = atoi(argv[1]);
        store_update(i2c_temp_addr, &addr);

        (void)printf("I2C temp slave address set to %i\r\n", (int)addr);

        return 0;
}

static int i2c_temp_addr_get_(int argc, char** argv)
{
        (void)printf("%i\r\n", (int)store_get(i2c_temp_addr));
        return 0;
}

static int temp_(int argc, char** argv)
{
        (void)argc;
        (void)argv;

        uint32_t t = 0;
        ptrdiff_t status =
            i2c_master_recv(store_get(i2c_temp_addr), (uint8_t*)&t, sizeof(t));
        if (status != sizeof(t)) {
                return status < 0 ? -status : E_IO;
        }

        (void)printf("Temperature: %imC\r\n", (int)t);

        return 0;
}

static int fan_invalid_(uint8_t index)
{
     (void)printf("Invalid fan %i, valid range is 0-7\r\n", (int)index);
     
     return -E_INVAL;   
}

static int fanctrl_(int argc, char** argv)
{
        if (argc < 3) {
                (void)printf("Expected 3 arguments, got %i\r\n", argc);
                return -E_INVAL;
        }
        
        uint8_t index = atoi(argv[1]);
        const char* speed = argv[2];
        
        /* Ensure we actually have a valid key, aswell as a null terminator */
        if (strnlen(speed, 10) >= 10) {
                (void)printf("Invalid speed\r\n");
                
                return -E_INVAL;
        }
        if (index >= 8) {
                return fan_invalid_(index);
        }
        
        (void)fan_set_speed(index, speed);
        
        return 0;
}

static int fancheck_(int argc, char** argv)
{
        if (argc < 2) {
                for (uint8_t i = 0; i < 8; i++) {
                        fan_check_speed(i);
                }
                
                return 0;
        }
        
        uint8_t index = atoi(argv[1]);
        if (index >= 8) {
                return fan_invalid_(index);
        }
        
        fan_check_speed(index);
        
        return 0;
}

static void fanspeed_one_(uint8_t index)
{
        uint16_t speed = fan_get_speed(index);
        
        (void)printf("Fan %i speed: %i RPM\r\n", (int)index, (int)speed);
}

static int fanspeed_(int argc, char** argv)
{
        if (argc < 2) {
                for (uint8_t i = 0; i < 8; i++) {
                        fanspeed_one_(i);
                }
                
                return 0;
        }
        
        uint8_t index = atoi(argv[1]);
        if (index >= 8) {
                return fan_invalid_(index);
        }
        
        fanspeed_one_(index);
        
        return 0;
}

static int reboot_(int argc, char** argv)
{
        (void)argc;
        (void)argv;

        RSTCTRL.SWRR = RSTCTRL_SWRST_bm;
        return 0;
}

static int help(int argc, char** argv);

static struct {
        const char* cmd;
        int (*callback)(int, char**);
        const char* help;
        const char* usage;
} cmd_lookup_[] = {
    {"hello", say_hello, "Say Hello!", "<your name>"},
    {
        "i2c_addr_set",
        i2c_addr_set_,
        "Set I2C slave address",
        "<address>",
    },
    {
        "i2c_addr_get",
        i2c_addr_get_,
        "Get I2C slave address",
        "",
    },
    {
        "i2c_temp_addr_set",
        i2c_temp_addr_set_,
        "Set I2C temperature slave address",
        "<address>",
    },
    {
        "i2c_temp_addr_get",
        i2c_temp_addr_get_,
        "Get I2C temperature slave address",
        "<address>",
    },
    {
        "temp",
        temp_,
        "Get temperature (in mC)",
        "",
    },
    {
        "fanctrl",
        fanctrl_,
        "Set speed of fan",
        "<fan_index> <off|low|medium|max>",
    },
    {
        "fancheck",
        fancheck_,
        "Check speed of fan to ensure there is a \r\n\t\tsimilarity between "
        "measured speed and nominal speed.\r\n\t\t"
        "If no index is supplied, all fans will be checked",
        "[<fan_index>]",
    },
    {
        "fanspeed",
        fanspeed_,
        "Get speed of fan. \r\n\t\tIf no index is supplied, all speeds will be "
        "printed.",
        "[<fan_index>]",
    },
    {
        "reboot",
        reboot_,
        "Reboot the MCU",
        "",
    },
    {"help", help, "Show helptext", ""},
};

static int help(int argc, char** argv)
{
        (void)printf("Usage - Description\r\n");

        for (size_t i = 0; i < ARR_LEN_(cmd_lookup_); i++) {
                (void)printf(
                    "\t%s %s - %s\r\n", cmd_lookup_[i].cmd,
                    cmd_lookup_[i].usage, cmd_lookup_[i].help
                );
        }

        return 0;
}

/**
 * @brief Flush the incoming data, until the first termination character is
 * found
 */
static void flush_incoming_(void)
{
        for (char c[1]; usart_read(c, 1) > 0 && c[0] != TERM_CHAR_;) {
        }
}

static int parse_args_(void)
{
        int len = 0;
        char* tok = strtok(sh_buf_.tmpbuf, " ");

        sh_buf_.args[0] = tok;

        for (; tok != NULL && len < ARR_LEN_(sh_buf_.args);) {
                tok = strtok(NULL, " ");
                sh_buf_.args[++len] = tok;
        }

        return len;
}

static int process_cmd_(int argc)
{
        for (size_t i = 0; i < ARR_LEN_(cmd_lookup_); i++) {
                if (strcmp(cmd_lookup_[i].cmd, sh_buf_.args[0]) == 0) {
                        return cmd_lookup_[i].callback(argc, sh_buf_.args);
                }
        }

        return -E_NOENT;
}

void shell_tick(void)
{
        while (usart_read(sh_buf_.tmpbuf + sh_buf_.tmpidx, 1) > 0) {
                char c = sh_buf_.tmpbuf[sh_buf_.tmpidx];

                if (c == DEL_CHAR_) {
                        if (sh_buf_.tmpidx > 0) {
                                sh_buf_.tmpidx--;
                        }
                } else {
                        sh_buf_.tmpidx++;
                }

                /* Data too long, unable to process it */
                if (sh_buf_.tmpidx >= sizeof(sh_buf_.tmpbuf)) {
                        flush_incoming_();
                        sh_buf_.tmpidx = 0;

                        break;
                }

                if (c == TERM_CHAR_) {
                        /* Echo newline */
                        (void)printf("\r\n");

                        sh_buf_.tmpbuf[sh_buf_.tmpidx - 1] = 0;

                        int argc = parse_args_();
                        int ret = process_cmd_(argc);

                        if (ret != 0) {
                                (void)printf(
                                    "%s: %s\r\n", sh_buf_.args[0],
                                    e_str(ret < 0 ? -ret : ret)
                                );
                        }

                        sh_buf_.tmpidx = 0;
                        break;
                } else {
                        /* Echo entered characters */
                        (void)printf("%c", c);
                }
        }
}
