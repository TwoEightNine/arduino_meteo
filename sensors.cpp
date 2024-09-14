#include "Arduino.h"
#include "sensors.h"


SensorsProvider::SensorsProvider() {
    dht = new DHT_Unified(PIN_DHT, DHTTYPE);
    dht->begin();

    bmp = new Adafruit_BMP280();
    bmp->begin(0x76);
    bmp->setSampling(
        Adafruit_BMP280::MODE_NORMAL, // Режим работы
        Adafruit_BMP280::SAMPLING_X2, // Точность изм. температуры
        Adafruit_BMP280::SAMPLING_X16, // Точность изм. давления
        Adafruit_BMP280::FILTER_X16, // Уровень фильтрации
        Adafruit_BMP280::STANDBY_MS_500
    );

    OneWire oneWire(PIN_D18B20);
    dallasTemp = new DallasTemperature(&oneWire);
    dallasTemp->begin();

    mhz19 = new MHZ19_uart();
    mhz19->begin(PIN_MHZ_TX, PIN_MHZ_RX);
    mhz19->setAutoCalibration(false);
    mhz19->getStatus();
}

float SensorsProvider::readHumidity() {
    sensors_event_t event;
    dht->humidity().getEvent(&event);
    return event.relative_humidity;
}

uint16_t SensorsProvider::readPressure() {
    return round(bmp->readPressure() / 133.3);
}

float SensorsProvider::readTempInternal() {
    sensors_event_t event;
    dht->temperature().getEvent(&event);
    float temp_dht = event.temperature;
    // return temp_dht;
    float temp_bmp = bmp->readTemperature();

    float temp;
    if (temp_dht != 0 && temp_bmp != 0) {
        temp = (temp_dht + temp_bmp) / 2.0;
    } else if (temp_bmp != 0) {
        temp = temp_bmp;
    } else {
        temp = temp_dht;
    }
    return temp;
}

float SensorsProvider::readTempExternal() {
    uint8_t sensorSignal = digitalRead(PIN_D18B20);
    // DEBUG_SERIAL.print("read d18b20: ");
    // DEBUG_SERIAL.println(sensorSignal);

    if (sensorSignal) {
        dallasTemp->requestTemperatures();
        float c = dallasTemp->getTempCByIndex(0);
        bool isConnected = c != DEVICE_DISCONNECTED_C;

        if (isConnected) {
            return c;
        } else {
            return TEMP_EXTERNAL_NONE;
        }
        // DEBUG_SERIAL.print("temp d18b20: ");
        // DEBUG_SERIAL.println(c);
    } else {
        return TEMP_EXTERNAL_NONE;
    }
}

uint16_t SensorsProvider::readCo2() {
    int ppm = mhz19->getPPM();
    if (ppm != CO2_NONE) {
        // DEBUG_SERIAL.print("read co2: ");
        // DEBUG_SERIAL.println(result.co2ppm);
        return (uint16_t) ppm; 
    } else {
        return CO2_NONE;
    }
}