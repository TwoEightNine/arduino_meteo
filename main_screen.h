#include "screen.h"
#include "sensors.h"
#include "clock.h"

// lcd 2004
#include <LiquidCrystal_I2C.h>
#include "lcd_large_digits_2.h"

#define PIN_LCD_BRIGHTNESS 3
#define PIN_INDICATOR_RED 9
#define PIN_INDICATOR_GREEN 10
#define PIN_PHOTO_BRIGHTNESS A1

#define MODE_TEMP_INT 0
#define MODE_HUMIDITY 1
#define MODE_PRESSURE 2
#define MODE_CO2      3
#define MODE_TEMP_EXT 4
#define MODE_TIME     5
#define MODES_COUNT   6

#define HUM_WARN_MIN 40
#define HUM_WARN_MAX 60
#define HUM_URGENT_MIN 30
#define HUM_URGENT_MAX 70
#define PRES_WARN_MIN 720
#define PRES_URGENT_MIN 700
#define PRES_WARN_MAX 780
#define PRES_URGENT_MAX 800
#define CO2_WARN_MIN 0
#define CO2_URGENT_MIN 0
#define CO2_WARN_MAX 1000
#define CO2_URGENT_MAX 2000

struct Sensors {
    float humidity;
    float temperatureInternal;
    uint16_t pressure;  
    float temperatureExternal;
    uint16_t co2ppm;
};

class MainScreen : public Screen {
private:
    SensorsProvider *sensorsProvider;
    Clock *clock;
    LiquidCrystal_I2C *lcd;

    uint8_t mode = MODE_TIME;
    uint8_t warnState = 0;
    uint8_t quality = 100;
    uint8_t isWarnSuppressed = false;
    uint8_t isFirstLaunch = true;
    uint8_t modeChanged = true;

    unsigned long lupdSensors;
    unsigned long lupdWarnings;
    unsigned long lupdClockTick;
    unsigned long lupdTime;
    unsigned long lupdBrightness;

    Sensors lastSensors;
    uint8_t lastBrightnessPercent;

    void updateTime(uint8_t tickTime);
    void updateMainInfo(Sensors& sensors);
    void updateSideInfo(Sensors& sensors);
    void readSensors(Sensors& result);
    void updateQuality();
    uint8_t calculateQuality(Sensors& sensors);
    uint8_t getWarningRank(uint16_t warnMin, uint16_t urgentMin, uint16_t warnMax, uint16_t urgentMax, uint16_t value);
    uint8_t getWarningRank(uint16_t warn, uint16_t urgent, uint16_t value);
    uint8_t readBrightness();

public:
    MainScreen(SensorsProvider *sensorsProvider, Clock *clock, LiquidCrystal_I2C *lcd);
    void loop(); 
    void onButtonClicked(uint8_t btn);
};
