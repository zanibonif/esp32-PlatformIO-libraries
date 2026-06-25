#include "WifiHandler.h"

WifiHandler& WifiHandler::GetInstance () {
    static WifiHandler Instance;
    return Instance;
}
WifiHandler& Wifi = WifiHandler::GetInstance();

WifiHandler::WifiHandler () {
    LOG(INFO, _LogName, "Istanza creata");
}

// --- Configurazione STA ---

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

    if (_State != NOT_CONNECTED) {
        LOG(INFO, _LogName, "Nuove credenziali riavvio connessione");
        _RestartConnectionRequest = true;
    }
}

void WifiHandler::SetPostConnectionDelay (unsigned long DelayMs) {
    _PostConnectionDelay = DelayMs;
    LOG(INFO, _LogName, "PostConnectionDelay: " + String(_PostConnectionDelay) + " ms");
}

void WifiHandler::SetConnectionMaxTime (unsigned long TimeoutMs) {
    _ConnectionMaxTime = TimeoutMs;
    LOG(INFO, _LogName, "ConnectionMaxTime: " + String(_ConnectionMaxTime) + " ms");
}

void WifiHandler::SetDisconnectionMaxTime (unsigned long TimeoutMs) {
    _DisconnectionMaxTime = TimeoutMs;
    LOG(INFO, _LogName, "DisconnectionMaxTime: " + String(_DisconnectionMaxTime) + " ms");
}

void WifiHandler::SetOnConnectedCallback (ConnectionCallback Callback) {
    _OnConnectedCallback = Callback;
    LOG(INFO, _LogName, "Connected callback impostata");
}

void WifiHandler::SetOnDisconnectedCallback (DisconnectionCallback Callback) {
    _OnDisconnectedCallback = Callback;
    LOG(INFO, _LogName, "Disconnected callback impostata");
}

// --- Configurazione AP fallback ---

void WifiHandler::SetAPCredentials (const String& SSID, const String& Password) {
    _APSSID     = SSID;
    _APPassword = Password;
    LOG(INFO, _LogName, "AP credentials: " + _APSSID);
}

void WifiHandler::EnableAPFallback () {
    _APFallbackEnabled = true;
    LOG(INFO, _LogName, "AP fallback abilitato");
}

void WifiHandler::DisableAPFallback () {
    _APFallbackEnabled = false;
    LOG(INFO, _LogName, "AP fallback disabilitato");
}

void WifiHandler::SetAPRetryInterval (unsigned long IntervalMs) {
    _APRetryInterval = IntervalMs;
    LOG(INFO, _LogName, "AP retry interval: " + String(_APRetryInterval) + " ms");
}

