#include "DS3231_RtcHandler.h"

DS3231_RtcHandler& DS3231_RtcHandler::GetInstance () {
    static DS3231_RtcHandler Instance;
    return Instance;
}
DS3231_RtcHandler& Rtc = DS3231_RtcHandler::GetInstance();

DS3231_RtcHandler::DS3231_RtcHandler () {
    // Alias I2cBus non ancora affidabile durante l'init statico: usare GetInstance()
    I2cBusHandler::GetInstance().AddDevice(DS3231_RTC_I2C_ADDRESS, "DS3231");
    LOG(INFO, _LogName, "Instance created");
}

// --- Configurazione ---

void DS3231_RtcHandler::SetDateTime (uint16_t Year, uint8_t Month, uint8_t Day, uint8_t Hour, uint8_t Minute, uint8_t Second) {
    if (!_Enabled || !_Initialized || !IsAvailable()) {
        LOG(WARNING, _LogName, "SetDateTime ignored: RTC not available");
        return;
    }

    DateTime Dt(Year, Month, Day, Hour, Minute, Second);

    if (!I2cBus.TakeBus()) {
        LOG(ERROR, _LogName, "SetDateTime failed: I2C bus busy");
        return;
    }
    _Rtc.adjust(Dt);
    I2cBus.GiveBus();

    char Buffer[32];
    snprintf(Buffer, sizeof(Buffer), "%02u/%02u/%04u %02u:%02u:%02u", Day, Month, Year, Hour, Minute, Second);
    LOG(INFO, _LogName, "RTC time set to " + String(Buffer));
}

// --- Controllo runtime ---

void DS3231_RtcHandler::Enable () {
    if (_Enabled) return;

    _Enabled = true;
    LOG(INFO, _LogName, "Enabled");
}

void DS3231_RtcHandler::Disable () {
    if (!_Enabled) return;

    _Enabled = false;
    LOG(INFO, _LogName, "Disabled");
}

bool DS3231_RtcHandler::IsEnabled () const {
    return _Enabled;
}

bool DS3231_RtcHandler::IsAvailable () const {
    return I2cBus.IsAvailable(DS3231_RTC_I2C_ADDRESS);
}

// --- Metodi principali ---

// Non bloccante: legge la cache aggiornata dalla Loop (epoch 0 = nessuna lettura valida)
DateTime DS3231_RtcHandler::GetDateTime () {
    if (!_Enabled) return DateTime(static_cast<uint32_t>(0));
    return DateTime(static_cast<uint32_t>(_CachedEpoch));
}

unsigned long DS3231_RtcHandler::GetEpochTime () {
    return static_cast<unsigned long>(_CachedEpoch);
}

// Non bloccante: formatta la cache aggiornata dalla Loop
String DS3231_RtcHandler::GetFormattedTime (const String& Format) {
    if (!_Enabled) return "RTC Disabled";

    uint32_t Epoch = _CachedEpoch;
    if (Epoch == 0) return "";

    DateTime Now(Epoch);

    char Buffer[64];
    struct tm TimeInfo;
    TimeInfo.tm_year = Now.year() - 1900;
    TimeInfo.tm_mon  = Now.month() - 1;
    TimeInfo.tm_mday = Now.day();
    TimeInfo.tm_hour = Now.hour();
    TimeInfo.tm_min  = Now.minute();
    TimeInfo.tm_sec  = Now.second();

    strftime(Buffer, sizeof(Buffer), Format.c_str(), &TimeInfo);

    return String(Buffer);
}

// --- Chiamato ciclicamente ---

void DS3231_RtcHandler::Loop () {
    if (!_Enabled) return;

    if (!I2cBus.IsAvailable(DS3231_RTC_I2C_ADDRESS)) {
        _CachedEpoch = 0;
        return;
    }

    // Inizializzazione del chip alla prima disponibilità del modulo
    if (!_Initialized) {
        if (!I2cBus.TakeBus()) return;

        bool BeginOk = _Rtc.begin();
        if (BeginOk && _Rtc.lostPower()) {
            LOG(WARNING, _LogName, "RTC lost power, setting to compile time");
            _Rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        }
        I2cBus.GiveBus();

        if (!BeginOk) {
            LOG(ERROR, _LogName, "DS3231 begin failed");
            return;
        }

        _Initialized = true;
        LOG(INFO, _LogName, "RTC initialized");
        return;
    }

    if (!I2cBus.TakeBus()) return;   // bus occupato: la cache resta l'ultima lettura valida
    DateTime Now = _Rtc.now();
    I2cBus.GiveBus();

    _CachedEpoch = Now.isValid() ? Now.unixtime() : 0;
}
