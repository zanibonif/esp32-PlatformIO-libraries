#pragma once
#include <WiFi.h>
#include <LoggerHandler.h>
#include <System.h>

typedef void (*ConnectionCallback)();
typedef void (*DisconnectionCallback)();

class WifiHandler {
public:
    WifiHandler();
    ~WifiHandler();

    // Configurazione - solo in fase di setup
    void SetClockTime(unsigned long ClockTime);
    void SetHostname(const String& Hostname);
    void SetSSIDAndPassword(const String& Ssid, const String& Password);
    void SetEncryptionKey(const byte* Key);
    void SetEncryptionIV(const byte* IV);
    void SetOnConnectedCallback(ConnectionCallback Callback);
    void SetOnDisconnectedCallback(DisconnectionCallback Callback);

    // Controllo runtime
    void Enable();
    void Disable();

    // Diagnostica
    bool   IsConnected();
    int    GetSignalStrength();
    String GetIPAddress();

    // Chiamato ciclicamente
    void Loop();

private:

    enum WifiStateEnum {
        NOT_CONNECTED,
        CONNECTION_IN_PROGRESS,
        POST_CONNECTION_DELAY,
        CONNECTED,
        DISCONNECTION_IN_PROGRESS,
    };

    String _LogName                  = "WifiHandler";
    bool   _WifiConnected            = false;
    bool   _Enabled                  = false;
    String _WifiHostname             = "UndefinedHostname";
    String _WifiSSID                 = "";
    String _WifiPassword             = "";

    unsigned long _ClockTime            = 100;   // milliseconds
    unsigned long _PostConnectionDelay  = 5000;  // milliseconds
    unsigned long _ConnectionMaxTime    = 20000; // milliseconds
    unsigned long _DisconnectionMaxTime = 20000; // milliseconds

    WifiStateEnum _State             = NOT_CONNECTED;

    ConnectionCallback    _OnConnectedCallback    = nullptr;
    DisconnectionCallback _OnDisconnectedCallback = nullptr;

    byte _EncryptionKey[16] = {0x00, 0x01, 0x02, 0x23, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x1C, 0x0D, 0x0E, 0x0F};
    byte _EncryptionIV[16]  = {0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x19, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00};

};