#pragma once

#include <NTPClient.h>
#include <WiFiUdp.h>
#include <System.h>
#include "DateTimeProvider.h"

// Server di riferimento: uno solo, il pool pubblico. Si sovrascrive con un -D se
// un impianto ha un server interno.
#ifndef NTP_SERVER_NAME
#define NTP_SERVER_NAME "pool.ntp.org"
#endif

typedef void (*TimeSyncCallback)();

class NtpHandler : public DateTimeProvider {
public:
    static NtpHandler& GetInstance ();

    NtpHandler (const NtpHandler&)            = delete;
    NtpHandler& operator= (const NtpHandler&) = delete;

    // Configurazione - solo in fase di setup
    void SetClockTime (unsigned long ClockTime);
    void SetGmtOffset (int GmtOffsetHours);
    void SetUpdateInterval (unsigned long IntervalMs);
    void SetConnectionMaxTime (unsigned long TimeoutMs);
    void SetOnSyncCallback (TimeSyncCallback Callback);
    void SetOnDesyncCallback (TimeSyncCallback Callback);

    // Controllo runtime
    void Enable ();
    void Disable ();

    // Diagnostica
    bool          IsConnected ();
    bool          IsEnabled ();
    const char*   GetStateText ();          // stato della macchina, in chiaro
    const char*   GetServerName ();
    int           GetGmtOffset ();          // ore
    unsigned long GetUpdateInterval ();     // millisecondi
    // Tempo trascorso dall'ultima sincronizzazione andata a buon fine, in
    // millisecondi. Vale zero se non e' mai stata fatta: va letto assieme a
    // IsConnected(), che dice se quel valore ha un senso.
    unsigned long GetTimeSinceLastSync ();
    // False fino alla prima sincronizzazione riuscita: distingue "non ho ancora
    // mai preso l'ora" da "l'ho presa e poi l'ho persa", che in diagnostica sono
    // due guasti diversi.
    bool          HasSynchronized ();
    String        GetFormattedTime (const String& Format = "%H:%M:%S") override;
    unsigned long GetEpochTime () override;

    // Chiamato ciclicamente
    void Loop ();

private:
    NtpHandler ();

    enum NtpState {
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
    NtpState         _State               = NOT_CONNECTED;
    int              _GmtOffsetHours      = 0;
    bool             _Synchronized        = false;
    unsigned long    _TimeSinceLastSync   = ZERO_TIME; // millisecondi, cresce di _ClockTime per ciclo

    TimeSyncCallback _OnSyncCallback      = nullptr;
    TimeSyncCallback _OnDesyncCallback    = nullptr;

};

extern NtpHandler& Ntp;
