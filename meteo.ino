// dht11
#include <DHT_U.h>

// lcd 2004
#include <LiquidCrystal_I2C.h>
#include "lcd_large_digits_2.h"

// bmp280
#include <Adafruit_BMP280.h>

// ds1302
#include <ThreeWire.h>  
#include <RtcDS1302.h>

// d18b20
#include <OneWire.h>
#include <DallasTemperature.h>

//mh-z19b
#include "MHZ19_uart.h"

#define DEBUG false
#define DEBUG_SERIAL if(DEBUG)Serial

// pins
#define PIN_DHT 2
#define PIN_LCD_BRIGHTNESS 3
#define PIN_RTC_DAT 4
#define PIN_RTC_CLK 5
#define PIN_RTC_RST 6
#define PIN_BTN_1 7
#define PIN_BTN_2 8
#define PIN_INDICATOR_RED 9
#define PIN_INDICATOR_GREEN 10
// #define PIN_INDICATOR_BLUE 11
#define PIN_MHZ_TX 11
#define PIN_MHZ_RX 12
#define PIN_D18B20 13
#define PIN_PHOTO_BRIGHTNESS A1

// sensors configs
#define DHTTYPE DHT11
#define LCD_WIDTH 20
#define LCD_HEIGHT 4

// constants
#define CHAR_DEGREE 10

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

#define TEMP_EXTERNAL_NONE 666.13
#define HUMID_NONE 0
#define CO2_NONE -1

#define MODE_TEMP_INT 0
#define MODE_HUMIDITY 1
#define MODE_PRESSURE 2
#define MODE_CO2      3
#define MODE_TEMP_EXT 4
#define MODE_TIME     5
#define MODES_COUNT   6

OneWire oneWire(PIN_D18B20);
DallasTemperature sensors(&oneWire);

DHT_Unified dht(PIN_DHT, DHTTYPE);
Adafruit_BMP280 bmp;
LiquidCrystal_I2C lcd(0x27, LCD_WIDTH, LCD_HEIGHT);

ThreeWire myWire(PIN_RTC_DAT, PIN_RTC_CLK, PIN_RTC_RST);
RtcDS1302<ThreeWire> Rtc(myWire);

MHZ19_uart mhz19;

struct Sensors {
    float humidity;
    float temperatureInternal;
    uint16_t pressure;  
    float temperatureExternal;
    uint16_t co2ppm;
};

uint8_t mode = MODE_TIME;
uint8_t warnState = 0;
uint8_t quality = 100;
uint8_t isWarnSuppressed = false;
uint8_t isFirstLaunch = true;

unsigned long lupdSensors;
unsigned long lupdWarnings;
unsigned long lupdClockTick;
unsigned long lupdTime;
unsigned long lupdBrightness;

Sensors lastSensors;
uint8_t lastBrightnessPercent;


void setup() {
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(5, 1);
    lcd.print(F("Loading..."));

    bmp.begin(0x76);
    bmp.setSampling(
        Adafruit_BMP280::MODE_NORMAL, // Режим работы
        Adafruit_BMP280::SAMPLING_X2, // Точность изм. температуры
        Adafruit_BMP280::SAMPLING_X16, // Точность изм. давления
        Adafruit_BMP280::FILTER_X16, // Уровень фильтрации
        Adafruit_BMP280::STANDBY_MS_500
    );

    dht.begin();

    pinMode(PIN_LCD_BRIGHTNESS, OUTPUT);
    pinMode(PIN_BTN_1, INPUT);
    analogWrite(PIN_LCD_BRIGHTNESS, 128);
    pinMode(PIN_INDICATOR_RED, OUTPUT);
    pinMode(PIN_INDICATOR_GREEN, OUTPUT);

    initLargeChars(lcd);
    sensors.begin();

    DEBUG_SERIAL.begin(9600);
    DEBUG_SERIAL.print(__DATE__);
    DEBUG_SERIAL.println(__TIME__);

    Rtc.Begin();

    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

    if (!Rtc.IsDateTimeValid()) {
        Rtc.SetDateTime(compiled);
    }

    if (Rtc.GetIsWriteProtected()) {
        Rtc.SetIsWriteProtected(false);
    }

    if (!Rtc.GetIsRunning()) {
        Rtc.SetIsRunning(true);
    }

    RtcDateTime now = Rtc.GetDateTime();
    if (now < compiled) {
        Rtc.SetDateTime(compiled);
    }

    mhz19.begin(PIN_MHZ_TX, PIN_MHZ_RX);
    mhz19.setAutoCalibration(false);
    mhz19.getStatus();
    // delay(500);
}

