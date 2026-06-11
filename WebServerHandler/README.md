# WebServerHandler

Wrapper singleton per `AsyncWebServer`. Gestisce il ciclo di vita del server e fornisce il puntatore al server per aggiungere route dall'esterno.

## Alias globale

```cpp
extern WebServerHandler& WebServer;
```

## Setup

```cpp
WebServer.SetPort(80);   // default 80, chiamare prima di Start()
WebServer.Start();
```

## Aggiungere route

```cpp
WebServer.GetServer()->on("/api/status", HTTP_GET, [](AsyncWebServerRequest* Request) {
    Request->send(200, "application/json", "{\"ok\":true}");
});
```

`GetServer()` restituisce `AsyncWebServer*`. Tutte le route vanno aggiunte **prima** di `Start()`, oppure dinamicamente (AsyncWebServer lo supporta).

## Controllo runtime

```cpp
WebServer.Start();
WebServer.Stop();
bool Running = WebServer.IsRunning();
```

## Integrazione con LoggerHandler

Dopo `Start()`, notificare il Logger:

```cpp
Logger.SetWebServer(WebServer.GetServer());
Logger.EnableWebSerial();
Logger.SetWebServerRunning();
```

## Note

- Non ha `Loop()`: AsyncWebServer gestisce internamente le connessioni tramite il task `async_tcp` su Core 0 a priorità 3.
- Non va agganciato al DMPOScheduler.

## Dipendenze

- `ESPAsyncWebServer`
