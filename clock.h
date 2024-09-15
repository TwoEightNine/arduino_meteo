#include "Arduino.h"

#include <ThreeWire.h>  
#include <RtcDS1302.h>

class Clock {
private:
    RtcDS1302<ThreeWire> *rtc;

public:
    Clock();
    uint32_t getDateTime();
};