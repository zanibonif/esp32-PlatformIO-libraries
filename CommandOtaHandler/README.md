# CommandOtaHandler

Aggiornamento firmware OTA via rete (ArduinoOTA). Richiede WiFi connesso e `Loop()` agganciato al DMPOScheduler.

## Alias globale

```cpp
extern CommandOtaHandler& Ota;
```

## Setup

```cpp
Ota.SetHostname("mio-dispositivo");   // visibile nel network per l'upload
Ota.SetPassword("ota-secret");        // opzionale
Ota.SetPort(3232);                    // default 3232
```

`Start()` va chiamato dopo che il WiFi è connesso:

```cpp
Wifi.SetOnConnectedCallback([]() {
    Ota.Start();
});
Wifi.SetOnDisconnectedCallback([]() {
    Ota.Stop();
});
```

## Wiring con DMPOScheduler

```cpp
DMPOScheduler::TaskConfig OtaTask;
OtaTask.Name        = "OTA";
OtaTask.PeriodUs    = 100000;   // 100ms
OtaTask.AppCritical = false;
OtaTask.StackSize   = 4096;
Scheduler.AddTask(OtaTask);
Scheduler.AddFunction(OtaTask.ID, []() { Ota.Loop(); });
```

## Pausa task durante upload

Durante l'upload è consigliato disabilitare i task non essenziali per liberare risorse:

```cpp
Scheduler.AddFunction(OtaTask.ID, []() {
    bool WasUploading = Ota.IsUploadInProgress();
    Ota.Loop();
    if (!WasUploading && Ota.IsUploadInProgress())
        Scheduler.DisableAllTasks();  // oppure disabilitare selettivamente
});
```

## Controllo runtime

```cpp
Ota.Start();
Ota.Stop();
bool Uploading = Ota.IsUploadInProgress();
```

## Dipendenze

- `ArduinoOTA`
- `LoggerHandler`
- `System`
- WiFi connesso
