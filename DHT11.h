#ifndef _DHT11_H_
#define _DHT11_H_

#include <pigpio.h>
#include <chrono>

class DHT
{
protected:
    uint8_t pin;
    uint8_t bits[5]; // buffer of 5 8-bit unsigned int to receive data
    float temperature;
    float humidity;

    std::chrono::milliseconds requestTime;
    std::chrono::microseconds timeoutTime;
    std::chrono::microseconds responseTolerance;

public:
    DHT(uint8_t p);
    int readData();
    float getTemp();
    float getHum();
};

#endif