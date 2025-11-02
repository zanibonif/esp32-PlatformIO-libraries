#ifndef WIFI_HANDLER_H
#define WIFI_HANDLER_H

#include <WiFi.h>
#include <System.h>
#include <LoggerHandler.h>

typedef void (*ConnectionCallback)();
typedef void (*DisconnectionCallback)();

class WifiHandler {
    private:

        String LogName = "WifiHandler";

        // Private enum
        enum WifiStateEnum {
            NOT_CONNECTED,
            CONNECTION_IN_PROGRESS,
            POST_CONNECTION_DELAY,
            CONNECTED,
            DISCONNECTION_IN_PROGRESS,
        };

        // Private variables
        int HandlerTaskPriority = 3;
        TaskHandle_t HandlerTaskPointer = NULL;
        bool WifiConnected = false;
        String WifiHostname = "UndefinedHostname";
        String WifiSSID = "";
        String WifiPassword = "";
        unsigned long ClockTime = 100; // milliseconds
        unsigned long PostConnectionDelay = 5000; // milliseconds
        unsigned long ConnectionMaxTime = 20000; // milliseconds
        unsigned long DisconnectionMaxTime = 20000; // milliseconds
        bool Enabled = false;

        ConnectionCallback OnConnectedCallback = nullptr;
        DisconnectionCallback OnDisconnectedCallback = nullptr;

        byte EncryptionKey[16] = {0x00, 0x01, 0x02, 0x23, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x1C, 0x0D, 0x0E, 0x0F};
        byte EncryptionIV[16]  = {0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x19, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00};

        // Private functions
        void HandlerTask(void *pvParameters);

    public:
        // Constructor and distructor
        WifiHandler();
        ~WifiHandler();

        // Public configuration functions
        void SetHostname(const String& hostname);
        void SetSSIDAndPassword(const String& ssid, const String& password);
        void SetEncryptionKey(const byte* key);
        void SetEncryptionIV(const byte* iv);

        // Public functions
        void Enable();
        void Disable();
        void SetOnConnectedCallback(ConnectionCallback Callback);
        void SetOnDisconnectedCallback(DisconnectionCallback Callback);
        bool IsConnected();
        int GetSignalStrength();
        String GetIPAddress();
};

// Costruttore
WifiHandler::WifiHandler() {
    LOG(INFO, LogName, "Instance created");

    BaseType_t Task;
    Task = xTaskCreatePinnedToCore([](void* pvParameters) {
        WifiHandler* _this = reinterpret_cast<WifiHandler*>(pvParameters);
        _this->HandlerTask(pvParameters);
    }, "WiFi_HandlerTask", 10240, this, HandlerTaskPriority, &HandlerTaskPointer, 0);

    if (Task == pdPASS) {
        LOG(INFO, LogName, "Task created");
    } else {
        LOG(INFO, LogName, "Failed to create task");
    }
}

// Distruttore
WifiHandler::~WifiHandler() {
    LOG(INFO, LogName, "Instance deleted");

    if (HandlerTaskPointer != NULL) {
        LOG(INFO, LogName, "Task deleted");
        vTaskDelete(HandlerTaskPointer);
    }
}

// Metodo per avviare la gestione WiFi
void WifiHandler::Enable() {
    Enabled = true;
    LOG(INFO, LogName, "Enabled");
}

void WifiHandler::Disable() {
    Enabled = false;
    LOG(INFO, LogName, "Disabled");
}

void WifiHandler::SetOnConnectedCallback(ConnectionCallback Callback) {
    OnConnectedCallback = Callback;
    LOG(INFO, LogName, "Connected callback set");
}

void WifiHandler::SetOnDisconnectedCallback(DisconnectionCallback Callback) {
    OnDisconnectedCallback = Callback;
    LOG(INFO, LogName, "Disconnected callback set");
}

// Metodo per verificare se è connesso
bool WifiHandler::IsConnected() {
    return WifiConnected;
}

// Metodo per ottenere la potenza del segnale
int WifiHandler::GetSignalStrength() {
    return 100 * (120 + WiFi.RSSI()) / 120;
}

// Metodo per ottenere l'indirizzo IP
String WifiHandler::GetIPAddress() {
    return WiFi.localIP().toString();
}

void WifiHandler::SetHostname(const String& hostname) {
    WifiHostname = hostname;
    LOG(INFO, LogName, "Hostname is " + WifiHostname);
}

void WifiHandler::SetSSIDAndPassword(const String& ssid, const String& password) {
    WifiSSID = ssid;
    WifiPassword = password;
    LOG(INFO, LogName, "SSID is " + WifiSSID);
    LOG(INFO, LogName, "Password is " + WifiPassword);
}

void WifiHandler::SetEncryptionKey(const byte* Key) {
    memcpy(EncryptionKey, Key, sizeof(EncryptionKey));
    LOG(INFO, LogName, "Encryption key updated");
}

