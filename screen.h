#include "Arduino.h"

class Screen {

public:
    Screen() {};
    ~Screen() {};
    virtual void loop() = 0; 
    virtual void onButtonClicked(uint8_t btn) = 0;
};