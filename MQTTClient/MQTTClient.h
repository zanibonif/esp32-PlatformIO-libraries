#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <LoggerHandler.h>


#define MQTT_TOPIC_MAX_LENGTH    50    // char
#define MQTT_MESSAGE_MAX_LENGHT  1024  // char

typedef void (*ConnectionCallback)();
typedef void (*DisconnectionCallback)();

class MQTTClient {
    private:
        String LogName = "MQTTClient";
        enum MQTTClientStateEnum {
            NOT_CONNECTED,
            CONNECTION_IN_PROGRESS,
            POST_CONNECTION_DELAY,
            PRE_SUBSCRIPTION_DELAY,
            CONNECTED
        };

        struct TopicCallbackPair {
            char Topic[MQTT_TOPIC_MAX_LENGTH];
            void (*CallbackFunction)(char*, byte*, unsigned int);
            SemaphoreHandle_t Semaphore = xSemaphoreCreateBinary();
        };

        struct TaskParams {
            char* Topic;
            byte* Payload;
            unsigned int Length;
            struct TopicCallbackPair* CallbackPair;
        };


        unsigned long KeepaliveTime = 90; // seconds
        unsigned long SocketTimeout = 90; // seconds

        WiFiClient EspClient;
        PubSubClient Client;
        int HandlerTaskPriority = 9;
        TaskHandle_t HandlerTaskPointer = NULL;
        String ServerAddress = "127.0.0.1";
        uint16_t ServerPort = 1883;
        String Username = "";
        String Password = "";
        String ClientName = "UndefinedClientName";
        unsigned long ClockTime = 100; // milliseconds
        unsigned long KeepAliveSemaphoreMaxTime = 150; // milliseconds
        unsigned long ConnectionMaxTime = 5000; // milliseconds
        unsigned long PostConnectionDelay = 5000; // milliseconds
        unsigned long PreSubscriptionDelay = 1000; // milliseconds
        bool Enabled = false;
        uint8_t MaxTopics = 10;
        TopicCallbackPair* TopicCallbacks;
        int TopicsCallbackTasksPriority = 2;
        unsigned long TopicsCallbackSemaphoreMaxTime = 35; // milliseconds


        uint8_t TopicsCount = 0;

        SemaphoreHandle_t KeepAliveSemaphore;

        ConnectionCallback OnConnectedCallback = nullptr;
        DisconnectionCallback OnDisconnectedCallback = nullptr;

        void MqttCallback(char* Topic, byte* Payload, unsigned int Length);
        void SubscribeTopics();
        void UnsubscribeTopics();
        void HandlerTask(void *pvParameters);

    public:
        // Constructor and destructor
        MQTTClient();
        ~MQTTClient();

        // Public configuration functions
        void SetServer(const String& Address, uint16_t Port);
        void SetCredentials(const String& User, const String& Pass);
        void SetClientName(const String& Name);

        // Public functions
        void Enable();
        void Disable();
        void SetOnConnectedCallback(ConnectionCallback Callback);
        void SetOnDisconnectedCallback(DisconnectionCallback Callback);
        bool Subscribe(const String& Topic, void (*Callback)(char*, byte*, unsigned int));
        bool PublishString(const String& Topic, const String& Message);
        bool PublishJSON(const String& Topic, JsonDocument& Doc);
};

MQTTClient::MQTTClient() : Client(EspClient), TopicCallbacks(new TopicCallbackPair[MaxTopics]) {
    LOG(INFO, LogName, "Instance created");

    KeepAliveSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(KeepAliveSemaphore);

    Client.setCallback([this](char* Topic, byte* Payload, unsigned int Length) {
        this->MqttCallback(Topic, Payload, Length);
    });
    for (int i = 0; i < MaxTopics; ++i) {
        TopicCallbacks[i].Semaphore = xSemaphoreCreateBinary();
        xSemaphoreGive(TopicCallbacks[i].Semaphore);
    }

    BaseType_t Task;
    Task = xTaskCreatePinnedToCore([](void* pvParameters) {
        MQTTClient* _this = reinterpret_cast<MQTTClient*>(pvParameters);
        _this->HandlerTask(pvParameters);
    }, "MQTT_HandlerTask", 10240, this, HandlerTaskPriority, &HandlerTaskPointer, 0);

    if (Task == pdPASS) {
        LOG(INFO, LogName, "Task created");
    } else {
        LOG(FATAL_ERROR, LogName, "Failed to create task");
    }

}

