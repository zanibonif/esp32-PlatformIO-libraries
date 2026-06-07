# ESP32 Libraries — Contesto di progetto

## Panoramica

Raccolta di librerie C++ per ESP32 (Arduino/IDF). Ogni libreria sta in una sottocartella con `.h`, `.cpp` (opzionale) e `library.json`.

Il nucleo del sistema è il **DMPOScheduler**: uno scheduler FreeRTOS a cui vengono agganciati i metodi ciclici di ogni libreria. Tutto il lifecycle dei task deve passare per lui.

---

## Stato librerie

| Libreria | Stato |
|---|---|
| DMPOScheduler | ✅ omogenizzato |
| LoggerHandler | ✅ omogenizzato |
| System | ✅ omogenizzato |
| WebServerHandler | ✅ omogenizzato |
| DateTimeProvider | ✅ omogenizzato |
| NtpHandler | ✅ omogenizzato |
| WifiHandler | ✅ omogenizzato |
| DigitalSignalHandler | ✅ omogenizzato |
| DS3231_RtcHandler | ✅ omogenizzato |
| AnalogInputHandler | ✅ omogenizzato |
| CommandOtaHandler | ✅ omogenizzato |
| MQTTClient | ✅ omogenizzato |
| LittleFSHandler | Singleton ref OK, manca extern alias, header-only |
| TimeDiscreteFilter | ✅ omogenizzato |

---

## Convenzioni di codice

### Naming

- **PascalCase** per tutto: classi, metodi, parametri, variabili locali
- **`_` prefix** per membri privati: `_Enabled`, `_ClockTime`, `_LogQueue`
- **`_` prefix** per metodi privati: `_FormatLog`, `_PublishLog`
- **SCREAMING_SNAKE_CASE** per costanti `#define`: `LOGGER_QUEUE_SIZE`
- **SCREAMING_SNAKE_CASE** per valori enum: `NOT_CONNECTED`, `CONNECTION_IN_PROGRESS`; il tipo enum in PascalCase: `NtpState`
- **`#pragma once`** come include guard (mai `#ifndef/#define/#endif`)
- **`nullptr`** sempre, mai `NULL`
- **Allineamento membri privati**: tipo paddato alla larghezza del tipo più largo, `=` allineati alla stessa colonna
- **Spazio prima di `(`** nelle dichiarazioni e definizioni di funzioni (non nelle chiamate): `void SetPort (uint16_t Port);` / `void WebServerHandler::SetPort (uint16_t Port) {`

### Pattern Singleton

Meyer's singleton con extern alias. Sempre questo pattern, nessun altro:

```cpp
// .h
class MyClass {
public:
    static MyClass& GetInstance();
    MyClass(const MyClass&)            = delete;
    MyClass& operator=(const MyClass&) = delete;
private:
    MyClass();
};
extern MyClass& MyAlias;

// .cpp
MyClass& MyClass::GetInstance() {
    static MyClass Instance;
    return Instance;
}
MyClass& MyAlias = MyClass::GetInstance();
```

- Metodo sempre chiamato `GetInstance()`
- Extern alias per uso comodo nel codice applicativo (`Scheduler`, `Logger`, ecc.)
- NO singleton a puntatore con `Destroy()`

### Pattern Timer (no millis)

Mai usare `millis()`. I timer si implementano per sottrazione del `_ClockTime`:

```cpp
if (Timer > _ClockTime) {
    Timer = Timer - _ClockTime;
} else {
    Timer = ZERO_TIME;
}
bool Timeout = (Timer == ZERO_TIME);
```

Per aspettare N ms: `Timer = N`. Il `_ClockTime` è il periodo con cui `Loop()` viene chiamato dal DMPOScheduler.

### Architettura: Loop() vs task interni

Le librerie **non** creano task FreeRTOS in autonomia. Espongono:
- `Loop()` per logiche cicliche
- `Update(valore)` per aggiornamenti event-driven

Questi metodi vengono agganciati al DMPOScheduler dal codice applicativo.

### Multi-target con booleani indipendenti

Per funzionalità abilitate/disabilitate in modo indipendente, usare booleani separati con metodi `Enable/Disable` per ciascuno. Mai enum che codificano combinazioni.

```cpp
// NO
enum class Target { SerialOnly, WebSerialOnly, Both };

// SI
bool _SerialEnabled    = true;
bool _WebSerialEnabled = false;
void EnableSerial();    void DisableSerial();
void EnableWebSerial(); void DisableWebSerial();
```

### Struttura file .h

```
#pragma once
// include

// define configurabili

// enum / struct pubblici

class MyClass {
public:
    // Singleton
    // Configurazione (setup)
    // Controllo runtime
    // Metodi principali
    // Chiamato ciclicamente (Loop / Update)

private:
    // Metodi privati
    // Membri privati
};

// extern alias (se singleton)
// macro di convenienza (se presenti)
```

### Struttura file .cpp

I metodi nel `.cpp` seguono lo stesso ordine delle sezioni del `.h`, con commenti di sezione:

```cpp
// --- Configurazione ---
// --- Controllo runtime ---
// --- Internals ---
```

---

## Librerie esterne utilizzate

- FreeRTOS (incluso in ESP-IDF)
- ESPAsyncWebServer
- WebSerial
- RTClib (per DS3231)
- NTPClient
- PubSubClient + ArduinoJson (per MQTT)
- ArduinoOTA
- LittleFS
