# NtpHandler

Sincronizzazione orario via NTP. Implementa `DateTimeProvider`. Richiede WiFi connesso prima di `Enable()` e `Loop()` agganciato al DMPOScheduler.

## Alias globale

```cpp
extern NtpHandler& Ntp;
```

## Setup

```cpp
Ntp.SetGmtOffset(1);            // offset UTC in ore (es. 1 per CET)
Ntp.SetUpdateInterval(60000);   // intervallo di aggiornamento in ms (default 60s)
Ntp.SetConnectionMaxTime(10000);

Ntp.SetOnSyncCallback([]() {
    LOG(INFO, "Main", "Ora sincronizzata: " + Ntp.GetFormattedTime());
});
Ntp.SetOnDesyncCallback([]() {
    LOG(WARNING, "Main", "NTP perso");
});
```

`Enable()` va chiamato **dopo** che il WiFi è connesso, tipicamente dalla callback `OnConnected` di `WifiHandler`:

```cpp
Wifi.SetOnConnectedCallback([]() {
    Ntp.Enable();
});
Wifi.SetOnDisconnectedCallback([]() {
    Ntp.Disable();
});
```

## Wiring con DMPOScheduler

**Attenzione: `Loop()` può bloccare fino a ~1 s.** La `forceUpdate()` di NTPClient aspetta la risposta UDP con un ciclo `delay(10)` fino a 1000 ms; succede alla prima sincronizzazione e a ogni re-sync periodico (`SetUpdateInterval`, default 60 s). Il `delay` cede la CPU (nessun rischio watchdog), ma il task che la ospita si ferma per quel tempo: se nel task ci sono funzioni sensibili alle deadline, ospitare `Ntp.Loop()` in un task con deadline tollerante (es. quello aperiodico).

```cpp
DMPOScheduler::TaskConfig NtpTask;
NtpTask.Name        = "Ntp";
NtpTask.PeriodUs    = 100000;     // 100ms
NtpTask.AppCritical = false;
NtpTask.StackSize   = 4096;
NtpTask.DeadlineUs  = 60000000;   // deadline tollerante: Loop() può bloccare ~1 s
Scheduler.AddTask(NtpTask);
Scheduler.AddFunction(NtpTask.ID, []() { Ntp.Loop(); });

Ntp.SetClockTime(100);   // ms — deve corrispondere a PeriodUs / 1000
```

## Lettura orario

I getter sono **non bloccanti**: `getEpochTime()` di NTPClient calcola l'ora come epoch dell'ultima sincronizzazione + tempo trascorso sul clock interno (`millis()`), senza alcun accesso alla rete. Possono essere chiamati liberamente anche da task veloci.

```cpp
String Time   = Ntp.GetFormattedTime("%H:%M:%S");
String DT     = Ntp.GetFormattedTime("%d/%m/%Y %H:%M:%S");
unsigned long Epoch = Ntp.GetEpochTime();
bool Synced   = Ntp.IsConnected();
```

## Utilizzo come DateTimeProvider

```cpp
Logger.SetDateTimeProvider(&Ntp);
```

## Dipendenze

- `NTPClient`
- `WiFiUdp`
- `System`
- `DateTimeProvider`
- WiFi connesso
