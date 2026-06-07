#include "WifiHandler.h"

WifiHandler::WifiHandler() {
    LOG(INFO, _LogName, "Istanza creata");
}

WifiHandler::~WifiHandler() {
    LOG(INFO, _LogName, "Istanza eliminata");
}

void WifiHandler::SetClockTime(unsigned long ClockTime) {
    _ClockTime = ClockTime;
    LOG(INFO, _LogName, "ClockTime set to " + String (_ClockTime) + " ms");
}

void WifiHandler::Enable() {
    _Enabled = true;
    LOG(INFO, _LogName, "Abilitato");
}

void WifiHandler::Disable() {
    _Enabled = false;
    LOG(INFO, _LogName, "Disabilitato");
}

void WifiHandler::SetHostname(const String& Hostname) {
    _WifiHostname = Hostname;
    LOG(INFO, _LogName, "Hostname: " + _WifiHostname);
}

void WifiHandler::SetSSIDAndPassword(const String& Ssid, const String& Password) {
    _WifiSSID     = Ssid;
    _WifiPassword = Password;
    LOG(INFO, _LogName, "SSID: " + _WifiSSID);
}

void WifiHandler::SetEncryptionKey(const byte* Key) {
    memcpy(_EncryptionKey, Key, sizeof(_EncryptionKey));
    LOG(INFO, _LogName, "Encryption key aggiornata");
}

void WifiHandler::SetEncryptionIV(const byte* IV) {
    memcpy(_EncryptionIV, IV, sizeof(_EncryptionIV));
    LOG(INFO, _LogName, "Encryption IV aggiornato");
}

void WifiHandler::SetOnConnectedCallback(ConnectionCallback Callback) {
    _OnConnectedCallback = Callback;
    LOG(INFO, _LogName, "Connected callback impostata");
}

void WifiHandler::SetOnDisconnectedCallback(DisconnectionCallback Callback) {
    _OnDisconnectedCallback = Callback;
    LOG(INFO, _LogName, "Disconnected callback impostata");
}

bool WifiHandler::IsConnected() {
    return _WifiConnected;
}

int WifiHandler::GetSignalStrength() {
    return 100 * (120 + WiFi.RSSI()) / 120;
}

String WifiHandler::GetIPAddress() {
    return WiFi.localIP().toString();
}

void WifiHandler::Loop() {

    static unsigned long Timer = ZERO_TIME;
    bool Timeout;
    int WifiStatus;

    if (Timer > _ClockTime) {
        Timer = Timer - _ClockTime;
    } else {
        Timer = ZERO_TIME;
    }
    Timeout = (Timer == ZERO_TIME);

    WifiStatus = WiFi.status();

    switch (_State){
        case NOT_CONNECTED:
            if (_Enabled) {
                WiFi.setHostname(_WifiHostname.c_str());
                WiFi.setSleep(WIFI_PS_NONE);
                WiFi.useStaticBuffers(true);
                WiFi.mode(WIFI_STA);
                WiFi.begin(_WifiSSID.c_str(), _WifiPassword.c_str());
                LOG(INFO, _LogName, "Connessione a " + _WifiSSID + " in corso...");
                Timer = _ConnectionMaxTime;
                _State = CONNECTION_IN_PROGRESS;
            }
            break;

        case CONNECTION_IN_PROGRESS:
            if (WifiStatus == WL_CONNECTED) {
                LOG(INFO, _LogName, "Connesso a " + _WifiSSID +
                    " | RSSI: " + String(WiFi.RSSI()) +
                    " | IP: " + GetIPAddress());
                Timer = _PostConnectionDelay;
                _State = POST_CONNECTION_DELAY;
            } else if (!_Enabled) {
                LOG(INFO, _LogName, "Disconnessione in corso");
                WiFi.disconnect();
                Timer = _DisconnectionMaxTime;
                _State = DISCONNECTION_IN_PROGRESS;
            } else if (Timeout) {
                LOG(WARNING, _LogName, "Timeout connessione");
                WiFi.disconnect();
                Timer = _DisconnectionMaxTime;
                _State = DISCONNECTION_IN_PROGRESS;
            }
            break;

        case POST_CONNECTION_DELAY:
            if (!_Enabled) {
                LOG(INFO, _LogName, "Disconnessione in corso");
                WiFi.disconnect();
                Timer = _DisconnectionMaxTime;
                _State = DISCONNECTION_IN_PROGRESS;
            } else if (WifiStatus != WL_CONNECTED) {
                LOG(WARNING, _LogName, "Connessione persa durante post-connection delay");
                WiFi.disconnect();
                Timer = _DisconnectionMaxTime;
                _State = DISCONNECTION_IN_PROGRESS;
            } else if (Timeout) {
                if (_OnConnectedCallback)
                    _OnConnectedCallback();
                _State = CONNECTED;
            }
            break;

        case CONNECTED:
            if (!_Enabled) {
                LOG(INFO, _LogName, "Disconnessione in corso");
                WiFi.disconnect();
                Timer = _DisconnectionMaxTime;
                _State = DISCONNECTION_IN_PROGRESS;
            } else if (WifiStatus != WL_CONNECTED) {
                LOG(WARNING, _LogName, "Connessione persa | WiFi status: " + String(WifiStatus));
                if (_OnDisconnectedCallback)
                    _OnDisconnectedCallback();
                WiFi.disconnect();
                Timer = _DisconnectionMaxTime;
                _State = DISCONNECTION_IN_PROGRESS;
            }
            break;

        case DISCONNECTION_IN_PROGRESS:
            if (WifiStatus != WL_CONNECTED) {
                LOG(INFO, _LogName, "Disconnesso");
                _State = NOT_CONNECTED;
            } else if (Timeout) {
                LOG(WARNING, _LogName, "Timeout disconnessione");
                _State = NOT_CONNECTED;
            }
            break;
    }

    _WifiConnected = (_State == CONNECTED);
}