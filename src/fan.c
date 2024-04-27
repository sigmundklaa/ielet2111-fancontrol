#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>
#include <string.h>
#include <util/delay.h>

#define TCB_INSTANCE TCB0

/*fan modes/percentages*/
#define off (0)
#define low (40)
#define medium (70)
#define max (100)

/*from fan datasheet graph: rpm corresponding to percentage*/
static const int SUPPOSED_OFF_RPM = 0;
static const int SUPPOSED_LOW_RPM = 3500;
static const int SUPPOSED_MEDIUM_RPM = 8000;
static const int SUPPOSED_MAX_RPM = 13100;

static uint32_t pulse[8]; // Array to store the pulse values for 8 fans
static uint32_t rpm[8];   // Array to store the RPM values for 8 fans
/*PB0, PB1, PB2, PB3, PB4, PB5, PD2, PD3*/
static int fan_speeds[] = {low, low, low, low, low, low, low, low};

#define PERIOD (9)
#define fanspeed_1 ((9 / 100.0) * fan_speeds[0])
#define fanspeed_2 ((9 / 100.0) * fan_speeds[1])
#define fanspeed_3 ((9 / 100.0) * fan_speeds[2])
#define fanspeed_4 ((9 / 100.0) * fan_speeds[3])
#define fanspeed_5 ((9 / 100.0) * fan_speeds[4])
#define fanspeed_6 ((9 / 100.0) * fan_speeds[5])
#define fanspeed_7 ((9 / 100.0) * fan_speeds[6])
#define fanspeed_8 ((9 / 100.0) * fan_speeds[7])

/*prototypes*/
void TCA0_init(void);
void TCA1_init(void);
void PORT_init(void);
void check_fan_speeds(uint8_t fan_index);

static register8_t* speed_register(uint8_t fan_index)
{
        switch (fan_index) {
        case 0:
                return &TCA0.SPLIT.LCMP0;
        case 1:
                return &TCA0.SPLIT.LCMP1;
        case 2:
                return &TCA0.SPLIT.LCMP2;
        case 3:
                return &TCA0.SPLIT.HCMP0;
        case 4:
                return &TCA0.SPLIT.HCMP1;
        case 5:
                return &TCA0.SPLIT.HCMP2;
        case 6:
                return &TCA1.SPLIT.LCMP2;
        case 7:
                return &TCA1.SPLIT.HCMP0;
        }

        return NULL;
}

void PORT_init(void)
{
        /* set pins of PORT D and B as PWM outputs */
        PORTD.DIR |= PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm | PIN4_bm | PIN5_bm;
        PORTB.DIR |= PIN2_bm | PIN3_bm;

        /* tacho (read) pins */
        PORTC.DIRCLR = PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm | PIN4_bm |
                       PIN5_bm | PIN6_bm | PIN7_bm;
        /* Enable pull-up resistor */
        PORTC.PIN0CTRL |= PORT_PULLUPEN_bm;
        PORTC.PIN1CTRL |= PORT_PULLUPEN_bm;
        PORTC.PIN2CTRL |= PORT_PULLUPEN_bm;
        PORTC.PIN3CTRL |= PORT_PULLUPEN_bm;
        PORTC.PIN4CTRL |= PORT_PULLUPEN_bm;
        PORTC.PIN5CTRL |= PORT_PULLUPEN_bm;
        PORTC.PIN6CTRL |= PORT_PULLUPEN_bm;
        PORTC.PIN7CTRL |= PORT_PULLUPEN_bm;
}

// Initialize TCB for frequency measurement
void TCB_init(void)
{
        TCB0.CTRLA = TCB_CLKSEL_DIV1_gc |
                     TCB_ENABLE_bm; // Set clock division factor to 1
        TCB0.CTRLB = TCB_CNTMODE_FRQ_gc |
                     TCB_CCMPEN_bm; // Set TCB to frequency measurement mode and
                                    // enable Compare/Capture output
        TCB0.CCMP = 0xFFFF;         // Set TOP value to maximum
        TCB0.EVCTRL = TCB_CAPTEI_bm; // Enable event input
        TCB0.INTCTRL = TCB_CAPT_bm;  // Enable capture interrupt
        EVSYS.USERTCB0CAPT = EVSYS_USER_CHANNEL2_gc;
        EVSYS.CHANNEL2 =
            EVSYS_CHANNEL2_PORTC_PIN0_gc; // Change to the first tacho pin
}

