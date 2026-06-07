#include "MQTTClient.h"

MQTTClient& MQTTClient::GetInstance () {
    static MQTTClient Instance;
    return Instance;
}
MQTTClient& Mqtt = MQTTClient::GetInstance();

MQTTClient::MQTTClient ()
    : _Client(_EspClient),
      _TopicCallbacks(new TopicCallbackPair[_MaxTopics])
{
    _KeepAliveSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(_KeepAliveSemaphore);

    _Client.setCallback([this](char* Topic, byte* Payload, unsigned int Length) {
        this->_MqttCallback(Topic, Payload, Length);
    });

    for (int i = 0; i < _MaxTopics; ++i) {
        _TopicCallbacks[i].Semaphore = xSemaphoreCreateBinary();
        xSemaphoreGive(_TopicCallbacks[i].Semaphore);
    }

    LOG(INFO, _LogName, "Instance created");
}

// --- Configurazione ---

void MQTTClient::SetClockTime (unsigned long ClockTime) {
    _ClockTime = ClockTime;
}

void MQTTClient::SetServer (const String& Address, uint16_t Port) {
    _ServerAddress = Address;
    _ServerPort    = Port;
    LOG(INFO, _LogName, "Server: " + _ServerAddress + ":" + String(_ServerPort));
}

void MQTTClient::SetCredentials (const String& Username, const String& Password) {
    _Username = Username;
    _Password = Password;
    LOG(INFO, _LogName, "Credentials set for user: " + _Username);
}

void MQTTClient::SetClientName (const String& Name) {
    _ClientName = Name;
    LOG(INFO, _LogName, "ClientName: " + _ClientName);
}

void MQTTClient::SetOnConnectedCallback (MqttConnectionCallback Callback) {
    _OnConnectedCallback = Callback;
    LOG(INFO, _LogName, "Connected callback set");
}

void MQTTClient::SetOnDisconnectedCallback (MqttDisconnectionCallback Callback) {
    _OnDisconnectedCallback = Callback;
    LOG(INFO, _LogName, "Disconnected callback set");
}

// --- Controllo runtime ---

void MQTTClient::Enable () {
    _Enabled = true;
    LOG(INFO, _LogName, "Enabled");
}

void MQTTClient::Disable () {
    _Enabled = false;
    LOG(INFO, _LogName, "Disabled");
}

bool MQTTClient::Subscribe (const String& Topic, void (*Callback)(char*, byte*, unsigned int)) {
    if (_TopicsCount >= _MaxTopics) {
        LOG(ERROR, _LogName, "Cannot add topic: " + Topic + " — max topics reached");
        return false;
    }
    strncpy(_TopicCallbacks[_TopicsCount].Topic, Topic.c_str(), MQTT_TOPIC_MAX_LENGTH);
    _TopicCallbacks[_TopicsCount].CallbackFunction = Callback;
    _TopicsCount++;
    LOG(INFO, _LogName, "Topic added: " + Topic);
    return true;
}

bool MQTTClient::PublishString (const String& Topic, const String& Message) {
    bool Result = false;
    if (xSemaphoreTake(_KeepAliveSemaphore, _KeepAliveSemaphoreMaxTime / portTICK_PERIOD_MS)) {
        Result = _Client.publish(Topic.c_str(), Message.c_str(), 1);
        xSemaphoreGive(_KeepAliveSemaphore);
    } else {
        LOG(FATAL_ERROR, _LogName, "Failed to acquire semaphore in PublishString");
    }

    if (Result) {
        LOG(INFO, _LogName, "Published to <<" + Topic + ">>: " + Message);
    } else {
        LOG(ERROR, _LogName, "Publish failed to <<" + Topic + ">>");
    }
    return Result;
}

bool MQTTClient::PublishJSON (const String& Topic, JsonDocument& Doc) {
    String Message;
    serializeJson(Doc, Message);
    return PublishString(Topic, Message);
}

// --- Loop ---