void loop() {
    bool modeChanged = false;
    bool sensorsUpdated = false;

    if (digitalRead(PIN_BTN_1)) {
        while (digitalRead(PIN_BTN_1)) ;
        mode++;
        mode = mode % MODES_COUNT;
        modeChanged = true;
    }

    // if (digitalRead(PIN_D18B20)) {
    //     mode = MODE_TEMP_EXT;
    //     modeChanged = true;
    // }

    if (millis() - lupdSensors >= 5000 || isFirstLaunch) {
        readSensors(lastSensors);
        lupdSensors = millis();
        updateSideInfo(lastSensors);
        // warnState = calculateWanrState(lastSensors);
        quality = calculateQuality(lastSensors);
        updateQuality();
        sensorsUpdated = true;
    }

    if (modeChanged || sensorsUpdated) {
        updateMainInfo(lastSensors);
    }

    if (millis() - lupdTime >= 100) {
        uint8_t tick = (millis() % 2000) >= 1000;
        updateTime(tick);
        lupdTime = millis();
    }

    // if (millis() - lupdWarnings >= 500) {
    //     updateWarnings();
    //     lupdWarnings = millis();
    // }

    if (millis() - lupdBrightness >= 100) {
        lastBrightnessPercent = readBrightness();
        analogWrite(PIN_LCD_BRIGHTNESS, round(lastBrightnessPercent * 2.55));
        lupdBrightness = millis();
    }

    isFirstLaunch = false;
    delay(50);
}

void updateQuality() {
    uint8_t greenValue = 255 * quality / 100;
    uint8_t redValue = 255 * (100 - quality) / 100;
    DEBUG_SERIAL.print("Green: ");
    DEBUG_SERIAL.println(greenValue);
    DEBUG_SERIAL.print("Red: ");
    DEBUG_SERIAL.println(redValue);
    analogWrite(PIN_INDICATOR_GREEN, greenValue);
    analogWrite(PIN_INDICATOR_RED, redValue);
}

void updateWarnings() {
    if (warnState >= 100) {
        bool first500 = (millis() % 1000) < 500;
        digitalWrite(PIN_INDICATOR_RED, first500);
        digitalWrite(PIN_INDICATOR_GREEN, 0);
    } else if (warnState == 0) {
        digitalWrite(PIN_INDICATOR_RED, 0);
        digitalWrite(PIN_INDICATOR_GREEN, 0);
    } else {
        digitalWrite(PIN_INDICATOR_RED, 1);
        uint8_t greenValue = ((float) (100 - warnState)) / 100 * 255;
        analogWrite(PIN_INDICATOR_GREEN, greenValue);
    }
}

