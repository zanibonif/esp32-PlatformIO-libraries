# LoggerHandler

Logger asincrono per ESP32 con output su Serial e/o WebSerial. I messaggi vengono messi in una FreeRTOS queue e pubblicati nel `Loop()` — nessun output bloccante nel codice applicativo.

## Alias globale

```cpp
extern LoggerHandler& Logger;
```

Usare direttamente la macro `LOG` (definita nell'header):

```cpp
LOG(INFO,    "NomeModulo", "messaggio");
LOG(WARNING, "NomeModulo", "valore: " + String(Val));
LOG(ERROR,   "NomeModulo", "errore critico");
LOG(DEBUG,   "NomeModulo", "dettaglio");
LOG(FATAL_ERROR, "NomeModulo", "sistema instabile");
```

Da ISR usare `LOG_ISR` (non alloca String):

```cpp
LOG_ISR(WARNING, "NomeModulo", "chiamato da ISR");
```

## Dipendenze

- `DateTimeProvider` — opzionale, aggiunge timestamp al formato log
- `ESPAsyncWebServer` — opzionale, necessario solo per WebSerial
- `WebSerial` — opzionale

## Setup minimo

```cpp
Logger.SetSerialSpeed(115200);
// opzionale: Logger.SetDateTimeProvider(&DateTimeProvider::GetInstance());
```

## Setup con WebSerial

```cpp
Logger.SetSerialSpeed(115200);
Logger.SetWebServer(WebServer.GetServer());
Logger.EnableWebSerial();

// quando il server viene avviato:
Logger.SetWebServerRunning();

// quando il server viene fermato:
Logger.SetWebServerNotRunning();
```

## Wiring con DMPOScheduler

`Loop()` preleva dalla queue e pubblica. Va agganciato a un task periodico:

```cpp
DMPOScheduler::TaskConfig LogTask;
LogTask.Name        = "Logger";
LogTask.PeriodUs    = 50000;   // 50ms
LogTask.AppCritical = false;
LogTask.StackSize   = 4096;
Scheduler.AddTask(LogTask);
Scheduler.AddFunction(LogTask, []() { Logger.Loop(); });
```

## Controllo runtime

```cpp
Logger.Enable();            // abilita tutto (default)
Logger.Disable();           // disabilita tutto
Logger.EnableSerial();      // solo Serial (default on)
Logger.DisableSerial();
Logger.EnableWebSerial();   // solo WebSerial (default off)
Logger.DisableWebSerial();
Logger.SetMaxMessagesPerCycle(50);  // messaggi per tick (default 50)
```

## Formato output

```
dd/mm/yyyy HH:MM:SS | LEVEL    | NomeModulo: messaggio
```

Senza DateTimeProvider:

```
LEVEL    | NomeModulo: messaggio
```

## Note

- Queue di 100 elementi (configurabile con `LOGGER_QUEUE_SIZE`). I messaggi in eccesso vengono contati e loggati come warning al prossimo ciclo.
- Lunghezza massima nome modulo: 64 caratteri (`LOGGER_FUNCTION_NAME_MAX_LEN`).
- Lunghezza massima messaggio: 192 caratteri (`LOGGER_MESSAGE_MAX_LEN`). I messaggi più lunghi vengono troncati con `...`.
- WebSerial usa un mutex interno per accesso thread-safe.

## Macro LOG e inizializzazione globale (SIOF)

Il macro `LOG` usa `LoggerHandler::GetInstance()` — non il riferimento alias `Logger` — per evitare il **Static Initialization Order Fiasco**.

In C++, i costruttori di variabili globali vengono eseguiti prima di `setup()` in ordine non deterministico tra translation unit. Se un'altra libreria (es. `WifiHandler`) chiama `LOG` nel proprio costruttore, e il suo `.cpp` viene linkato prima di `LoggerHandler.cpp`, il riferimento `Logger` sarebbe ancora null → crash `LoadProhibited` a `EXCVADDR = 0x0000000e`.

Usando `GetInstance()` direttamente nel macro, il Logger viene creato on-demand alla prima chiamata, indipendentemente dall'ordine di inizializzazione. Non usare mai `Logger.Log(...)` direttamente nei costruttori di variabili globali.
