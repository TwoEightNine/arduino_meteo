#include "main_screen.h"


MainScreen::MainScreen(SensorsProvider *sensorsProvider, RtcDS1302<ThreeWire> *rtc, LiquidCrystal_I2C *lcd) {
    this->sensorsProvider = sensorsProvider;
    this->rtc = rtc;
    this->lcd = lcd;
}

void MainScreen::readSensors(Sensors& result) {
    result.humidity = this->sensorsProvider->readHumidity();
    result.temperatureInternal = this->sensorsProvider->readTempInternal();
    result.pressure = this->sensorsProvider->readPressure();
    result.temperatureExternal = this->sensorsProvider->readTempExternal();
    result.co2ppm = this->sensorsProvider->readCo2();
}

void MainScreen::loop() {
    bool sensorsUpdated = false;

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

    if (millis() - lupdBrightness >= 100) {
        lastBrightnessPercent = readBrightness();
        analogWrite(PIN_LCD_BRIGHTNESS, round(lastBrightnessPercent * 2.55));
        lupdBrightness = millis();
    }

    if (modeChanged) {
        modeChanged = false;
    }
    isFirstLaunch = false;
    delay(50);
}

void MainScreen::onButtonClicked(uint8_t btn) {
    mode++;
    mode = mode % MODES_COUNT;
    modeChanged = true;
}

void MainScreen::updateTime(uint8_t tickTime) {
    if (mode == MODE_TIME) {
        RtcDateTime now = rtc->GetDateTime();
        
        uint8_t hours = now.Hour();
        uint8_t minutes = now.Minute();
        
        printLargeTime(*lcd, hours, minutes, tickTime);
        lcd->setCursor(0, 3);

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

        lcd->print(dayOfWeekName);
        lcd->print(day);
        lcd->print(F("."));
        lcd->print(month);
        lcd->print(F("     "));
    }
}

uint8_t MainScreen::readBrightness() {
    uint16_t brValue = analogRead(PIN_PHOTO_BRIGHTNESS);
    uint8_t brightnessPercent = (uint8_t) round(((float) brValue) / 10.24);
    // DEBUG_SERIAL.print("read brightness: ");
    // DEBUG_SERIAL.println(brightnessPercent);
    return brightnessPercent;
}

void MainScreen::updateMainInfo(Sensors& sensors) {
    if (mode == MODE_TEMP_INT) {
        printLargeFloat(*lcd, sensors.temperatureInternal);
        lcd->setCursor(0, 3);
        lcd->print(F("temp. b/i  "));
    } else if (mode == MODE_HUMIDITY) {
        printLargeFloat(*lcd, sensors.humidity);
        lcd->setCursor(0, 3);
        lcd->print(F("humidity   "));
    } else if (mode == MODE_PRESSURE) {
        printLargeInt(*lcd, sensors.pressure);
        lcd->setCursor(0, 3);
        lcd->print(F("pressure   "));
    } else if (mode == MODE_TEMP_EXT) {
        if (sensors.temperatureExternal != TEMP_EXTERNAL_NONE) {
            printLargeFloat(*lcd, sensors.temperatureExternal);
        } else {
            printLargeFloat(*lcd, 0.0);
        }
        lcd->setCursor(0, 3);
        lcd->print(F("temp. ext. "));
    } else if (mode == MODE_CO2) {
        printLargeInt(*lcd, sensors.co2ppm);
        lcd->setCursor(0, 3);
        lcd->print(F("co2 ppm    "));
    }
}

void MainScreen::updateSideInfo(Sensors& sensors) {
    lcd->setCursor(17, 0);
    lcd->print((int) round(sensors.temperatureInternal));
    lcd->write(0xdf);

    lcd->setCursor(17, 1);
    if (sensors.humidity != HUMID_NONE) {
        lcd->print((int) round(sensors.humidity));
        lcd->print('%');
    } else {
        lcd->print(F("   "));
    }
  
    lcd->setCursor(17, 2);
    lcd->print(sensors.pressure);

    lcd->setCursor(17, 3);
    if (sensors.temperatureExternal != TEMP_EXTERNAL_NONE) {
        lcd->print((int) round(sensors.temperatureExternal));
        lcd->write(0xdf);
    } else if (sensors.co2ppm != CO2_NONE) {
        uint8_t thousands = sensors.co2ppm / 1000;
        uint8_t hundreds = (int)(sensors.co2ppm % 1000) / 100;
        lcd->print(thousands);
        lcd->print('.');
        lcd->print(hundreds);
    } else {
        lcd->print(F("   "));
    }
}

uint8_t MainScreen::calculateQuality(Sensors& sensors) {
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

    // DEBUG_SERIAL.print("Humidity quality: ");
    // DEBUG_SERIAL.println(humWarnState);
    // DEBUG_SERIAL.print("Pressure quality: ");
    // DEBUG_SERIAL.println(presWarnState);
    // DEBUG_SERIAL.print("CO2 quality: ");
    // DEBUG_SERIAL.println(co2WarnState);

    return (uint8_t) ((presWarnState + humWarnState + co2WarnState) / 3);
}

void MainScreen::updateQuality() {
    uint8_t greenValue = 255 * quality / 100;
    uint8_t redValue = 255 * (100 - quality) / 100;
    // DEBUG_SERIAL.print("Green: ");
    // DEBUG_SERIAL.println(greenValue);
    // DEBUG_SERIAL.print("Red: ");
    // DEBUG_SERIAL.println(redValue);
    analogWrite(PIN_INDICATOR_GREEN, greenValue);
    analogWrite(PIN_INDICATOR_RED, redValue);
}

uint8_t MainScreen::getWarningRank(uint16_t warnMin, uint16_t urgentMin, uint16_t warnMax, uint16_t urgentMax, uint16_t value) {
    if (value <= urgentMin || value >= urgentMax) return 100;
    if (value > warnMin && value < warnMax) return 0;

    if (value <= warnMin) {
        return getWarningRank(warnMin, urgentMin, value);
    } else {
        return getWarningRank(warnMax, urgentMax, value);
    }
}

uint8_t MainScreen::getWarningRank(uint16_t warn, uint16_t urgent, uint16_t value) {
    if (value >= urgent) return 100;
    if (value <= warn) return 0;

    float f = ((float) (value - warn)) / (urgent - warn);
    return (uint8_t) (100 * f);
}
