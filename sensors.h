#include "Arduino.h"

// dht11
#include <DHT_U.h>

// bmp280
#include <Adafruit_BMP280.h>

// d18b20
#include <OneWire.h>
#include <DallasTemperature.h>

// mh-z19b
#include "MHZ19_uart.h"

// pins
#define PIN_DHT 2
#define PIN_MHZ_TX 11
#define PIN_MHZ_RX 12
#define PIN_D18B20 13

// sensors configs
#define DHTTYPE DHT11

#define CO2_NONE -1
#define TEMP_EXTERNAL_NONE 666.13
#define HUMID_NONE 0

class SensorsProvider {

private:
    DHT_Unified *dht;
    Adafruit_BMP280 *bmp;
    DallasTemperature *dallasTemp;
    MHZ19_uart *mhz19;

public:
    SensorsProvider();
    float readHumidity();
    float readTempInternal();
    uint16_t readPressure();
    float readTempExternal();
    uint16_t readCo2();
};