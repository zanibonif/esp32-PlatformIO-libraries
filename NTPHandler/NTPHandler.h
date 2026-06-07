#pragma once

#include <NTPClient.h>
#include <WiFiUdp.h>
#include <System.h>
#include "DateTimeProvider.h"

typedef void (*TimeSyncCallback)();

class NtpHandler : public DateTimeProvider {
public:
    static NtpHandler& GetInstance();

    NtpHandler(const NtpHandler&)            = delete;
    NtpHandler& operator=(const NtpHandler&) = delete;

    // Configurazione - solo in fase di setup
    void SetClockTime(unsigned long ClockTime);
    void SetGmtOffset(int GmtOffsetHours);
    void SetUpdateInterval(unsigned long IntervalMs);
    void SetConnectionMaxTime(unsigned long TimeoutMs);
    void SetOnSyncCallback(TimeSyncCallback Callback);
    void SetOnDesyncCallback(TimeSyncCallback Callback);

    // Controllo runtime
    void Enable();
    void Disable();

    // Diagnostica
    bool   IsConnected();
    String GetFormattedTime(const String& Format = "%H:%M:%S") override;
    unsigned long GetEpochTime();

    // Chiamato ciclicamente
    void Loop();

private:
    NtpHandler();

    enum NtpStateEnum {
        NOT_CONNECTED,
        CONNECTION_IN_PROGRESS,
        CONNECTED
    };

    String           _LogName             = "NtpHandler";
    WiFiUDP          _Udp;
    NTPClient        _NtpClient;
    bool             _Enabled             = false;
    bool             _Connected           = false;
    unsigned long    _ClockTime           = 100;   // milliseconds
    unsigned long    _ConnectionMaxTime   = 10000; // milliseconds
    unsigned long    _UpdateInterval      = 60000; // milliseconds
    unsigned long    _UpdateTimeout       = 5000;  // milliseconds
    NtpStateEnum     _State               = NOT_CONNECTED;

    TimeSyncCallback _OnSyncCallback      = nullptr;
    TimeSyncCallback _OnDesyncCallback    = nullptr;

};