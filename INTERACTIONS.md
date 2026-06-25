# Interazioni tra librerie

Pattern ricorrenti di wiring tra le librerie. Da usare come riferimento per il codice applicativo.

---

## Sequenza di inizializzazione tipica in setup()

```
1.  Logger             → SetSerialSpeed, Enable
2.  LittleFSHandler    → Init()                       // obbligatorio prima di ParametersHandler
3.  ParametersHandler  → SetClockTime, AddFile, AddParameter, Begin()
4.  System             → SetCpuFrequency
5.  WifiHandler        → SetSSIDAndPassword (letti da Parameters) + Enable
6.  NtpHandler         → configurazione (senza Enable — lo farà WiFi)
7.  I2cBusHandler      → SetSdaPin/SetSclPin, SetClockTime, Enable
8.  DS3231_RtcHandler  → Enable   // registrato su I2cBus (0x68) alla costruzione
9.  Logger             → SetDateTimeProvider(&Rtc)   // timestamp da RTC da subito
10. DMPOScheduler      → AddTask + AddFunction per ogni task
11. Scheduler.Begin()
```

`ParametersHandler` va prima di `WifiHandler` perché le credenziali WiFi si leggono da `Parameters.Get(WIFI_SSID)` prima di `Wifi.Enable()`.

`I2cBusHandler` è l'**unico punto del sistema** che chiama `Wire.begin()` (lo fa la `Loop()` al primo ciclo da abilitato): nessun'altra libreria o codice applicativo deve inizializzare il bus. La `I2cBus.Loop()` (init + probe di disponibilità) va agganciata al task **low-rate**.

In `loop()` cancellare il task Arduino: tutto il lavoro avviene nei task FreeRTOS.

```cpp
void loop() {
    vTaskDelete(nullptr);
}
```

---

## WiFi come driver dei servizi di rete

WiFi notifica gli altri servizi tramite callback. I servizi dipendenti dal WiFi si avviano e fermano in risposta:

```cpp
Wifi.SetOnConnectedCallback([]() {
    WebServer.Start();
    FileManager.Begin(WebServer.GetServer());   // idempotente — sicuro anche se già chiamato da OnAPStarted
    Logger.SetWebServer(WebServer.GetServer());
    Logger.SetWebServerRunning();
    Logger.EnableWebSerial();
    Ntp.Enable();
});

Wifi.SetOnAPStartedCallback([]() {
    WebServer.Start();
    FileManager.Begin(WebServer.GetServer());   // file manager disponibile anche in modalità AP
});

Wifi.SetOnDisconnectedCallback([]() {
    WebServer.Stop();
    Logger.SetWebServerNotRunning();
    Ntp.Disable();
    // Mqtt.Disable(), Ota.Stop(), ecc.
});
```

Regola: **nessun servizio di rete chiama Enable() in setup()**. Lo fa sempre la callback `OnConnected` del WiFi.

---

## Catena di sincronizzazione tempo: NTP → RTC → Logger

NTP sincronizza il DS3231 alla prima connessione. Il Logger usa sempre l'RTC come sorgente (disponibile anche offline).

```cpp
// Setup
Logger.SetDateTimeProvider(&Rtc);   // RTC come provider permanente

// Callback NTP: al primo sync, aggiorna il RTC
Ntp.SetOnSyncCallback([]() {
    unsigned long RawTime = Ntp.GetEpochTime();
    time_t Time = static_cast<time_t>(RawTime);
    struct tm T;
    localtime_r(&Time, &T);
    Rtc.SetDateTime(T.tm_year + 1900, T.tm_mon + 1, T.tm_mday,
                    T.tm_hour, T.tm_min, T.tm_sec);
});
```

Flusso:
```
WiFi connesso → Ntp.Enable() → NTP sincronizzato → RTC aggiornato
Logger usa Rtc.GetFormattedTime() per i timestamp (online e offline)
```

---

## Struttura task DMPOScheduler

Pattern a fasce di frequenza, con task APERIODIC per Logger e operazioni non critiche:

