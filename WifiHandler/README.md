# WifiHandler

Gestione connessione WiFi con state machine. Supporta modalità STA, AP fallback automatico e retry STA dall'AP. Richiede `Loop()` agganciato al DMPOScheduler.

## Alias globale

```cpp
extern WifiHandler& Wifi;
```

## Setup minimo (solo STA)

```cpp
Wifi.SetSSIDAndPassword("MyNetwork", "password");
Wifi.SetOnConnectedCallback([]() {
    // avviare i servizi che dipendono dal WiFi: NTP, MQTT, WebServer...
});
Wifi.SetOnDisconnectedCallback([]() {
    // fermare i servizi WiFi-dipendenti
});
Wifi.Enable();
```

## Setup con AP fallback

Se la connessione STA fallisce, il dispositivo avvia un access point. Ogni `SetAPRetryInterval` ms ritenta la connessione STA in background.

```cpp
Wifi.SetAPCredentials("ESP32-Config", "admin1234");
Wifi.EnableAPFallback();
Wifi.SetAPRetryInterval(300000);   // ritenta ogni 5 minuti (default)
Wifi.SetOnAPStartedCallback([]() {
    // es. avviare il web server per la pagina di configurazione
});
```

## Wiring con DMPOScheduler

```cpp
DMPOScheduler::TaskConfig WifiTask;
WifiTask.Name        = "Wifi";
WifiTask.PeriodUs    = 100000;   // 100ms
WifiTask.AppCritical = false;
WifiTask.StackSize   = 4096;
Scheduler.AddTask(WifiTask);
Scheduler.AddFunction(WifiTask, []() { Wifi.Loop(); });
```

`SetClockTime` deve corrispondere al PeriodUs del task:

```cpp
Wifi.SetClockTime(100);   // ms — deve corrispondere a PeriodUs / 1000
```

## Cambio credenziali a runtime

```cpp
Wifi.SetSSIDAndPassword("NewNetwork", "newpassword");
// Se già connesso, la state machine avvia automaticamente la riconnessione
```

## Diagnostica

```cpp
bool Connected   = Wifi.IsConnected();
bool InAPMode    = Wifi.IsAPMode();
int  Signal      = Wifi.GetSignalStrength();   // 0–100
String IP        = Wifi.GetIPAddress();
String APIP      = Wifi.GetAPIPAddress();
```

## Controllo runtime

```cpp
Wifi.Enable();
Wifi.Disable();   // disconnette e blocca i tentativi di riconnessione
```

## Parametri configurabili

```cpp
Wifi.SetHostname("mio-dispositivo");
Wifi.SetPostConnectionDelay(5000);       // ms di attesa prima di segnalare CONNECTED (default 5s)
Wifi.SetConnectionMaxTime(20000);        // timeout connessione STA (default 20s)
Wifi.SetDisconnectionMaxTime(20000);     // timeout disconnessione (default 20s)
```

## Dipendenze

- `LoggerHandler`
- `System`