void updateTime(uint8_t tickTime) {
    if (mode == MODE_TIME) {
        RtcDateTime now = Rtc.GetDateTime();
        
        uint8_t hours = now.Hour();
        uint8_t minutes = now.Minute();
        
        printLargeTime(lcd, hours, minutes, tickTime);
        lcd.setCursor(0, 3);

        uint8_t day = now.Day();
        uint8_t month = now.Month();
        uint8_t dayOfWeek = now.DayOfWeek();

        __FlashStringHelper* dayOfWeekName;
        if (dayOfWeek == 0) {
            dayOfWeekName = F("sun ");
        } else if (dayOfWeek == 1) {
            dayOfWeekName = F("mon ");
        } else if (dayOfWeek == 2) {
            dayOfWeekName = F("tue ");
        } else if (dayOfWeek == 3) {
            dayOfWeekName = F("wed ");
        } else if (dayOfWeek == 4) {
            dayOfWeekName = F("thu ");
        } else if (dayOfWeek == 5) {
            dayOfWeekName = F("fri ");
        } else if (dayOfWeek == 6) {
            dayOfWeekName = F("sat ");
        } else {
            dayOfWeekName = F("wtf ");
        }

        lcd.print(dayOfWeekName);
        lcd.print(day);
        lcd.print(F("."));
        lcd.print(month);
        lcd.print(F("     "));
    }
}

void updateMainInfo(Sensors& sensors) {
    if (mode == MODE_TEMP_INT) {
        printLargeFloat(lcd, sensors.temperatureInternal);
        lcd.setCursor(0, 3);
        lcd.print(F("temp. b/i  "));
    } else if (mode == MODE_HUMIDITY) {
        printLargeFloat(lcd, sensors.humidity);
        lcd.setCursor(0, 3);
        lcd.print(F("humidity   "));
    } else if (mode == MODE_PRESSURE) {
        printLargeInt(lcd, sensors.pressure);
        lcd.setCursor(0, 3);
        lcd.print(F("pressure   "));
    } else if (mode == MODE_TEMP_EXT) {
        if (sensors.temperatureExternal != TEMP_EXTERNAL_NONE) {
            printLargeFloat(lcd, sensors.temperatureExternal);
        } else {
            printLargeFloat(lcd, 0.0);
        }
        lcd.setCursor(0, 3);
        lcd.print(F("temp. ext. "));
    } else if (mode == MODE_CO2) {
        printLargeInt(lcd, sensors.co2ppm);
        lcd.setCursor(0, 3);
        lcd.print(F("co2 ppm    "));
    }
}

void updateSideInfo(Sensors& sensors) {
    lcd.setCursor(17, 0);
    lcd.print((int) round(sensors.temperatureInternal));
    lcd.write(0xdf);

    lcd.setCursor(17, 1);
    if (sensors.humidity != HUMID_NONE) {
        lcd.print((int) round(sensors.humidity));
        lcd.print('%');
    } else {
        lcd.print(F("   "));
    }
  
    lcd.setCursor(17, 2);
    lcd.print(sensors.pressure);

    lcd.setCursor(17, 3);
    if (sensors.temperatureExternal != TEMP_EXTERNAL_NONE) {
        lcd.print((int) round(sensors.temperatureExternal));
        lcd.write(0xdf);
    } else if (sensors.co2ppm != CO2_NONE) {
        uint8_t thousands = sensors.co2ppm / 1000;
        uint8_t hundreds = (int)(sensors.co2ppm % 1000) / 100;
        lcd.print(thousands);
        lcd.print('.');
        lcd.print(hundreds);
    } else {
        lcd.print(F("   "));
    }
}

uint8_t readBrightness() {
    uint16_t brValue = analogRead(PIN_PHOTO_BRIGHTNESS);
    uint8_t brightnessPercent = (uint8_t) round(((float) brValue) / 10.24);
    DEBUG_SERIAL.print("read brightness: ");
    DEBUG_SERIAL.println(brightnessPercent);
    return brightnessPercent;
}

