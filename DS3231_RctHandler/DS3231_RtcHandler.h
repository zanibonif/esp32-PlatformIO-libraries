#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include "LoggerHandler.h"

class DS3231_RtcHandler {
public:
    static DS3231_RtcHandler* GetInstance();
    static void Destroy();

    void Enable();
    void Disable();
    bool IsEnabled() const;

    void SetDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);
    DateTime GetDateTime();
    String GetFormattedTime(const String& format = "%d/%m/%Y %H:%M:%S");

private:
    DS3231_RtcHandler();
    ~DS3231_RtcHandler() = default;

    static DS3231_RtcHandler* StaticInstance;

    RTC_DS3231 Rtc;
    bool Enabled = false;
    const String LogName = "RTC";
};
