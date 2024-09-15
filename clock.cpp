#include "clock.h"

#define PIN_RTC_DAT 4
#define PIN_RTC_CLK 5
#define PIN_RTC_RST 6

Clock::Clock() {
    ThreeWire myWire(PIN_RTC_DAT, PIN_RTC_CLK, PIN_RTC_RST);

    this->rtc = new RtcDS1302<ThreeWire>(myWire);
    this->rtc->Begin();

    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

    if (!this->rtc->IsDateTimeValid()) {
        this->rtc->SetDateTime(compiled);
    }

    if (this->rtc->GetIsWriteProtected()) {
        this->rtc->SetIsWriteProtected(false);
    }

    if (!this->rtc->GetIsRunning()) {
        this->rtc->SetIsRunning(true);
    }

    RtcDateTime now = this->rtc->GetDateTime();
    if (now < compiled) {
        this->rtc->SetDateTime(compiled);
    }
}

uint32_t Clock::getDateTime() {
    RtcDateTime now = this->rtc->GetDateTime();
        
    uint8_t hours = now.Hour();
    uint8_t minutes = now.Minute();
    uint16_t time = (hours << 8) | (minutes);

    uint8_t day = now.Day();
    uint8_t month = now.Month();
    uint8_t dayOfWeek = now.DayOfWeek();
    uint16_t date = ((day & 0x20) << 7) | ((month & 0x10) << 3) | (dayOfWeek & 0x07);

    return date << 16 | time;
}