void readSensors(Sensors& result) {
    sensors_event_t event;
  
    dht.temperature().getEvent(&event);
    float temp_dht = event.temperature;

    dht.humidity().getEvent(&event);
    float hum = event.relative_humidity;

    float temp_bmp = bmp.readTemperature();
    uint16_t pressure = round(bmp.readPressure() / 133.3);

    float temp;
    if (temp_dht != 0 && temp_bmp != 0) {
        temp = (temp_dht + temp_bmp) / 2.0;
    } else if (temp_bmp != 0) {
        temp = temp_bmp;
    } else {
        temp = temp_dht;
    }

    result.humidity = hum;
    result.temperatureInternal = temp;
    result.pressure = pressure;

    uint8_t sensorSignal = digitalRead(PIN_D18B20);
    DEBUG_SERIAL.print("read d18b20: ");
    DEBUG_SERIAL.println(sensorSignal);

    if (sensorSignal) {
        sensors.requestTemperatures();
        float c = sensors.getTempCByIndex(0);
        bool isConnected = c != DEVICE_DISCONNECTED_C;

        if (isConnected) {
            result.temperatureExternal = c;
        } else {
            result.temperatureExternal = TEMP_EXTERNAL_NONE;
        }
        DEBUG_SERIAL.print("temp d18b20: ");
        DEBUG_SERIAL.println(c);
    } else {
        result.temperatureExternal = TEMP_EXTERNAL_NONE;
    }

    int ppm = mhz19.getPPM();
    if (ppm != CO2_NONE) {
        result.co2ppm = (uint16_t) ppm; 
        DEBUG_SERIAL.print("read co2: ");
        DEBUG_SERIAL.println(result.co2ppm);
    } else {
        result.co2ppm = CO2_NONE;
    }
}

uint8_t calculateWanrState(Sensors& sensors) {
    uint8_t humWarnState = getWarningRank(
        HUM_WARN_MIN, HUM_URGENT_MIN,
        HUM_WARN_MAX, HUM_URGENT_MAX,
        sensors.humidity
    );

    uint8_t presWarnState = getWarningRank(
        PRES_WARN_MIN, PRES_URGENT_MIN,
        PRES_WARN_MAX, PRES_URGENT_MAX,
        sensors.pressure
    );

    return max(presWarnState, humWarnState);
}

uint8_t calculateQuality(Sensors& sensors) {
    uint8_t humWarnState = 100 - getWarningRank(
        HUM_WARN_MIN, HUM_URGENT_MIN,
        HUM_WARN_MAX, HUM_URGENT_MAX,
        sensors.humidity
    );

    uint8_t presWarnState = 100 - getWarningRank(
        PRES_WARN_MIN, PRES_URGENT_MIN,
        PRES_WARN_MAX, PRES_URGENT_MAX,
        sensors.pressure
    );

    uint8_t co2WarnState = 100 - getWarningRank(
        CO2_WARN_MIN, CO2_URGENT_MIN,
        CO2_WARN_MAX, CO2_URGENT_MAX,
        sensors.co2ppm
    );

    DEBUG_SERIAL.print("Humidity quality: ");
    DEBUG_SERIAL.println(humWarnState);
    DEBUG_SERIAL.print("Pressure quality: ");
    DEBUG_SERIAL.println(presWarnState);
    DEBUG_SERIAL.print("CO2 quality: ");
    DEBUG_SERIAL.println(co2WarnState);

    return (uint8_t) ((presWarnState + humWarnState + co2WarnState) / 3);
}

uint8_t getWarningRank(uint16_t warnMin, uint16_t urgentMin, uint16_t warnMax, uint16_t urgentMax, uint16_t value) {
    if (value <= urgentMin || value >= urgentMax) return 100;
    if (value > warnMin && value < warnMax) return 0;

    if (value <= warnMin) {
        return getWarningRank(warnMin, urgentMin, value);
    } else {
        return getWarningRank(warnMax, urgentMax, value);
    }
}

uint8_t getWarningRank(uint16_t warn, uint16_t urgent, uint16_t value) {
    if (value >= urgent) return 100;
    if (value <= warn) return 0;

    float f = ((float) (value - warn)) / (urgent - warn);
    return (uint8_t) (100 * f);
}
