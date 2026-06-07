#include "DS3231_RtcHandler.h"

DS3231_RtcHandler& DS3231_RtcHandler::GetInstance () {
    static DS3231_RtcHandler Instance;
    return Instance;
}
DS3231_RtcHandler& Rtc = DS3231_RtcHandler::GetInstance();

DS3231_RtcHandler::DS3231_RtcHandler () {
    _Mutex = xSemaphoreCreateMutex();
    if (!_Mutex) {
        LOG(ERROR, _LogName, "Mutex creation failed");
        return;
    }

    Wire.begin();
    if (!_Rtc.begin()) {
        LOG(ERROR, _LogName, "DS3231 not found");
    } else {
        LOG(INFO, _LogName, "RTC initialized");
        if (_Rtc.lostPower()) {
            LOG(WARNING, _LogName, "RTC lost power, setting to compile time");
            _Rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        }
    }
}

// --- Configurazione ---

void DS3231_RtcHandler::SetDateTime (uint16_t Year, uint8_t Month, uint8_t Day, uint8_t Hour, uint8_t Minute, uint8_t Second) {
    if (!_Enabled || !_Mutex) return;
    DateTime Dt(Year, Month, Day, Hour, Minute, Second);
    _Rtc.adjust(Dt);
    LOG(INFO, _LogName, "RTC time set to " + GetFormattedTime());
}

// --- Controllo runtime ---

void DS3231_RtcHandler::Enable () {
    if (!_Mutex) {
        LOG(ERROR, _LogName, "Cannot enable: RTC not initialized");
        return;
    }
    _Enabled = true;
    LOG(INFO, _LogName, "Enabled");
}

void DS3231_RtcHandler::Disable () {
    _Enabled = false;
    LOG(INFO, _LogName, "Disabled");
}

bool DS3231_RtcHandler::IsEnabled () const {
    return _Enabled;
}

// --- Metodi principali ---

DateTime DS3231_RtcHandler::GetDateTime () {
    if (!_Enabled || !_Mutex) return DateTime(static_cast<uint32_t>(0));

    xSemaphoreTake(_Mutex, portMAX_DELAY);
    DateTime Now = _Rtc.now();
    xSemaphoreGive(_Mutex);

    return Now;
}

String DS3231_RtcHandler::GetFormattedTime (const String& Format) {
    if (!_Enabled || !_Mutex) return "RTC Disabled";

    xSemaphoreTake(_Mutex, portMAX_DELAY);

    DateTime Now = _Rtc.now();
    char Buffer[64];

    struct tm TimeInfo;
    TimeInfo.tm_year = Now.year() - 1900;
    TimeInfo.tm_mon  = Now.month() - 1;
    TimeInfo.tm_mday = Now.day();
    TimeInfo.tm_hour = Now.hour();
    TimeInfo.tm_min  = Now.minute();
    TimeInfo.tm_sec  = Now.second();

    strftime(Buffer, sizeof(Buffer), Format.c_str(), &TimeInfo);

    xSemaphoreGive(_Mutex);

    return String(Buffer);
}
