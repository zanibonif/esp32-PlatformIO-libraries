# MQTTClient

Client MQTT con state machine, reconnect automatico e dispatch dei messaggi ricevuti. Richiede WiFi connesso e `Loop()` agganciato al DMPOScheduler.

## Alias globale

```cpp
extern MQTTClient& Mqtt;
```

## Setup

```cpp
Mqtt.SetServer("192.168.1.100", 1883);
Mqtt.SetClientName("esp32-device");
Mqtt.SetCredentials("user", "password");   // opzionale

Mqtt.SetOnConnectedCallback([]() {
    LOG(INFO, "Main", "MQTT connesso");
    Mqtt.Subscribe("cmd/device", [](char* Topic, byte* Payload, unsigned int Len) {
        String Msg = String((char*)Payload).substring(0, Len);
        LOG(INFO, "MQTT", "Ricevuto: " + Msg);
    });
});
Mqtt.SetOnDisconnectedCallback([]() {
    LOG(WARNING, "Main", "MQTT disconnesso");
});
```

`Enable()` va chiamato dopo che il WiFi è connesso:

```cpp
Wifi.SetOnConnectedCallback([]() {
    Mqtt.Enable();
});
Wifi.SetOnDisconnectedCallback([]() {
    Mqtt.Disable();
});
```

## Wiring con DMPOScheduler

```cpp
DMPOScheduler::TaskConfig MqttTask;
MqttTask.Name        = "MQTT";
MqttTask.PeriodUs    = 100000;   // 100ms
MqttTask.AppCritical = false;
MqttTask.StackSize   = 8192;
Scheduler.AddTask(MqttTask);
Scheduler.AddFunction(MqttTask, []() { Mqtt.Loop(); });

Mqtt.SetClockTime(100);   // ms — deve corrispondere a PeriodUs / 1000
```

## Publish

```cpp
Mqtt.PublishString("stato/device", "online");

JsonDocument Doc;
Doc["temp"] = 23.5;
Doc["hum"]  = 60;
Mqtt.PublishJSON("telemetry/device", Doc);
```

## Subscribe

```cpp
Mqtt.Subscribe("cmd/device", [](char* Topic, byte* Payload, unsigned int Len) {
    String Msg((char*)Payload, Len);
    // gestire il messaggio
});
```

I subscribe vanno registrati nella callback `OnConnected`: vengono rifatti automaticamente ad ogni riconnessione.

## Controllo runtime

```cpp
Mqtt.Enable();
Mqtt.Disable();
```

## Dipendenze

- `PubSubClient`
- `ArduinoJson`
- `LoggerHandler`
- `System`
- WiFi connesso

## Note

- Stack minimo consigliato: 8192 byte (i callback JSON possono essere pesanti).
- Max 10 topic sottoscritti contemporaneamente (configurabile internamente).
- Lunghezza massima topic: 50 caratteri (`MQTT_TOPIC_MAX_LENGTH`).
- Lunghezza massima messaggio: 1024 caratteri (`MQTT_MESSAGE_MAX_LENGTH`).