void TCA0_init(void)
{
        /* set waveform output on PORT A */
        PORTMUX.TCAROUTEA = PORTMUX_TCA0_PORTD_gc;

        /* enable split mode */
        TCA0.SPLIT.CTRLD = TCA_SPLIT_SPLITM_bm;

        TCA0.SPLIT.CTRLB = TCA_SPLIT_HCMP0EN_bm | TCA_SPLIT_HCMP1EN_bm |
                           TCA_SPLIT_HCMP2EN_bm /* enable compare channel
for the higher byte */
                           | TCA_SPLIT_LCMP0EN_bm | TCA_SPLIT_LCMP1EN_bm |
                           TCA_SPLIT_LCMP2EN_bm; /* enable compare channel
for the lower byte */

        /* set the PWM frequencies and duty cycles */
        TCA0.SPLIT.LPER = PERIOD;
        TCA0.SPLIT.LCMP0 = fanspeed_1;
        TCA0.SPLIT.LCMP1 = fanspeed_2;
        TCA0.SPLIT.LCMP2 = fanspeed_3;

        TCA0.SPLIT.HPER = PERIOD;
        TCA0.SPLIT.HCMP0 = fanspeed_4;
        TCA0.SPLIT.HCMP1 = fanspeed_5;
        TCA0.SPLIT.HCMP2 = fanspeed_6;

        TCA0.SPLIT.CTRLA = TCA_SPLIT_CLKSEL_DIV16_gc /* set clock source
        (sys_clk/16) */
                           | TCA_SPLIT_ENABLE_bm;    /* start timer */
}

void TCA1_init(void)
{
        /* set waveform output on PORT A */
        PORTMUX.TCAROUTEA = PORTMUX_TCA1_PORTB_gc;

        /* enable split mode */
        TCA1.SPLIT.CTRLD = TCA_SPLIT_SPLITM_bm;

        TCA1.SPLIT.CTRLB = TCA_SPLIT_HCMP0EN_bm | TCA_SPLIT_HCMP1EN_bm |
                           TCA_SPLIT_HCMP2EN_bm    /* enable compare channel
for the higher byte */
                           | TCA_SPLIT_LCMP2EN_bm; /* enable compare channel
                            for the lower byte */

        /* set the PWM frequencies and duty cycles */
        TCA1.SPLIT.LPER = PERIOD;
        TCA1.SPLIT.LCMP2 = fanspeed_7;

        TCA1.SPLIT.HPER = PERIOD;
        TCA1.SPLIT.HCMP0 = fanspeed_8;

        TCA1.SPLIT.CTRLA = TCA_SPLIT_CLKSEL_DIV16_gc /* set clock source
        (sys_clk/16) */
                           | TCA_SPLIT_ENABLE_bm;    /* start timer */
}

// TCB0 interrupt routine
ISR(TCB0_INT_vect)
{
        static uint8_t current_tacho_pin = 0;
        uint8_t next_tacho_pin = (current_tacho_pin + 1) % 8;

        pulse[current_tacho_pin] =
            TCB0.CCMP; // Store the new capture value for the current tacho pin

        rpm[current_tacho_pin] =
            ((1000000000UL) / (pulse[current_tacho_pin] * 250 * 2)) * 60;

        // Switch to the next tacho pin
        EVSYS.CHANNEL2 = EVSYS_CHANNEL2_PORTC_PIN0_gc + next_tacho_pin;

        current_tacho_pin = next_tacho_pin;
}

void fan_init(void)
{
        TCB_init(); // Initialize TCB
        PORT_init();
        TCA1_init();
        TCA0_init();
}

void fan_check_speed(uint8_t fan_index)
{
        int threshold;

        /* debug
        printf("Fan RPM: %d\r\n", (int)fan_speeds[fan_index]);
        printf("max: %d\r\n", max);
        printf("medium: %d\r\n", medium);
        printf("low: %d\r\n", low);
        */

        if (fan_speeds[fan_index] == max) {
                threshold = SUPPOSED_MAX_RPM;
        } else if (fan_speeds[fan_index] == medium) {
                threshold = SUPPOSED_MEDIUM_RPM;
        } else if (fan_speeds[fan_index] == low) {
                threshold = SUPPOSED_LOW_RPM;
        } else {
                threshold = SUPPOSED_OFF_RPM;
        }

        if (rpm[fan_index] < threshold - 1500) {
                printf(
                    "Error: Fan speed %d is too low, should be: %d\r\n",
                    fan_index + 1, (int)threshold
                );
        }
}

void fan_set_speed(uint8_t fan_index, const char* speed)
{
        int duty_cycle = off;
        register8_t* reg = NULL;

        if (strcmp(speed, "max") == 0) {
                duty_cycle = max;
        } else if (strcmp(speed, "medium") == 0) {
                duty_cycle = medium;
        } else if (strcmp(speed, "low") == 0) {
                duty_cycle = low;
        }

        reg = speed_register(fan_index);
        if (reg == NULL) {
                return;
        }

        fan_speeds[fan_index] = duty_cycle;
        *reg = (uint8_t)duty_cycle;
}

uint16_t fan_get_speed(uint8_t fan_index)
{
        return (uint16_t)rpm[fan_index];
}