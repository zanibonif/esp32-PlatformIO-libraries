# ESP32 Libraries — Contesto di progetto

## Panoramica

Raccolta di librerie C++ per ESP32 (Arduino/IDF). Ogni libreria sta in una sottocartella con `.h`, `.cpp` (opzionale) e `library.json`.

Il nucleo del sistema è il **DMPOScheduler**: uno scheduler FreeRTOS a cui vengono agganciati i metodi ciclici di ogni libreria. Tutto il lifecycle dei task deve passare per lui.

---

## Stato librerie

| Libreria | Stato |
|---|---|
| DMPOScheduler | ✅ omogenizzato 📄 |
| LoggerHandler | ✅ omogenizzato 📄 |
| System | ✅ omogenizzato 📄 |
| WebServerHandler | ✅ omogenizzato 📄 |
| DateTimeProvider | ✅ omogenizzato 📄 |
| NtpHandler | ✅ omogenizzato 📄 |
| WifiHandler | ✅ omogenizzato 📄 — AP fallback integrato nella state machine |
| DigitalSignalHandler | ✅ omogenizzato 📄 |
| DS3231_RtcHandler | ✅ omogenizzato 📄 |
| I2cBusHandler | ✅ omogenizzato 📄 |
| AnalogInputHandler | ✅ omogenizzato 📄 |
| CommandOtaHandler | ✅ omogenizzato 📄 |
| MQTTClient | ✅ omogenizzato 📄 |
| LittleFSHandler | ✅ omogenizzato 📄 |
| TimeDiscreteFilter | ✅ omogenizzato 📄 |
| ParametersHandler | ✅ omogenizzato 📄 |
| WebFileManager | ✅ omogenizzato 📄 |

---

## Documentazione librerie

Ogni libreria ha un `README.md` nella propria cartella con alias globale, dipendenze, snippet di setup/wiring e controllo runtime. Le librerie documentate sono indicate con 📄 nella tabella sopra.

Per i pattern di interazione tra librerie (ordine di init, WiFi come driver, catena NTP→RTC→Logger, struttura task) vedere **[INTERACTIONS.md](INTERACTIONS.md)**.

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

### LOG nei costruttori e SIOF

Il macro `LOG` usa `LoggerHandler::GetInstance()` internamente. Chiamarlo nel costruttore di una libreria singleton è **sicuro e corretto**: il Logger viene creato on-demand alla prima chiamata, prima ancora che il costruttore termini.

Non usare mai `Logger.Log(...)` direttamente — il riferimento alias `Logger` può essere null durante `do_global_ctors` se la translation unit di `LoggerHandler.cpp` viene linkata dopo quella della libreria che chiama LOG.

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

### Enable/Disable idempotenti

`Enable()` esce subito se la libreria è già abilitata; `Disable()` esce subito se è già disabilitata. Niente log né side-effect quando lo stato non cambia. Evita doppie inizializzazioni hardware (es. doppio `Wire.begin()`), ri-scatti delle callback dei `DigitalSignalHandler` interni (la loro `Enable()` rimette `_Startup = true`) e log fuorvianti.

```cpp
void MyClass::Enable () {
    if (_Enabled) return;
    // ... inizializzazione e _Enabled = true
}

void MyClass::Disable () {
    if (!_Enabled) return;
    // ... _Enabled = false e spegnimento
}
```

### Enable/Disable leggeri (flag-only)

Oltre a essere idempotenti, `Enable()` e `Disable()` si limitano a settare il flag di stato (più eventuale LOG). Tutto il lavoro vero — inizializzazione hardware, registrazioni, transazioni, scritture GPIO — avviene nel `Loop()` (o nella loop lenta della libreria) al rilevamento del cambio di stato, o alla prima iterazione da abilitato. Motivo: Enable/Disable possono essere chiamate da callback o da task veloci e non devono mai bloccare né accedere all'hardware dal contesto del chiamante.

```cpp
void MyClass::Enable ()  { if (_Enabled)  return; _Enabled = true;  }
void MyClass::Disable () { if (!_Enabled) return; _Enabled = false; }

void MyClass::Loop () {
    if (_Enabled != _PreviousEnabled) {
        if (_Enabled) { /* inizializzazione / accensione */ }
        else          { /* spegnimento */ }
        _PreviousEnabled = _Enabled;
    }
    if (!_Enabled) return;
    // ... lavoro ciclico
}
```

Le operazioni di configurazione nei metodi `Set*` (pinMode, registrazioni, parametri) restano ammesse: girano nella fase di setup.

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

### Pattern registrazione con struct (DMPOScheduler, ConfigHandler)

Le librerie che richiedono registrazione pre-avvio usano una **struct di configurazione pubblica** il cui ID viene assegnato internamente dalla libreria. Il chiamante passa la struct (non l'ID) ai metodi successivi — l'ID è un dettaglio interno che il main non deve conoscere né manipolare direttamente.

```cpp
// SI
DMPOScheduler::TaskConfig MyTask;
MyTask.Name     = "MY_TASK";
MyTask.PeriodUs = 100000;
Scheduler.AddTask(MyTask);
Scheduler.AddFunction(MyTask, []() { MyLib.Loop(); });   // passa la struct, non MyTask.ID

// NO
Scheduler.AddFunction(MyTask.ID, []() { MyLib.Loop(); }); // espone un dettaglio interno
```

Regola: se un metodo deve riferirsi a un elemento già registrato, riceve la **stessa struct** usata per la registrazione. L'eccezione sono i metodi di controllo runtime (`EnableTask`, `DisableTask`) che operano per nome stringa o sono chiamati da contesti dove la struct non è disponibile.

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
