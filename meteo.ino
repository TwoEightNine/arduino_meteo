// lcd 2004
#include <LiquidCrystal_I2C.h>
#include "lcd_large_digits_2.h"

// ds1302
#include <ThreeWire.h>  
#include <RtcDS1302.h>

#include "main_screen.h"

#define DEBUG true
#define DEBUG_SERIAL if(DEBUG)Serial

// pins
#define PIN_RTC_DAT 4
#define PIN_RTC_CLK 5
#define PIN_RTC_RST 6
#define PIN_BTN_1 7
#define PIN_BTN_2 8
// sensors configs
#define LCD_WIDTH 20
#define LCD_HEIGHT 4

// constants
#define CHAR_DEGREE 10

LiquidCrystal_I2C *lcd;

RtcDS1302<ThreeWire> *rtc;

SensorsProvider *sensorsProvider;
MainScreen *mainScreen;


void setup() {
    lcd = new LiquidCrystal_I2C(0x27, LCD_WIDTH, LCD_HEIGHT);
    lcd->begin();
    lcd->backlight();
    lcd->setCursor(5, 1);
    lcd->print(F("Loading..."));

    sensorsProvider = new SensorsProvider();

    pinMode(PIN_LCD_BRIGHTNESS, OUTPUT);
    pinMode(PIN_BTN_1, INPUT);
    analogWrite(PIN_LCD_BRIGHTNESS, 128);
    pinMode(PIN_INDICATOR_RED, OUTPUT);
    pinMode(PIN_INDICATOR_GREEN, OUTPUT);

    initLargeChars(*lcd);

    DEBUG_SERIAL.begin(9600);
    DEBUG_SERIAL.print(__DATE__);
    DEBUG_SERIAL.println(__TIME__);

    ThreeWire myWire(PIN_RTC_DAT, PIN_RTC_CLK, PIN_RTC_RST);
    rtc = new RtcDS1302<ThreeWire>(myWire);
    rtc->Begin();

    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

    if (!rtc->IsDateTimeValid()) {
        rtc->SetDateTime(compiled);
    }

    if (rtc->GetIsWriteProtected()) {
        rtc->SetIsWriteProtected(false);
    }

    if (!rtc->GetIsRunning()) {
        rtc->SetIsRunning(true);
    }

    RtcDateTime now = rtc->GetDateTime();
    if (now < compiled) {
        rtc->SetDateTime(compiled);
    }

    mainScreen = new MainScreen(sensorsProvider, rtc, lcd);
}

void loop() {
    mainScreen->loop();

    if (digitalRead(PIN_BTN_1)) {
        while (digitalRead(PIN_BTN_1)) ;
        mainScreen->onButtonClicked(1);
    }
}