void WifiHandler::SetEncryptionIV(const byte* IV) {
    memcpy(EncryptionIV, IV, sizeof(EncryptionIV));
    LOG(INFO, LogName, "Encryption IV updated");
}

void WifiHandler::HandlerTask(void *pvParameters) {
    WifiHandler *_this = reinterpret_cast<WifiHandler*>(pvParameters);
    static unsigned long Timer = ZERO_TIME;
    static unsigned long ClockTime = _this->ClockTime;
    WifiStateEnum State = NOT_CONNECTED;
    bool Timeout;

    TickType_t StartTick, ExecutionTick, TaskTickPeriod;
    TaskTickPeriod = ClockTime / portTICK_PERIOD_MS;

    while (true) {

        StartTick = xTaskGetTickCount();

        if ((Timer - ClockTime) < ClockTime) {
            Timer = ZERO_TIME;
        } else {
            Timer = Timer - ClockTime;
        }
        Timeout = (Timer == ZERO_TIME);

        int WifiStatus = WiFi.status();
        switch (State) {
            case NOT_CONNECTED:
                if (_this->Enabled) {
                    WiFi.setHostname(_this->WifiHostname.c_str());
                    WiFi.setSleep(WIFI_PS_NONE);
                    WiFi.useStaticBuffers(true);
                    WiFi.mode(WIFI_STA);
                    WiFi.begin(_this->WifiSSID.c_str(), _this->WifiPassword.c_str());
                    LOG(INFO, LogName, "Attempting to connect to " + _this->WifiSSID);
                    Timer = _this->ConnectionMaxTime;
                    State = CONNECTION_IN_PROGRESS;
                }
                break;

            case CONNECTION_IN_PROGRESS:
                if (Timeout) {
                    LOG(WARNING, LogName, "Connection timeout");
                    WiFi.disconnect();
                    Timer = _this->DisconnectionMaxTime;
                    State = DISCONNECTION_IN_PROGRESS;
                } else if (WifiStatus == WL_CONNECTED) {
                    LOG(INFO, LogName, "Successfully connected to " + _this->WifiSSID + " - Signal Strength: " + String(WiFi.RSSI()) + "% - IP Address: " + _this->GetIPAddress());
                    Timer = _this->PostConnectionDelay;
                    State = POST_CONNECTION_DELAY;
                } else if (!_this->Enabled) {
                    LOG(INFO, LogName, "Disconnection in progress");
                    WiFi.disconnect();
                    Timer = _this->DisconnectionMaxTime;
                    State = DISCONNECTION_IN_PROGRESS;
                }
                break;

            case POST_CONNECTION_DELAY:
                if (!_this->Enabled) {
                    LOG(INFO, LogName, "Disconnection in progress");
                    WiFi.disconnect();
                    Timer = _this->DisconnectionMaxTime;
                    State = DISCONNECTION_IN_PROGRESS;
                } if (WifiStatus != WL_CONNECTED) {
                    LOG(WARNING, LogName, "Connection lost, WiFi status is " + String(WiFi.status()));
                    WiFi.disconnect();
                    Timer = _this->DisconnectionMaxTime;
                    State = DISCONNECTION_IN_PROGRESS;
                } else if (Timeout) {
                    if (_this->OnConnectedCallback) {
                        _this->OnConnectedCallback();
                    }
                    State = CONNECTED;
                }
                break;

            case CONNECTED:
                if (!_this->Enabled) {
                    LOG(INFO, LogName, "Disconnection in progress");
                    WiFi.disconnect();
                    Timer = _this->DisconnectionMaxTime;
                    State = DISCONNECTION_IN_PROGRESS;
                } if (WifiStatus != WL_CONNECTED) {
                    LOG(WARNING, LogName, "Connection lost, WiFi status is " + String(WiFi.status()));
                    if (_this->OnDisconnectedCallback) {
                        _this->OnDisconnectedCallback();
                    }
                    WiFi.disconnect();
                    Timer = _this->DisconnectionMaxTime;
                    State = DISCONNECTION_IN_PROGRESS;
                }
                break;

            case DISCONNECTION_IN_PROGRESS:
                if (Timeout) {
                    LOG(WARNING, LogName, "Disconnection timeout");
                    State = NOT_CONNECTED;
                } else if (WifiStatus != WL_CONNECTED) {
                    LOG(INFO, LogName, "Disconnected");
                    State = NOT_CONNECTED;
                }
                break;
        }
        _this->WifiConnected = (State == CONNECTED);

        ExecutionTick = xTaskGetTickCount() - StartTick;
        if (ExecutionTick < TaskTickPeriod) {
          vTaskDelay(TaskTickPeriod - ExecutionTick);
        }
    }
}

#endif // WIFI_HANDLER_H
