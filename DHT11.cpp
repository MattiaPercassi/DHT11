#include <pigpio.h>
#include <iostream>
#include <chrono>
#include "DHT11.h"

DHT::DHT(uint8_t p) : pin{p}
{
    timeoutLoops = 200;                         // placeholder number
    timeoutms = std::chrono::microseconds(200); // timeout after 200 us
    gpioSetMode(pin, PI_OUTPUT);
    gpioWrite(pin, 1);
    // wait 2 seconds for ensuring proper connection
    auto start = std::chrono::high_resolution_clock::now();
    while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start) <= std::chrono::seconds(2))
        ;
};

int DHT::readData()
{

    // initially the pin must send in output a low signal
    if (gpioSetMode(pin, PI_OUTPUT) != 0)
    {
        return 1; // error with library
    };
    gpioWrite(pin, 0); // send communication
    // wait for 20 ms, at least 18 as per data sheet
    auto start = std::chrono::high_resolution_clock::now();
    while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start) <= std::chrono::milliseconds(20))
        ;
    gpioWrite(pin, 1);
    gpioSetMode(pin, PI_INPUT);

    /*  Response from sensor is LOW-HIGH-TRANSMISSION, timing is 80us-80us-4ms
        First listen to high while waiting for the sensor to start the reply, then listen to low and ensure is not timing out, then listen to high and ensure is not timing out, then start listening for data
    */
    start = std::chrono::high_resolution_clock::now();
    while (gpioRead(pin) != 0)
    {
        if (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start) >= timeoutms)
            return 2;
    };
    start = std::chrono::high_resolution_clock::now();
    while (gpioRead(pin) == 0)
    {
        if (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start) >= timeoutms)
            return 3; // communication error
    };
    start = std::chrono::high_resolution_clock::now();
    while (gpioRead(pin) != 0)
    {
        if (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start) >= timeoutms)
            return 4;
    };

    /*If there have been no errors means the communication is starting correctly.
    Each bit is sent as a LOW-HIGH couple with timing 50us - 28 us for 0 and 50us - 70us for 1
    To define short and long HIGH values we use a counter
    */
    uint8_t state{0};
    uint8_t pstate{0};
    std::chrono::microseconds zeroLoop{0};
    uint8_t mask = 128; // bitwise 10000000
    uint8_t data{0};    // bitwise 00000000
    auto delta = std::chrono::microseconds(10);
    start = std::chrono::high_resolution_clock::now();
    for (int i{0}; i < 40;)
    {
        state = gpioRead(pin);
        if (state == 0 && pstate != 0) // change of voltage high to low indicates change of bit
        {
            // first bit is a zero, used to calibrate the length in loops
            if (i == 0)
            {
                zeroLoop = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start); // average length of the loop
            }
            // since loopsCounter is reset only when bit changes we can check if it was a "short bit", a 0 or a "long bit" a 1
            else if (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start) >= zeroLoop + delta) // this is a 1
            {
                data |= mask; // write 1 in the current location of the 1 in mask MSBF (most significant bit first)
            }
            mask >>= 1;    // i.e. 10000000 -> 01000000 etc.
            if (mask == 0) // after 8 bits the mask is 0, the values need to be written to buffer and the next set of bits need to be read
            {
                bits[i / 8] = data;
                data = 0;
                mask = 128;
            };
            start = std::chrono::high_resolution_clock::now(); // reset start timer
            i++;
        }
        pstate = state;
        if (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start) >= timeoutms)
            return 5;
    };

    // once the communication is ended and all the bits are recorded we need to do the test for read correctness and translate the data into humidity and temperature floats
    uint8_t validationSum{0};
    for (size_t i{0}; i < 4; ++i)
    {
        validationSum += bits[i];
    };
    if (validationSum != bits[4])
        return 6; // wrong read

    // test code for validate the reading of data
    for (size_t i{0}; i < 5; ++i)
    {
        std::cout << i << ": " << static_cast<int>(bits[i]) << std::endl;
    };
    return 0;
};

float DHT::getTemp()
{
    return temperature;
};

float DHT::getHum()
{
    return humidity;
};