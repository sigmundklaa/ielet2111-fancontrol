#include <Arduino.h>
#include <Wire.h>

#define R_25 (10000UL)

static uint32_t read_temp_(void)
{
    uint32_t voltage, resistance, temp;

    voltage = (analogRead(A0) * 5000UL) >> 10;
    resistance = ((5000 - voltage) * R_25) / voltage;

    /* Temperature in mC */
    temp = 1000UL;
    temp *= 4000UL * 298 / (4000 + 298 * (log(resistance) - log(R_25)));
    temp -= 273000;

    return temp;
}

void requestEvent()
{
    uint32_t temp = read_temp_();
    Serial.println(temp);

    Wire.write((uint8_t *)&temp, sizeof(temp));
}

void setup()
{
    pinMode(A0, INPUT);

    Wire.begin(5);
    Wire.onRequest(requestEvent);
    Serial.begin(9600);
}
void loop() { delay(2000); }
