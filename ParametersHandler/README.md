# ParametersHandler

Gestione parametri persistenti su file CSV in LittleFS. Supporta più file, tipi numerici/stringa/bool, cifratura e scrittura lazy con delay configurabile.

## Alias globale

```cpp
extern ParametersHandler& Parameters;
```

## Concetti chiave

- **`ParamId<T>`** — token tipato che identifica un parametro. Il tipo `T` è dedotto automaticamente in `Get`/`Set`: non va mai ripetuto nel codice applicativo.
- **`FileConfig`** — struct che associa un percorso a un file CSV. Ogni parametro è registrato su un file specifico.
- **Write delay** — ogni `Set()` non scrive immediatamente ma avvia un timer per file. La scrittura avviene dopo N ms di inattività; `Loop()` gestisce il countdown.

## Definizione dei parametri

Convenzione: raccogliere i token in un header separato con ID fissi e stabili.

```cpp
// AppParameters.h
#pragma once
#include <ParametersHandler.h>

#define CONFIGURATIONS_FILE_PATH "/data/configurations.csv"
#define RETAIN_FILE_PATH         "/data/retain.csv"

// ATTENZIONE: gli ID sono la chiave nel CSV — non vanno mai modificati dopo il deploy
constexpr ParamId<String>        WIFI_SSID     { 10 };
constexpr ParamId<String>        WIFI_PASSWORD { 11 };
constexpr ParamId<int>           GMT_OFFSET    { 12 };
constexpr ParamId<unsigned long> BOOT_COUNTER  {  1 };
```

Gli ID sono in uno spazio globale unico: non possono ripetersi tra file diversi.

## Setup

```cpp
// Prima di Begin() — ordine: SetClockTime, AddFile, AddParameter, Begin
Parameters.SetClockTime(LOW_RATE_TASK_PERIOD / MILLISECONDS_TO_MICROSECONDS);
Parameters.SetWriteDelay(500);  // ms prima di scrivere dopo un Set() (default: 500)

ParametersHandler::FileConfig ConfigFile;
ConfigFile.Path = CONFIGURATIONS_FILE_PATH;
Parameters.AddFile(ConfigFile);
Parameters.AddParameter(WIFI_SSID,     "WiFi SSID",     "default_ssid", ConfigFile);
Parameters.AddParameter(WIFI_PASSWORD, "WiFi Password", "default_pass", ConfigFile, /*Encrypted=*/true);
Parameters.AddParameter(GMT_OFFSET,    "GMT offset",    2,              ConfigFile);

ParametersHandler::FileConfig RetainFile;
RetainFile.Path = RETAIN_FILE_PATH;
Parameters.AddFile(RetainFile);
Parameters.AddParameter(BOOT_COUNTER, "Boot counter", 0UL, RetainFile);

Parameters.Begin();
```

### Comportamento di Begin()

- Se il file non esiste, lo crea con i valori default.
- Se esiste ma manca un parametro (es. aggiunto dopo il primo deploy), logga un `WARNING` e usa il default.
- Al termine salva tutti i file, così i parametri nuovi o modificati sono subito persistiti.

## Get / Set

```cpp
String Ssid = Parameters.Get(WIFI_SSID);   // tipo dedotto: String
int    Gmt  = Parameters.Get(GMT_OFFSET);  // tipo dedotto: int

Parameters.Set(WIFI_SSID, "NuovaRete");
Parameters.Set(GMT_OFFSET, 1);
```

Il tipo non va mai esplicitato nel chiamante — è dedotto dal token `ParamId<T>`. Se il tipo di un parametro cambia, si aggiorna solo la definizione del token.

## Write delay e Loop()

Ogni `Set()` avvia (o resetta) il timer del file corrispondente. La scrittura avviene quando il timer scade. Agganciare `Loop()` al task lento:

```cpp
Scheduler.AddFunction(LowRateTask, []() { Parameters.Loop(); });
```

Per forzare la scrittura immediata (es. prima di reboot o OTA):

```cpp
Parameters.ForceWrite();
```

## ID parametri

L'ID è la chiave primaria nel CSV. Regole:

- Non va mai cambiato dopo il primo deploy — cambiare un ID equivale a perdere il valore persistito.
- Non va riutilizzato, nemmeno dopo aver rimosso un parametro.
- Gli ID sono globali: non possono ripetersi tra file diversi dello stesso `ParametersHandler`.

## Cifratura

```cpp
Parameters.AddParameter(WIFI_PASSWORD, "Password", "", ConfigFile, /*Encrypted=*/true);
```

Il valore è cifrato nel file con XOR + chiave `PARAMETERS_ENCRYPT_KEY` (configurabile via `#define`, default `"ESP32Key"`). I valori cifrati hanno il prefisso `PARAMETERS_ENCRYPT_PREFIX` (`"ENC:"`).

## Formato CSV

```
ID,Nome,Tipo,Valore
1,Boot counter,ulong,42
10,WiFi SSID,string,Vodafone
11,WiFi Password,string,ENC:4a3f8c...
12,GMT offset,int,2
```

Tipi supportati: `int`, `uint`, `long`, `ulong`, `float`, `bool`, `string`.

## Controllo runtime

```cpp
// Valore grezzo come salvato su file (ENC:... se cifrato)
String Raw = Parameters.GetRaw(WIFI_SSID);
```

## Dipendenze

- `LoggerHandler`
- `LittleFSHandler`