MQTTClient::~MQTTClient() {
    delete[] TopicCallbacks;
    vSemaphoreDelete(KeepAliveSemaphore);
    LOG(INFO, LogName, "Instance deleted");

    if (HandlerTaskPointer != NULL) {
        LOG(INFO, LogName, "Task deleted");
        vTaskDelete(HandlerTaskPointer);
    }
}

void MQTTClient::Enable() {
    Enabled = true;
    LOG(INFO, LogName, "Enabled");
}

void MQTTClient::Disable() {
    Enabled = false;
    LOG(INFO, LogName, "Disabled");
}

void MQTTClient::SetOnConnectedCallback(ConnectionCallback Callback) {
    OnConnectedCallback = Callback;
    LOG(INFO, LogName, "Connected callback set");
}

void MQTTClient::SetOnDisconnectedCallback(DisconnectionCallback Callback) {
    OnDisconnectedCallback = Callback;
    LOG(INFO, LogName, "Disconnected callback set");
}

void MQTTClient::SetServer(const String& Address, uint16_t Port) {
    ServerAddress = Address;
    ServerPort = Port;
    LOG(INFO, LogName, "Server address set to " + String(ServerAddress) + ":" + String(ServerPort));
}

void MQTTClient::SetCredentials(const String& User, const String& Pass) {
    Username = User;
    Password = Pass;
    LOG(INFO, LogName, "Username is " + Username);
    LOG(INFO, LogName, "Password is " + Password);
}

void MQTTClient::SetClientName(const String& Name) {
    ClientName = Name;
    LOG(INFO, LogName, "ClientName is " + ClientName);
}

bool MQTTClient::Subscribe(const String& Topic, void (*Callback)(char*, byte*, unsigned int)) {
    bool Success;
    if (TopicsCount >= MaxTopics) {
        LOG(ERROR, LogName, "Unable to add topic: " + Topic + ", max topics reached");
        Success = false;
    } else {
        strncpy(TopicCallbacks[TopicsCount].Topic, Topic.c_str(), MQTT_TOPIC_MAX_LENGTH);
        LOG(INFO, LogName, "Added handled topic: " + Topic);
        TopicCallbacks[TopicsCount].CallbackFunction = Callback;
        TopicsCount++;
        Success = true;
    }
    return Success;
}

bool MQTTClient::PublishString(const String& Topic, const String& Message) {
    bool Result = false;
    if (xSemaphoreTake(KeepAliveSemaphore, KeepAliveSemaphoreMaxTime / portTICK_PERIOD_MS)) {
        bool Result = Client.publish(Topic.c_str(), Message.c_str(), 1);
        xSemaphoreGive(KeepAliveSemaphore);
    } else {
        LOG(FATAL_ERROR, LogName, "Failed to acquire KeepAliveSemaphore semaphore in PublishString");
    }

    if (Result) {
        LOG(INFO, LogName, "Data successfully sent to topic <<" + Topic + ">> with value = " + Message);
    } else {
        LOG(ERROR, LogName, "Failed to send data to topic <<" + Topic + ">>. Data = " + Message);
    }
    return Result;
}

bool MQTTClient::PublishJSON(const String& Topic, JsonDocument& Doc) {
    String Message;
    serializeJson(Doc, Message);
    return PublishString(Topic, Message);
}

