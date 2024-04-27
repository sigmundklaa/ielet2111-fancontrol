#include <Arduino.h>
#include <Wire.h>

#define CONTROLLER_ (1)

struct __attribute__((packed)) packet_ {
    uint8_t cmd;
    uint8_t arg_len;
};

void setup()
{
    Wire.begin();
    Serial.begin(9600);
}

void loop()
{
    delay(5000);

    struct packet_ p = {
        0x0,
        0,
    };

    Serial.println("Reading speeds");
    Wire.beginTransmission(9);
    Wire.write((uint8_t *)&p, sizeof(p));
    Wire.endTransmission();
    delay(100);

    Wire.requestFrom(9, 17);
    delay(100);

    uint16_t arr[8] = {0};

    for (uint8_t i = 0; Wire.available(); i++) {
        delay(20);
        int c = Wire.read();
        if (c == -1) {
            break;
        }
        if (i == 0) {
            continue;
        }
        arr[(i - 1) >> 1] |= ((i - 1) & 1) ? (c << 8) : c;
    }

    Serial.println("Fan\tSpeed");

    for (uint8_t i = 0; i < 8; i++) {
        Serial.print((unsigned int)i);
        Serial.print("\t");
        Serial.print((unsigned int)arr[i]);
        Serial.println(" RPM");
    }

    Serial.println();
}