void MQTTClient::Loop () {
    if (_Timer > _ClockTime) {
        _Timer = _Timer - _ClockTime;
    } else {
        _Timer = ZERO_TIME;
    }
    bool Timeout = (_Timer == ZERO_TIME);

    switch (_State) {
        case NOT_CONNECTED:
            if (_Enabled) {
                _Client.setServer(_ServerAddress.c_str(), _ServerPort);
                _Client.setKeepAlive(_KeepaliveTime);
                _Client.setBufferSize(MQTT_MESSAGE_MAX_LENGTH);
                _Client.setSocketTimeout(_SocketTimeout);
                LOG(INFO, _LogName, "Connecting to " + _ServerAddress + ":" + String(_ServerPort));
                if (xSemaphoreTake(_KeepAliveSemaphore, _KeepAliveSemaphoreMaxTime / portTICK_PERIOD_MS)) {
                    _Client.connect(_ClientName.c_str(), _Username.c_str(), _Password.c_str());
                    xSemaphoreGive(_KeepAliveSemaphore);
                }
                _Timer = _ConnectionMaxTime;
                _State = CONNECTION_IN_PROGRESS;
            }
            break;

        case CONNECTION_IN_PROGRESS:
            if (!_Enabled) {
                _Client.disconnect();
                _State = NOT_CONNECTED;
            } else if (Timeout) {
                LOG(ERROR, _LogName, "Connection timeout");
                _Client.disconnect();
                _State = NOT_CONNECTED;
            } else if (_Client.connected()) {
                LOG(INFO, _LogName, "Connected to " + _ServerAddress + ":" + String(_ServerPort));
                _Timer = _PostConnectionDelay;
                _State = POST_CONNECTION_DELAY;
            }
            break;

        case POST_CONNECTION_DELAY:
            if (!_Enabled) {
                _Client.disconnect();
                _State = NOT_CONNECTED;
            } else if (!_Client.connected()) {
                LOG(ERROR, _LogName, "Connection lost | state: " + String(_Client.state()));
                _Client.disconnect();
                _State = NOT_CONNECTED;
            } else if (Timeout) {
                if (_OnConnectedCallback) _OnConnectedCallback();
                _Timer = _PreSubscriptionDelay;
                _State = PRE_SUBSCRIPTION_DELAY;
            }
            break;

        case PRE_SUBSCRIPTION_DELAY:
            if (!_Enabled) {
                _Client.disconnect();
                _State = NOT_CONNECTED;
            } else if (!_Client.connected()) {
                LOG(ERROR, _LogName, "Connection lost | state: " + String(_Client.state()));
                if (_OnDisconnectedCallback) _OnDisconnectedCallback();
                _Client.disconnect();
                _State = NOT_CONNECTED;
            } else if (Timeout) {
                _SubscribeTopics();
                _State = CONNECTED;
            }
            break;

        case CONNECTED:
            if (!_Enabled) {
                if (_OnDisconnectedCallback) _OnDisconnectedCallback();
                _Client.disconnect();
                _State = NOT_CONNECTED;
            } else if (!_Client.connected()) {
                LOG(ERROR, _LogName, "Connection lost | state: " + String(_Client.state()));
                if (_OnDisconnectedCallback) _OnDisconnectedCallback();
                _Client.disconnect();
                _State = NOT_CONNECTED;
            }
            break;
    }

    if (xSemaphoreTake(_KeepAliveSemaphore, _KeepAliveSemaphoreMaxTime / portTICK_PERIOD_MS)) {
        _Client.loop();
        xSemaphoreGive(_KeepAliveSemaphore);
    } else {
        LOG(ERROR, _LogName, "Failed to acquire semaphore for Client.loop()");
    }
}

// --- Internals ---

void MQTTClient::_MqttCallback (char* Topic, byte* Payload, unsigned int Length) {
    LOG(INFO, _LogName, "Received from <<" + String(Topic) + ">>");
    for (int i = 0; i < _TopicsCount; ++i) {
        if (strcmp(Topic, _TopicCallbacks[i].Topic) == 0) {
            if (xSemaphoreTake(_TopicCallbacks[i].Semaphore, _TopicsCallbackSemaphoreMaxTime) == pdTRUE) {
                auto* Params = new TaskParams{Topic, Payload, Length, &_TopicCallbacks[i]};
                xTaskCreatePinnedToCore(
                    [](void* pvParams) {
                        TaskParams* P = reinterpret_cast<TaskParams*>(pvParams);
                        P->CallbackPair->CallbackFunction(P->Topic, P->Payload, P->Length);
                        xSemaphoreGive(P->CallbackPair->Semaphore);
                        delete P;
                        vTaskDelete(nullptr);
                    },
                    "MQTT_CbTask",
                    16384,
                    Params,
                    _TopicsCallbackTasksPriority,
                    nullptr,
                    0
                );
            } else {
                LOG(ERROR, _LogName, "Callback busy, skipping <<" + String(Topic) + ">>");
            }
            return;
        }
    }
}

void MQTTClient::_SubscribeTopics () {
    if (xSemaphoreTake(_KeepAliveSemaphore, _KeepAliveSemaphoreMaxTime / portTICK_PERIOD_MS)) {
        for (int i = 0; i < _TopicsCount; ++i) {
            bool Result = _Client.subscribe(_TopicCallbacks[i].Topic, 1);
            if (Result) {
                LOG(INFO, _LogName, "Subscribed: " + String(_TopicCallbacks[i].Topic));
            } else {
                LOG(ERROR, _LogName, "Subscribe failed: " + String(_TopicCallbacks[i].Topic));
            }
        }
        xSemaphoreGive(_KeepAliveSemaphore);
    } else {
        LOG(FATAL_ERROR, _LogName, "Failed to acquire semaphore in _SubscribeTopics");
    }
}

void MQTTClient::_UnsubscribeTopics () {
    if (xSemaphoreTake(_KeepAliveSemaphore, _KeepAliveSemaphoreMaxTime / portTICK_PERIOD_MS)) {
        for (int i = 0; i < _TopicsCount; ++i) {
            bool Result = _Client.unsubscribe(_TopicCallbacks[i].Topic);
            if (Result) {
                LOG(INFO, _LogName, "Unsubscribed: " + String(_TopicCallbacks[i].Topic));
            } else {
                LOG(ERROR, _LogName, "Unsubscribe failed: " + String(_TopicCallbacks[i].Topic));
            }
        }
        xSemaphoreGive(_KeepAliveSemaphore);
    } else {
        LOG(FATAL_ERROR, _LogName, "Failed to acquire semaphore in _UnsubscribeTopics");
    }
}