```cpp
// HIGH_RATE   — 10ms  — I/O veloci, sensori, segnali digitali
// MEDIUM_RATE — 50ms  — elaborazioni intermedie
// LOW_RATE    — 100ms — Parameters.Loop(), I2cBus.Loop(), Rtc.Loop(), WiFi.Loop(), Mqtt.Loop(), Ota.Loop()
// APERIODIC   — 200ms con deadline 60s — Logger.Loop(), Ntp.Loop() (può bloccare ~1s), diagnostica

DMPOScheduler::TaskConfig LowRateTask;
LowRateTask.Name        = "LOW_RATE_TASK";
LowRateTask.PeriodUs    = 100000;
LowRateTask.DeadlineUs  = 100000;
LowRateTask.AppCritical = false;
LowRateTask.StackSize   = 4096;
Scheduler.AddTask(LowRateTask);
Scheduler.AddFunction(LowRateTask, []() { Wifi.Loop(); });

DMPOScheduler::TaskConfig AperiodicTask;
AperiodicTask.Name        = "APERIODIC_TASK";
AperiodicTask.PeriodUs    = 200000;
AperiodicTask.DeadlineUs  = 60000000;   // deadline 60s → priorità bassa
AperiodicTask.AppCritical = false;
AperiodicTask.StackSize   = 4096;
Scheduler.AddTask(AperiodicTask);
Scheduler.AddFunction(AperiodicTask, []() { Ntp.Loop(); });      // può bloccare ~1s in sync (vedi README NtpHandler)
Scheduler.AddFunction(AperiodicTask, []() { Logger.Loop(); });
```

Regola generale: le `Loop()` che possono bloccare (Ntp in sincronizzazione, Radar in configurazione, ecc.) vanno nel task **aperiodico** con deadline tollerante; i getter delle librerie devono invece essere sempre non bloccanti (lettura di cache aggiornata dalla Loop — vedi DS3231_RtcHandler e NTPClient).

`SetClockTime` di ogni libreria deve corrispondere al periodo del task che la ospita:

```cpp
Wifi.SetClockTime(LowRateTask.PeriodUs / MILLISECONDS_TO_MICROSECONDS);    // 100ms
Ntp.SetClockTime(AperiodicTask.PeriodUs / MILLISECONDS_TO_MICROSECONDS);   // 200ms
```

---

## ParametersHandler — init e task

`ParametersHandler` va inizializzato in `setup()` prima di qualunque libreria che legga da esso (es. WiFi):

```cpp
Parameters.SetClockTime(LOW_RATE_TASK_PERIOD / MILLISECONDS_TO_MICROSECONDS);

ParametersHandler::FileConfig ConfigFile;
ConfigFile.Path = "/data/configurations.csv";
Parameters.AddFile(ConfigFile);
Parameters.AddParameter(WIFI_SSID, "WiFi SSID", "default", ConfigFile);

Parameters.Begin();   // carica + salva subito (persiste i nuovi parametri)

Wifi.SetSSIDAndPassword(Parameters.Get(WIFI_SSID), Parameters.Get(WIFI_PASSWORD));
Wifi.Enable();
```

`Loop()` va nel task **low-rate** (stesso task di I2cBus e Rtc):

```cpp
Scheduler.AddFunction(LowRateTask, []() { Parameters.Loop(); });
```

Prima di un reboot o OTA, forzare la scrittura dei file pendenti:

```cpp
Parameters.ForceWrite();
ESP.restart();
```

---

## Logger + WebSerial + WebServer

WebSerial viene inizializzato solo dopo che il WebServer è avviato. L'ordine in `OnWifiConnected`:

```cpp
WebServer.Start();                           // 1. avvia il server
Logger.SetWebServer(WebServer.GetServer());  // 2. passa il puntatore al Logger
Logger.EnableWebSerial();                    // 3. registra la route /webserial
Logger.SetWebServerRunning();               // 4. abilita la pubblicazione
```

In `OnWifiDisconnected`:

```cpp
WebServer.Stop();
Logger.SetWebServerNotRunning();   // ferma la pubblicazione, non serve DisableWebSerial
```

---

## DigitalSignalHandler — Update() in task

`DigitalSignalHandler` non è un singleton. Ogni istanza deve ricevere `Update()` a ogni tick del task che la ospita:

```cpp
DigitalSignalHandler ButtonA;

// setup()
ButtonA.SetClockTime(HIGH_RATE_TASK_PERIOD / MILLISECONDS_TO_MICROSECONDS);
ButtonA.SetActivationDelay(50);
ButtonA.SetActivationCallback([]() { /* fronte salita */ });
ButtonA.Enable();

// nel task
Scheduler.AddFunction(HighRateTask, []() {
    ButtonA.Update(digitalRead(PIN_BUTTON));
});
```
