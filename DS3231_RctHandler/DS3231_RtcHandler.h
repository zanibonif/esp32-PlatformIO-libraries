#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include "LoggerHandler.h"
#include "DateTimeProvider.h"
#include "freertos/semphr.h"

class DS3231_RtcHandler : public DateTimeProvider {
public:
    static DS3231_RtcHandler& GetInstance ();
    DS3231_RtcHandler (const DS3231_RtcHandler&)            = delete;
    DS3231_RtcHandler& operator= (const DS3231_RtcHandler&) = delete;

    // Configurazione
    void SetDateTime (uint16_t Year, uint8_t Month, uint8_t Day, uint8_t Hour, uint8_t Minute, uint8_t Second);

    // Controllo runtime
    void Enable ();
    void Disable ();
    bool IsEnabled () const;

    // Metodi principali
    DateTime GetDateTime ();
    String   GetFormattedTime (const String& Format = "%d/%m/%Y %H:%M:%S") override;

private:
    DS3231_RtcHandler ();

    RTC_DS3231        _Rtc;
    bool              _Enabled = false;
    String            _LogName = "RTC";
    SemaphoreHandle_t _Mutex   = nullptr;
};

extern DS3231_RtcHandler& Rtc;