void WifiHandler::SetOnAPStartedCallback (APCallback Callback) {
    _OnAPStartedCallback = Callback;
    LOG(INFO, _LogName, "AP started callback impostata");
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

bool WifiHandler::IsAPMode () {
    return ((_State == AP_MODE) || (_State == AP_STA_RETRY));
}

int WifiHandler::GetSignalStrength () {
    return max(0, min(100, 100 * (120 + WiFi.RSSI()) / 120));
}

String WifiHandler::GetIPAddress () {
    return WiFi.localIP().toString();
}

String WifiHandler::GetAPIPAddress () {
    return WiFi.softAPIP().toString();
}

// --- Loop ---

void WifiHandler::Loop () {

    static unsigned long Timer = ZERO_TIME;
    bool Timeout;

    if (Timer > _ClockTime) {
        Timer = Timer - _ClockTime;
    } else {
        Timer = ZERO_TIME;
    }
    Timeout    = (Timer == ZERO_TIME);

    switch (_State) {
        case NOT_CONNECTED:
            if (_Enabled) {
                WiFi.setHostname(_Hostname.c_str());
                WiFi.setSleep(WIFI_PS_NONE);
                WiFi.useStaticBuffers(true);
                WiFi.mode(WIFI_STA);
                WiFi.begin(_SSID.c_str(), _Password.c_str());
                LOG(INFO, _LogName, "Connessione a " + _SSID + " in corso...");
                Timer  = _ConnectionMaxTime;
                _State = CONNECTION_IN_PROGRESS;
            }
            _RestartConnectionRequest = false;
            break;

        case CONNECTION_IN_PROGRESS:
            if (WiFi.status() == WL_CONNECTED) {
                LOG(INFO, _LogName, "Connesso a " + _SSID + " | RSSI: " + String(WiFi.RSSI()) + " | IP: "   + GetIPAddress());
                Timer  = _PostConnectionDelay;
                _State = POST_CONNECTION_DELAY;
            } else if (!_Enabled) {
                LOG(INFO, _LogName, "Disconnessione in corso");
                WiFi.disconnect();
                Timer  = _DisconnectionMaxTime;
                _State = DISCONNECTION_IN_PROGRESS;
            } else if (_RestartConnectionRequest) {
                LOG(INFO, _LogName, "Ripartenza connessione");
                WiFi.disconnect();
                Timer  = _DisconnectionMaxTime;
                _State = DISCONNECTION_IN_PROGRESS;
            } else if (Timeout) {
                LOG(WARNING, _LogName, "Timeout connessione a " + _SSID);
                WiFi.disconnect();
                if (_APFallbackEnabled) {
                    WiFi.mode(WIFI_AP);
                    WiFi.softAP(_APSSID.c_str(), _APPassword.isEmpty() ? nullptr : _APPassword.c_str());
                    LOG(INFO, _LogName, "AP mode avviato: " + _APSSID + " | IP: " + GetAPIPAddress());
                    if (_OnAPStartedCallback) _OnAPStartedCallback();
                    Timer  = _APRetryInterval;
                    _State = AP_MODE;
                } else {
                    Timer  = _DisconnectionMaxTime;
                    _State = DISCONNECTION_IN_PROGRESS;
                }
            }
            break;

        case POST_CONNECTION_DELAY:
            if (!_Enabled) {
                LOG(INFO, _LogName, "Disconnessione in corso");
                WiFi.disconnect();
                Timer  = _DisconnectionMaxTime;
                _State = DISCONNECTION_IN_PROGRESS;
            } else if (WiFi.status() != WL_CONNECTED) {
                LOG(WARNING, _LogName, "Connessione persa durante post-connection delay");
                WiFi.disconnect();
                Timer  = _DisconnectionMaxTime;
                _State = DISCONNECTION_IN_PROGRESS;
            } else if (Timeout) {
                if (_OnConnectedCallback) _OnConnectedCallback();
                _State = CONNECTED;
            } else if (_RestartConnectionRequest) {
                LOG(INFO, _LogName, "Ripartenza connessione");
                WiFi.disconnect();
                Timer  = _DisconnectionMaxTime;
                _State = DISCONNECTION_IN_PROGRESS;
            }
            break;

        case CONNECTED:
            if (!_Enabled) {
                LOG(INFO, _LogName, "Disconnessione in corso");
                if (_OnDisconnectedCallback) _OnDisconnectedCallback();
                WiFi.disconnect();
                Timer  = _DisconnectionMaxTime;
                _State = DISCONNECTION_IN_PROGRESS;
            } else if (WiFi.status() != WL_CONNECTED) {
                LOG(WARNING, _LogName, "Connessione persa | WiFi status: " + String(WiFi.status()));
                if (_OnDisconnectedCallback) _OnDisconnectedCallback();
                WiFi.disconnect();
                Timer  = _DisconnectionMaxTime;
                _State = DISCONNECTION_IN_PROGRESS;
            } else if (_RestartConnectionRequest) {
                if (_OnDisconnectedCallback) _OnDisconnectedCallback();
                WiFi.disconnect();
                LOG(INFO, _LogName, "Ripartenza connessione");
                _RestartConnectionRequest = false;
                Timer  = _DisconnectionMaxTime;
                _State = DISCONNECTION_IN_PROGRESS;
            }
            break;

        case DISCONNECTION_IN_PROGRESS:
            if (WiFi.status() != WL_CONNECTED) {
                LOG(INFO, _LogName, "Disconnesso");
                _State = NOT_CONNECTED;
            } else if (Timeout) {
                LOG(WARNING, _LogName, "Timeout disconnessione");
                _State = NOT_CONNECTED;
            }
            _RestartConnectionRequest = false;
            break;

        case AP_MODE:
            if (!_Enabled) {
                WiFi.softAPdisconnect(true);
                WiFi.mode(WIFI_STA);
                LOG(INFO, _LogName, "AP mode chiuso");
                if (_OnDisconnectedCallback) _OnDisconnectedCallback();
                _State = NOT_CONNECTED;
            } else if (Timeout) {
                LOG(INFO, _LogName, "Tentativo STA in AP+STA mode...");
                WiFi.mode(WIFI_AP_STA);
                WiFi.setHostname(_Hostname.c_str());
                WiFi.begin(_SSID.c_str(), _Password.c_str());
                Timer  = _ConnectionMaxTime;
                _State = AP_STA_RETRY;
            } else if (_RestartConnectionRequest) {
                LOG(INFO, _LogName, "Ripartenza connessione");
                WiFi.softAPdisconnect(true);
                WiFi.mode(WIFI_STA);
                if (_OnDisconnectedCallback) _OnDisconnectedCallback();
                _RestartConnectionRequest = false;
                _State = NOT_CONNECTED;
            }
            break;

        case AP_STA_RETRY:
            if (!_Enabled) {
                WiFi.softAPdisconnect(true);
                WiFi.disconnect();
                WiFi.mode(WIFI_STA);
                LOG(INFO, _LogName, "AP mode chiuso");
                if (_OnDisconnectedCallback) _OnDisconnectedCallback();
                _State = NOT_CONNECTED;
            } else if (_RestartConnectionRequest) {
                WiFi.softAPdisconnect(true);
                WiFi.disconnect();
                WiFi.mode(WIFI_STA);
                LOG(INFO, _LogName, "Ripartenza connessione");
                if (_OnDisconnectedCallback) _OnDisconnectedCallback();
                _RestartConnectionRequest = false;
                _State = NOT_CONNECTED;
            } else if (WiFi.status() == WL_CONNECTED) {
                LOG(INFO, _LogName, "STA connesso | RSSI: " + String(WiFi.RSSI()) + " | IP: " + GetIPAddress());
                WiFi.softAPdisconnect(true);
                WiFi.mode(WIFI_STA);
                LOG(INFO, _LogName, "AP mode chiuso");
                Timer  = _PostConnectionDelay;
                _State = POST_CONNECTION_DELAY;
            } else if (Timeout) {
                LOG(WARNING, _LogName, "Tentativo STA fallito — torno in AP mode");
                WiFi.disconnect();
                WiFi.mode(WIFI_AP);
                Timer  = _APRetryInterval;
                _State = AP_MODE;
            }
            break;
    }
}