void MQTTClient::MqttCallback(char* Topic, byte* Payload, unsigned int Length) {
    LOG(INFO, LogName, "Data successfully received from topic <<" + String(Topic) + ">>");
    for (int i = 0; i < TopicsCount; ++i) {
        if (strcmp(Topic, TopicCallbacks[i].Topic) == 0) {
            if (xSemaphoreTake(TopicCallbacks[i].Semaphore, TopicsCallbackSemaphoreMaxTime) == pdTRUE) {
                auto* TaskParamsInstance = new TaskParams{Topic, Payload, Length, &TopicCallbacks[i]};
                xTaskCreatePinnedToCore(
                    [](void* pvParams) {
                        TaskParams* Params = reinterpret_cast<TaskParams*>(pvParams);
                        Params->CallbackPair->CallbackFunction(Params->Topic, Params->Payload, Params->Length);
                        xSemaphoreGive(Params->CallbackPair->Semaphore);
                        delete Params;
                        vTaskDelete(nullptr);
                    },
                    "MQTT_CallbackTask",
                    16384,
                    TaskParamsInstance,
                    TopicsCallbackTasksPriority,
                    nullptr,
                    0
                );
            } else {
                LOG(ERROR, LogName, "Callback in progress, skipping message for topic <<" + String(Topic) + ">>");
            }
            return;
        }
    }
}

void MQTTClient::SubscribeTopics() {

    if (xSemaphoreTake(KeepAliveSemaphore, KeepAliveSemaphoreMaxTime / portTICK_PERIOD_MS)) {
        for (int i = 0; i < TopicsCount; ++i) {
            bool result = Client.subscribe(TopicCallbacks[i].Topic, 1);
            if (result) {
                LOG(INFO, LogName, "Subscribed topic " + String(TopicCallbacks[i].Topic));
            } else {
                LOG(ERROR, LogName, "Unable to add subscription for topic " + String(TopicCallbacks[i].Topic));
            }
        }
        xSemaphoreGive(KeepAliveSemaphore);
    } else {
        LOG(FATAL_ERROR, LogName, "Failed to acquire KeepAliveSemaphore semaphore in SubscribeTopics");
    }
}

void MQTTClient::UnsubscribeTopics() {
    if (xSemaphoreTake(KeepAliveSemaphore, KeepAliveSemaphoreMaxTime / portTICK_PERIOD_MS)) {
        for (int i = 0; i < TopicsCount; ++i) {
            bool result = Client.unsubscribe (TopicCallbacks[i].Topic);
            if (result) {
                LOG(INFO, LogName, "Unsubscribed topic " + String(TopicCallbacks[i].Topic));
            } else {
                LOG(ERROR, LogName, "Unable to remove subscription for topic " + String(TopicCallbacks[i].Topic));
            }
        }
        xSemaphoreGive(KeepAliveSemaphore);
    } else {
        LOG(FATAL_ERROR, LogName, "Failed to acquire KeepAliveSemaphore semaphore in UnsubscribeTopics");
    }
}

