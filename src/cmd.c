
#include <stdio.h>

#include "drivers/i2c.h"

struct __attribute__((packed)) cmd_packet_ {
        uint8_t cmd;
        uint8_t arg_len;
        uint8_t args[];        
};

enum cmd_type_ {
        CMD_MIN_ = 0x0,
        CMD_REPORT_ = 0x0,
        CMD_HELLO_,
        CMD_MAX_,        
};

typedef int (*cmd_fn_)(struct cmd_packet_* packet);

static int report_(struct cmd_packet_* packet)
{
        return 0;
}

static int hello_(struct cmd_packet_* packet)
{
        (void)i2c_slave_send("hey", 4);
        return 0;
}

static cmd_fn_ commands[] = {
        [CMD_REPORT_] = report_,
        [CMD_HELLO_] = hello_,        
};

void cmd_tick(void)
{
        static uint32_t ticks;
        static uint8_t buf[100];
        
        /* A delay is required to ensure we don't read too fast, as that will
         * not give the buffer time to fill up. The possibility of a collision
         * is still possible, but the chances are very slim.
         */
        if (++ticks < 10000) {
                return;
        }
        
        ticks = 0;
        
        size_t size = i2c_slave_recv(buf, sizeof(buf));
        if (size < sizeof(struct cmd_packet_)) {
                return;
        }
        
        struct cmd_packet_* packet = (void*)buf;
        if (packet->cmd >= CMD_MAX_) {
                (void)printf("Command packet too big, dropped\r\n");
                
                return;
        }
        
        cmd_fn_ fn = commands[packet->cmd];
        if (fn == NULL) {
                /* Invalid command ID */
                return;
        }
        
        fn(packet);
}