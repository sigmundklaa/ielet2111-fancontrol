
#include <stdio.h>
#include <string.h>

#include <avr/interrupt.h>

#include "drivers/usart.h"
#include "error.h"

#define BUF_SIZE_ (64)
#define TERM_CHAR_ ('\r')

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

static int help(int argc, char** argv);

static struct {
        const char* cmd;
        int (*callback)(int, char**);
        const char* help;
        const char* usage;
} cmd_lookup_[] = {
    {"hello", say_hello, "Say Hello!", "hello <your name>"},
    {"help", help, "Show helptext", "help"},
};

static int help(int argc, char** argv)
{
        (void)printf("Usage - Description\r\n");

        for (size_t i = 0; i < ARR_LEN_(cmd_lookup_); i++) {
                (void)printf(
                    "\t%s - %s\r\n", cmd_lookup_[i].usage, cmd_lookup_[i].help
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
        int len = 1;
        char* tok = strtok(sh_buf_.tmpbuf, " ");

        sh_buf_.args[0] = tok;

        for (; tok != NULL && len < ARR_LEN_(sh_buf_.args); len++) {
                tok = strtok(NULL, " ");
                sh_buf_.args[len] = tok;
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

        return -EINVAL;
}

void shell_tick(void)
{
        while (usart_read(sh_buf_.tmpbuf + sh_buf_.tmpidx, 1) > 0) {
                char c = sh_buf_.tmpbuf[sh_buf_.tmpidx];
                sh_buf_.tmpidx++;

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
                                    "Execution of %s failed: %i\r\n",
                                    sh_buf_.args[0], ret
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
