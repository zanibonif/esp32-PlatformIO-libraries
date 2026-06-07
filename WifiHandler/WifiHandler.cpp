#include "WifiHandler.h"

WifiHandler& WifiHandler::GetInstance () {
    static WifiHandler Instance;
    return Instance;
}
WifiHandler& Wifi = WifiHandler::GetInstance();

WifiHandler::WifiHandler () {
    LOG(INFO, _LogName, "Istanza creata");
}

// --- Configurazione ---

void WifiHandler::SetClockTime (unsigned long ClockTime) {
    _ClockTime = ClockTime;
    LOG(INFO, _LogName, "ClockTime set to " + String(_ClockTime) + " ms");
}

void WifiHandler::SetHostname (const String& Hostname) {
    _Hostname = Hostname;
    LOG(INFO, _LogName, "Hostname: " + _Hostname);
}

void WifiHandler::SetSSIDAndPassword (const String& Ssid, const String& Password) {
    _SSID     = Ssid;
    _Password = Password;
    LOG(INFO, _LogName, "SSID: " + _SSID);
}

void WifiHandler::SetPostConnectionDelay (unsigned long DelayMs) {
    _PostConnectionDelay = DelayMs;
    LOG(INFO, _LogName, "PostConnectionDelay set to " + String(_PostConnectionDelay) + " ms");
}

void WifiHandler::SetConnectionMaxTime (unsigned long TimeoutMs) {
    _ConnectionMaxTime = TimeoutMs;
    LOG(INFO, _LogName, "ConnectionMaxTime set to " + String(_ConnectionMaxTime) + " ms");
}

void WifiHandler::SetDisconnectionMaxTime (unsigned long TimeoutMs) {
    _DisconnectionMaxTime = TimeoutMs;
    LOG(INFO, _LogName, "DisconnectionMaxTime set to " + String(_DisconnectionMaxTime) + " ms");
}

void WifiHandler::SetOnConnectedCallback (ConnectionCallback Callback) {
    _OnConnectedCallback = Callback;
    LOG(INFO, _LogName, "Connected callback impostata");
}

void WifiHandler::SetOnDisconnectedCallback (DisconnectionCallback Callback) {
    _OnDisconnectedCallback = Callback;
    LOG(INFO, _LogName, "Disconnected callback impostata");
}

// --- Controllo runtime ---

void WifiHandler::Enable () {
    _Enabled = true;
    LOG(INFO, _LogName, "Abilitato");
}

void WifiHandler::Disable () {
    _Enabled = false;
    LOG(INFO, _LogName, "Disabilitato");
}

// --- Diagnostica ---

bool WifiHandler::IsConnected () {
    return (_State == CONNECTED);
}

int WifiHandler::GetSignalStrength () {
    return max(0, min(100, 100 * (120 + WiFi.RSSI()) / 120));
}

String WifiHandler::GetIPAddress () {
    return WiFi.localIP().toString();
}

// --- Loop ---

void WifiHandler::Loop () {

    static unsigned long Timer = ZERO_TIME;
    bool Timeout;
    int  WifiStatus;

    if (Timer > _ClockTime) {
        Timer = Timer - _ClockTime;
    } else {
        Timer = ZERO_TIME;
    }
    Timeout = (Timer == ZERO_TIME);

    WifiStatus = WiFi.status();

    switch (_State) {
        case NOT_CONNECTED:
            if (_Enabled) {
                WiFi.setHostname(_Hostname.c_str());
                WiFi.setSleep(WIFI_PS_NONE);
                WiFi.useStaticBuffers(true);
                WiFi.mode(WIFI_STA);
                WiFi.begin(_SSID.c_str(), _Password.c_str());
                LOG(INFO, _LogName, "Connessione a " + _SSID + " in corso...");
                Timer = _ConnectionMaxTime;
                _State = CONNECTION_IN_PROGRESS;
            }
            break;

        case CONNECTION_IN_PROGRESS:
            if (WifiStatus == WL_CONNECTED) {
                LOG(INFO, _LogName, "Connesso a " + _SSID +
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
                if (_OnConnectedCallback) _OnConnectedCallback();
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
                if (_OnDisconnectedCallback) _OnDisconnectedCallback();
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

}
