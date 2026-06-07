#pragma once
#include <WiFi.h>
#include <LoggerHandler.h>
#include <System.h>

typedef void (*ConnectionCallback)();
typedef void (*DisconnectionCallback)();

class WifiHandler {
public:
    static WifiHandler& GetInstance ();
    WifiHandler (const WifiHandler&)            = delete;
    WifiHandler& operator= (const WifiHandler&) = delete;

    // Configurazione
    void SetClockTime (unsigned long ClockTime);
    void SetHostname (const String& Hostname);
    void SetSSIDAndPassword (const String& Ssid, const String& Password);
    void SetPostConnectionDelay (unsigned long DelayMs);
    void SetConnectionMaxTime (unsigned long TimeoutMs);
    void SetDisconnectionMaxTime (unsigned long TimeoutMs);
    void SetOnConnectedCallback (ConnectionCallback Callback);
    void SetOnDisconnectedCallback (DisconnectionCallback Callback);

    // Controllo runtime
    void Enable ();
    void Disable ();

    // Diagnostica
    bool   IsConnected ();
    int    GetSignalStrength ();
    String GetIPAddress ();

    // Chiamato ciclicamente
    void Loop ();

private:
    WifiHandler ();

    enum WifiState {
        NOT_CONNECTED,
        CONNECTION_IN_PROGRESS,
        POST_CONNECTION_DELAY,
        CONNECTED,
        DISCONNECTION_IN_PROGRESS
    };

    String                _LogName                = "WifiHandler";
    bool                  _Enabled                = false;
    String                _Hostname               = "UndefinedHostname";
    String                _SSID                   = "";
    String                _Password               = "";

    unsigned long         _ClockTime              = 100;   // milliseconds
    unsigned long         _PostConnectionDelay    = 5000;  // milliseconds
    unsigned long         _ConnectionMaxTime      = 20000; // milliseconds
    unsigned long         _DisconnectionMaxTime   = 20000; // milliseconds

    WifiState             _State                  = NOT_CONNECTED;

    ConnectionCallback    _OnConnectedCallback    = nullptr;
    DisconnectionCallback _OnDisconnectedCallback = nullptr;
};

extern WifiHandler& Wifi;
