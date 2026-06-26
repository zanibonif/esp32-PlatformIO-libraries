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

## Log su file (LittleFS)

Oltre a Serial/WebSerial, il Logger può scrivere le righe formattate su file in LittleFS, con **rotazione a 2 file** e buffer in RAM per contenere l'usura della flash.

Il filesystem si assume **già montato esternamente** (es. `LittleFSHandler` / `FileSystem.Init()`): il Logger **non** chiama `LittleFS.begin()`. La cartella del path (es. `/log`) viene creata se manca.

```cpp
// dopo che LittleFS è montato:
Logger.SetClockTime(APERIODIC_TASK_PERIOD / MILLISECONDS_TO_MICROSECONDS); // periodo della Loop()
Logger.EnableFileLog();
```

`SetClockTime` serve al timer di scrittura periodica (il Logger non usa `millis()`): va impostato al periodo del task che chiama `Loop()`.

### API

```cpp
Logger.EnableFileLog();    // abilita (crea la cartella, recupera la dimensione di file0)
Logger.DisableFileLog();   // svuota il buffer e disabilita
Logger.WriteLogFile();     // forza la scrittura immediata del buffer

// lettura: callback riga per riga (niente String giganti)
Logger.ReadFullLog([](const char* Line) { Serial.println(Line); });   // storico + corrente
Logger.ReadLogFile(0, OnLine);   // 0 = corrente, 1 = storico
Logger.ClearLogFiles();          // cancella entrambi i file
```

### Quando viene scritto il buffer

- il buffer è pieno, **oppure**
- è trascorso `LOGGER_FILE_WRITE_INTERVAL_MS` con dati in pancia, **oppure**
- è arrivato un `ERROR`/`FATAL` (scrittura immediata: un crash non perde le ultime righe).

### Rotazione

Quando `file0` raggiunge `LOGGER_FILE_MAX_SIZE`: `file1` viene cancellato, `file0 → file1`, si riparte con un `file0` vuoto. Il log completo è `file1` (storico) seguito da `file0` (corrente): `ReadFullLog` li concatena in quest'ordine.

### Macro configurabili

| Macro | Default | Note |
|---|---|---|
| `LOGGER_FILE_PATH_0` | `/log/log0.txt` | file corrente |
| `LOGGER_FILE_PATH_1` | `/log/log1.txt` | storico |
| `LOGGER_FILE_BUFFER_SIZE` | 4096 | buffer RAM prima della scrittura |
| `LOGGER_FILE_MAX_SIZE` | 100000 | dimensione max per file (→ ~2× totale) |
| `LOGGER_FILE_WRITE_INTERVAL_MS` | 30000 | scrittura periodica |

> Le operazioni su file sono protette da un mutex interno (`_FileLogSemaphore`): una lettura (`ReadFullLog`) può convivere con le scritture della `Loop()`. Sotto logging molto fitto, durante una lettura lunga qualche riga può andare persa — segnalata con un marcatore nel file e una riga su seriale.

## Note

- Queue di 100 elementi (configurabile con `LOGGER_QUEUE_SIZE`). I messaggi in eccesso vengono contati e loggati come warning al prossimo ciclo.
- Lunghezza massima nome modulo: 64 caratteri (`LOGGER_FUNCTION_NAME_MAX_LEN`).
- Lunghezza massima messaggio: 192 caratteri (`LOGGER_MESSAGE_MAX_LEN`). I messaggi più lunghi vengono troncati con `...`.
- WebSerial usa un mutex interno per accesso thread-safe.

## Output su USB-CDC nativa (ESP32-S3 / C3)

Quando `Serial` è la USB-CDC nativa (`ARDUINO_USB_CDC_ON_BOOT=1`, tipico di S3/C3 senza convertitore UART), `SetSerialSpeed()` configura automaticamente la USB per ridurre la perdita di byte:

- `Serial.setTxBufferSize(LOGGER_USB_TX_BUFFER_SIZE)` prima di `begin()` — ring buffer TX più capiente.
- `Serial.setTxTimeoutMs(LOGGER_USB_TX_TIMEOUT_MS)` dopo `begin()` — la write attende che l'host legga invece di scartare i byte (blocca solo se un host è connesso).

Entrambi sono guardati da `#if ARDUINO_USB_CDC_ON_BOOT`: sulle build con UART (es. board `esp32dev`) le chiamate non vengono compilate e il comportamento resta invariato.

| Macro | Default | Override |
|---|---|---|
| `LOGGER_USB_TX_BUFFER_SIZE` | 2048 byte | `-D LOGGER_USB_TX_BUFFER_SIZE=...` |
| `LOGGER_USB_TX_TIMEOUT_MS`  | 100 ms   | `-D LOGGER_USB_TX_TIMEOUT_MS=...` |

> Nota: questi accorgimenti mitigano ma non eliminano del tutto la perdita di byte della USB-Serial-JTAG sotto **streaming continuo ad alta frequenza**. Per log molto fitti preferire un rate più basso o il canale WebSerial.

## Macro LOG e inizializzazione globale (SIOF)

Il macro `LOG` usa `LoggerHandler::GetInstance()` — non il riferimento alias `Logger` — per evitare il **Static Initialization Order Fiasco**.

In C++, i costruttori di variabili globali vengono eseguiti prima di `setup()` in ordine non deterministico tra translation unit. Se un'altra libreria (es. `WifiHandler`) chiama `LOG` nel proprio costruttore, e il suo `.cpp` viene linkato prima di `LoggerHandler.cpp`, il riferimento `Logger` sarebbe ancora null → crash `LoadProhibited` a `EXCVADDR = 0x0000000e`.

Usando `GetInstance()` direttamente nel macro, il Logger viene creato on-demand alla prima chiamata, indipendentemente dall'ordine di inizializzazione. Non usare mai `Logger.Log(...)` direttamente nei costruttori di variabili globali.
