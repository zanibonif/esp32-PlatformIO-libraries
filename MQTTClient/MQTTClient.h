#pragma once

#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <LoggerHandler.h>
#include <System.h>

#define MQTT_TOPIC_MAX_LENGTH   50    // char
#define MQTT_MESSAGE_MAX_LENGTH 1024  // char

typedef void (*MqttConnectionCallback)();
typedef void (*MqttDisconnectionCallback)();

class MQTTClient {
public:
    static MQTTClient& GetInstance ();
    MQTTClient (const MQTTClient&)            = delete;
    MQTTClient& operator= (const MQTTClient&) = delete;

    // Configurazione
    void SetClockTime (unsigned long ClockTime);
    void SetServer (const String& Address, uint16_t Port);
    void SetCredentials (const String& Username, const String& Password);
    void SetClientName (const String& Name);
    void SetOnConnectedCallback (MqttConnectionCallback Callback);
    void SetOnDisconnectedCallback (MqttDisconnectionCallback Callback);

    // Controllo runtime
    void Enable ();
    void Disable ();
    bool Subscribe (const String& Topic, void (*Callback)(char*, byte*, unsigned int));
    bool PublishString (const String& Topic, const String& Message);
    bool PublishJSON (const String& Topic, JsonDocument& Doc);

    // Chiamato ciclicamente
    void Loop ();

private:
    MQTTClient ();

    enum MQTTClientState {
        NOT_CONNECTED,
        CONNECTION_IN_PROGRESS,
        POST_CONNECTION_DELAY,
        PRE_SUBSCRIPTION_DELAY,
        CONNECTED
    };

    struct TopicCallbackPair {
        char              Topic[MQTT_TOPIC_MAX_LENGTH];
        void              (*CallbackFunction)(char*, byte*, unsigned int);
        SemaphoreHandle_t Semaphore = nullptr;
    };

    struct TaskParams {
        char*              Topic;
        byte*              Payload;
        unsigned int       Length;
        TopicCallbackPair* CallbackPair;
    };

    void _MqttCallback (char* Topic, byte* Payload, unsigned int Length);
    void _SubscribeTopics ();
    void _UnsubscribeTopics ();

    String             _LogName                        = "MQTTClient";
    unsigned long      _ClockTime                      = 100;   // milliseconds
    unsigned long      _KeepaliveTime                  = 90;    // seconds
    unsigned long      _SocketTimeout                  = 90;    // seconds

    WiFiClient         _EspClient;
    PubSubClient       _Client;
    String             _ServerAddress                  = "127.0.0.1";
    uint16_t           _ServerPort                     = 1883;
    String             _Username                       = "";
    String             _Password                       = "";
    String             _ClientName                     = "UndefinedClientName";

    unsigned long      _KeepAliveSemaphoreMaxTime      = 150;   // milliseconds
    unsigned long      _ConnectionMaxTime              = 5000;  // milliseconds
    unsigned long      _PostConnectionDelay            = 5000;  // milliseconds
    unsigned long      _PreSubscriptionDelay           = 1000;  // milliseconds

    bool               _Enabled                        = false;
    MQTTClientState    _State                          = NOT_CONNECTED;
    unsigned long      _Timer                          = ZERO_TIME;

    uint8_t            _MaxTopics                      = 10;
    TopicCallbackPair* _TopicCallbacks                 = nullptr;
    int                _TopicsCallbackTasksPriority    = 2;
    unsigned long      _TopicsCallbackSemaphoreMaxTime = 35;    // milliseconds
    uint8_t            _TopicsCount                    = 0;
    SemaphoreHandle_t  _KeepAliveSemaphore             = nullptr;

    MqttConnectionCallback    _OnConnectedCallback     = nullptr;
    MqttDisconnectionCallback _OnDisconnectedCallback  = nullptr;
};

extern MQTTClient& Mqtt;
