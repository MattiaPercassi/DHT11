#include <pigpio.h>
#include <iostream>

class DHT
{
protected:
    uint8_t pin;
    uint8_t bits[5]; // buffer of 5 8-bit unsigned int to receive data
    float temperature;
    float humidity;

public:
    DHT(uint8_t p) : pin{p} {};
    int readData();
    float getTemp();
    float getHum();
};