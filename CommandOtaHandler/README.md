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
Scheduler.AddFunction(OtaTask, []() { Ota.Loop(); });
```

## Pausa task durante upload

Durante l'upload è consigliato disabilitare i task non essenziali per liberare risorse:

```cpp
Scheduler.AddFunction(OtaTask, []() {
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

## Partition table

OTA richiede una partition table con almeno due slot app (`ota_0` / `ota_1`) e la partizione `otadata`. Senza di essa il firmware si avvia ma l'aggiornamento OTA fallisce silenziosamente.

Schema minimo (`partitions.csv`):

```
#   Name, Type,  SubType,   Offset,      Size, Flags
     nvs, data,      nvs,   0x9000,    0x5000,
 otadata, data,      ota,   0xE000,    0x2000,
    app0,  app,    ota_0,  0x10000,  0x160000,
    app1,  app,    ota_1, 0x170000,  0x160000,
  spiffs, data,   spiffs, 0x2D0000,  0x120000,
coredump, data, coredump, 0x3F0000,   0x10000,
```

Dimensionamento: ogni slot app deve essere **≥ dimensione del firmware compilato** (verificare `.pio/build/<env>/firmware.bin`). Ridurre gli slot app al minimo necessario massimizza lo spazio disponibile per LittleFS.

> Se non si usa OTA conviene rimuovere `otadata` e `app1`, cambiare `app0` in `factory` e assegnare lo spazio liberato a `spiffs` (guadagno tipico: ~2 MB su flash da 4 MB).

## Dipendenze

- `ArduinoOTA`
- `LoggerHandler`
- `System`
- WiFi connesso