void MQTTClient::HandlerTask(void *pvParameters) {
    MQTTClient *_this = reinterpret_cast<MQTTClient*>(pvParameters);
    static unsigned long Timer = ZERO_TIME;
    static unsigned long ClockTime = _this->ClockTime;
    MQTTClientStateEnum State = NOT_CONNECTED;
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

        switch (State) {
            case NOT_CONNECTED:
                if (_this->Enabled) {
                    LOG(INFO, LogName, "Connection enabled");
                    Client.setServer(ServerAddress.c_str(), ServerPort);
                    Client.setKeepAlive(_this->KeepaliveTime);
                    Client.setBufferSize(MQTT_MESSAGE_MAX_LENGHT);
                    Client.setSocketTimeout(_this->SocketTimeout);
                    LOG(INFO, LogName, "Attempting to connect to " + String(ServerAddress) + ":" + String(ServerPort));
                    if (xSemaphoreTake(KeepAliveSemaphore, KeepAliveSemaphoreMaxTime / portTICK_PERIOD_MS)) {
                        Client.connect(ClientName.c_str(), Username.c_str(), Password.c_str());
                        xSemaphoreGive(KeepAliveSemaphore);
                    } else {
                        LOG(ERROR, LogName, "Failed to acquire KeepAliveSemaphore semaphore for Client.connect()");
                    }
                    Timer = _this->ConnectionMaxTime;
                    State = CONNECTION_IN_PROGRESS;
                }
                break;

            case CONNECTION_IN_PROGRESS:

                if (!_this->Enabled) {
                    LOG(INFO, LogName, "Disconnection in progress");
                    Client.disconnect();
                    State = NOT_CONNECTED;
                } else if (Timeout) {
                    LOG(ERROR, LogName, "Connection timeout");
                    Client.disconnect();
                    State = NOT_CONNECTED;
                } else if (Client.connected()) {
                    LOG(INFO, LogName, "Successfully connected to " + String(ServerAddress) + ":" + String(ServerPort));
                    Timer = _this->PostConnectionDelay;
                    State = POST_CONNECTION_DELAY;
                }
                break;


            case POST_CONNECTION_DELAY:
                if (!_this->Enabled) {
                    LOG(INFO, LogName, "Disconnection in progress");
                    Client.disconnect();
                    State = NOT_CONNECTED;
                } else if (!Client.connected()) {
                    LOG(ERROR, LogName, "Connection lost from " + String(ServerAddress) + ":" + String(ServerPort) + " client state is " + String(Client.state()));
                    Client.disconnect();
                    State = NOT_CONNECTED;
                } else if (Timeout) {
                    if (_this->OnConnectedCallback) {
                        _this->OnConnectedCallback();
                    }
                    Timer = _this->PreSubscriptionDelay;
                    State = PRE_SUBSCRIPTION_DELAY;
                }
                break;


            case PRE_SUBSCRIPTION_DELAY:
                if (!_this->Enabled) {
                    LOG(INFO, LogName, "Disconnection in progress");
                    Client.disconnect();
                    State = NOT_CONNECTED;
                } else if (!Client.connected()) {
                    LOG(ERROR, LogName, "Connection lost from " + String(ServerAddress) + ":" + String(ServerPort) + " client state is " + String(Client.state()));
                    if (_this->OnDisconnectedCallback) {
                        _this->OnDisconnectedCallback();
                    }
                    Client.disconnect();
                    State = NOT_CONNECTED;
                } else if (Timeout) {
                    SubscribeTopics();
                    State = CONNECTED;
                }
                break;

            case CONNECTED:
                if (!_this->Enabled) {
                    LOG(INFO, LogName, "Disconnection in progress");
                    if (_this->OnDisconnectedCallback) {
                        _this->OnDisconnectedCallback();
                    }
                    Client.disconnect();
                    State = NOT_CONNECTED;
                } else if (!Client.connected()) {
                    LOG(ERROR, LogName, "Connection lost from " + String(ServerAddress) + ":" + String(ServerPort) + " client state is " + String(Client.state()));
                    if (_this->OnDisconnectedCallback) {
                        _this->OnDisconnectedCallback();
                    }
                    Client.disconnect();
                    State = NOT_CONNECTED;
                }
                break;
        }

        if (xSemaphoreTake(KeepAliveSemaphore, KeepAliveSemaphoreMaxTime / portTICK_PERIOD_MS)) {
            Client.loop();
            xSemaphoreGive(KeepAliveSemaphore);
        } else {
            LOG(ERROR, LogName, "Failed to acquire KeepAliveSemaphore semaphore for Client.loop()");
        }

        ExecutionTick = xTaskGetTickCount() - StartTick;

        if (ExecutionTick < TaskTickPeriod) {
          vTaskDelay(TaskTickPeriod - ExecutionTick);
        }
    }
}

#endif // MQTT_CLIENT_H