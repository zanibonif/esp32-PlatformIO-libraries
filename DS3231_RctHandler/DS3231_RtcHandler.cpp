#include "DS3231_RtcHandler.h"

DS3231_RtcHandler* DS3231_RtcHandler::StaticInstance = nullptr;

DS3231_RtcHandler::DS3231_RtcHandler() {
    Wire.begin();
    if (!Rtc.begin()) {
        LOG(ERROR, LogName, "DS3231 not found");
    } else {
        LOG(INFO, LogName, "RTC initialized");
        if (Rtc.lostPower()) {
            LOG(WARNING, LogName, "RTC lost power, setting to compile time");
            Rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        }
    }
}

DS3231_RtcHandler* DS3231_RtcHandler::GetInstance() {
    if (StaticInstance == nullptr) {
        StaticInstance = new DS3231_RtcHandler();
    }
    return StaticInstance;
}

void DS3231_RtcHandler::Destroy() {
    if (StaticInstance) {
        delete StaticInstance;
        StaticInstance = nullptr;
    }
}

void DS3231_RtcHandler::Enable() {
    Enabled = true;
    LOG(INFO, LogName, "Enabled");
}

void DS3231_RtcHandler::Disable() {
    Enabled = false;
    LOG(INFO, LogName, "Disabled");
}

bool DS3231_RtcHandler::IsEnabled() const {
    return Enabled;
}

void DS3231_RtcHandler::SetDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) {
    if (!Enabled) return;
    DateTime dt(year, month, day, hour, minute, second);
    Rtc.adjust(dt);
    LOG(INFO, LogName, "RTC time set to " + GetFormattedTime());
}

DateTime DS3231_RtcHandler::GetDateTime() {
    if (!Enabled) return DateTime(static_cast<uint32_t>(0));
    return Rtc.now();
}

String DS3231_RtcHandler::GetFormattedTime(const String& format) {
    if (!Enabled) return "RTC Disabled";

    DateTime now = Rtc.now();
    char buffer[64];

    struct tm timeinfo;
    timeinfo.tm_year = now.year() - 1900;
    timeinfo.tm_mon  = now.month() - 1;
    timeinfo.tm_mday = now.day();
    timeinfo.tm_hour = now.hour();
    timeinfo.tm_min  = now.minute();
    timeinfo.tm_sec  = now.second();

    strftime(buffer, sizeof(buffer), format.c_str(), &timeinfo);
    return String(buffer);
}
