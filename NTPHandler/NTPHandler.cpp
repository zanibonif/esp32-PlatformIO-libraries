#include "NtpHandler.h"
#include "LoggerHandler.h"

NtpHandler& NtpHandler::GetInstance () {
    static NtpHandler Instance;
    return Instance;
}
NtpHandler& Ntp = NtpHandler::GetInstance();

NtpHandler::NtpHandler ()
    : _NtpClient(_Udp, "pool.ntp.org", 0, 60000) {
    LOG(INFO, _LogName, "Istanza creata");
}

void NtpHandler::SetClockTime (unsigned long ClockTime) {
    _ClockTime = ClockTime;
    LOG(INFO, _LogName, "ClockTime set to " + String(_ClockTime) + " ms");
}

void NtpHandler::SetGmtOffset (int GmtOffsetHours) {
    _NtpClient.setTimeOffset(GmtOffsetHours * 3600);
    LOG(INFO, _LogName, "GMT offset impostato a " + String(GmtOffsetHours) + " ore");
}

void NtpHandler::SetUpdateInterval (unsigned long IntervalMs) {
    _UpdateInterval = IntervalMs;
    _NtpClient.setUpdateInterval(_UpdateInterval);
    LOG(INFO, _LogName, "UpdateInterval impostato a " + String(_UpdateInterval) + " ms");
}

void NtpHandler::SetConnectionMaxTime (unsigned long TimeoutMs) {
    _ConnectionMaxTime = TimeoutMs;
    LOG(INFO, _LogName, "ConnectionMaxTime impostato a " + String(_ConnectionMaxTime) + " ms");
}

void NtpHandler::SetOnSyncCallback (TimeSyncCallback Callback) {
    _OnSyncCallback = Callback;
    LOG(INFO, _LogName, "Sync callback impostata");
}

void NtpHandler::SetOnDesyncCallback (TimeSyncCallback Callback) {
    _OnDesyncCallback = Callback;
    LOG(INFO, _LogName, "Desync callback impostata");
}

void NtpHandler::Enable () {
    _Enabled = true;
    LOG(INFO, _LogName, "Abilitato");
}

void NtpHandler::Disable () {
    _Enabled = false;
    LOG(INFO, _LogName, "Disabilitato");
}

bool NtpHandler::IsConnected () {
    return _Connected;
}

String NtpHandler::GetFormattedTime (const String& Format) {
    unsigned long RawTime = _NtpClient.getEpochTime();
    time_t Time = static_cast<time_t>(RawTime);
    struct tm TimeInfo;
    localtime_r(&Time, &TimeInfo);
    char Buffer[64];
    strftime(Buffer, sizeof(Buffer), Format.c_str(), &TimeInfo);
    return String(Buffer);
}

unsigned long NtpHandler::GetEpochTime () {
    return _NtpClient.getEpochTime();
}

void NtpHandler::Loop () {

    static unsigned long Timer = ZERO_TIME;
    bool Timeout;

    if (Timer > _ClockTime) {
        Timer = Timer - _ClockTime;
    } else {
        Timer = ZERO_TIME;
    }
    Timeout = (Timer == ZERO_TIME);

    switch (_State) {
        case NOT_CONNECTED:
            if (_Enabled) {
                _NtpClient.begin();
                _NtpClient.setUpdateInterval(_UpdateInterval);
                LOG(INFO, _LogName, "Sincronizzazione tempo in corso...");
                Timer = _ConnectionMaxTime;
                _State = CONNECTION_IN_PROGRESS;
            }
            break;

        case CONNECTION_IN_PROGRESS:
            if (!_Enabled) {
                LOG(INFO, _LogName, "Sincronizzazione interrotta");
                _NtpClient.end();
                _State = NOT_CONNECTED;
            } else if (_NtpClient.forceUpdate()) {
                LOG(INFO, _LogName, "Tempo sincronizzato: " + GetFormattedTime("%d/%m/%Y %H:%M:%S"));
                LOG(INFO, _LogName, "Sincronizzazione automatica ogni " + String(_UpdateInterval) + " millisecondi abilitata");
                if (_OnSyncCallback) _OnSyncCallback();
                Timer = _UpdateInterval + _UpdateTimeout;
                _State = CONNECTED;
            } else if (Timeout) {
                LOG(WARNING, _LogName, "Timeout sincronizzazione");
                _NtpClient.end();
                _State = NOT_CONNECTED;
            }
            break;

        case CONNECTED:
            if (!_Enabled) {
                if (_OnDesyncCallback) _OnDesyncCallback();
                _NtpClient.end();
                LOG(INFO, _LogName, "Sincronizzazione fermata");
                _State = NOT_CONNECTED;
            } else if (_NtpClient.update()) {
                Timer = _UpdateInterval + _UpdateTimeout;
            } else if (Timeout) {
                LOG(WARNING, _LogName, "Timeout aggiornamento NTP - sincronizzazione persa");
                if (_OnDesyncCallback) _OnDesyncCallback();
                _NtpClient.end();
                _State = NOT_CONNECTED;
            }
            break;
    }

    _Connected = (_State == CONNECTED);
}
