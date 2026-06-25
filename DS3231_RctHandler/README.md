# DS3231_RtcHandler

Lettura/scrittura RTC hardware DS3231 via I2C. Implementa `DateTimeProvider`.

La transazione I2C di lettura avviene **solo nella `Loop()`** (da agganciare a un task lento), che tiene in cache l'epoch dell'ultima lettura valida: `GetDateTime()`/`GetFormattedTime()` leggono la cache e **non sono mai bloccanti** per il chiamante — il Logger e i task veloci possono chiamarle liberamente senza toccare il bus.

Si appoggia a **`I2cBusHandler`** per inizializzazione del bus, mutex e disponibilità del modulo:

- si **auto-registra** sul bus alla costruzione (indirizzo 0x68) — il main non deve chiamare `AddDevice` per l'RTC; `Enable()`/`Disable()` sono flag-only;
- non esegue transazioni quando il modulo non è disponibile: con modulo scollegato i getter tornano un fallback invece di leggere spazzatura (e i timestamp del Logger restano puliti);
- l'inizializzazione del chip (`begin` + gestione `lostPower`) avviene nella `Loop()` alla prima disponibilità del modulo, quindi il modulo può essere collegato anche a sistema avviato;
- le letture sono validate con `DateTime::isValid()` (protezione complementare al probe di disponibilità).

## Alias globale

```cpp
extern DS3231_RtcHandler& Rtc;
```

## Setup

Richiede `I2cBusHandler` configurato e abilitato **prima**:

```cpp
I2cBus.SetSdaPin(21);
I2cBus.SetSclPin(22);
I2cBus.SetClockTime(100);   // ms — periodo del task che ospita I2cBus.Loop()
I2cBus.Enable();

Rtc.Enable();               // flag-only (la registrazione sul bus avviene alla costruzione)

// Impostare l'orario solo se necessario (es. primo avvio o batteria scarica):
Rtc.SetDateTime(2025, 1, 15, 10, 30, 0);   // Anno, Mese, Giorno, Ore, Min, Sec
```

## Wiring con DMPOScheduler

La `Loop()` va nello stesso task (lento) di `I2cBus.Loop()`, dopo di essa — così tutte le transazioni I2C vivono in un unico task:

```cpp
Scheduler.AddFunction(LowRateTaskConfiguration, []() { I2cBus.Loop(); });
Scheduler.AddFunction(LowRateTaskConfiguration, []() { Rtc.Loop(); });
```

La cache viene aggiornata a ogni ciclo (con task a 100 ms la risoluzione è ampiamente sufficiente per orologio con secondi).

## Lettura orario

```cpp
String DT      = Rtc.GetFormattedTime("%d/%m/%Y %H:%M:%S");
String TimeStr = Rtc.GetFormattedTime("%H:%M:%S");
DateTime Now   = Rtc.GetDateTime();   // oggetto RTClib::DateTime
```

Il formato segue la sintassi `strftime`.

Fallback con modulo assente/guasto: `GetFormattedTime()` torna `"RTC Disabled"` se disabilitato, stringa **vuota** se la cache non contiene una lettura valida (il Logger in tal caso stampa senza timestamp); `GetDateTime()` torna `DateTime(0)`. Se il modulo sparisce, la cache viene invalidata dalla `Loop()` al ciclo successivo.

## Utilizzo come DateTimeProvider

```cpp
Logger.SetDateTimeProvider(&Rtc);
```

## Controllo runtime

```cpp
Rtc.Enable();
Rtc.Disable();
bool Active    = Rtc.IsEnabled();
bool Connected = Rtc.IsAvailable();   // disponibilità sul bus (via I2cBus)
```

## Dipendenze

- `RTClib`
- `I2cBusHandler`
- `LoggerHandler`
- `DateTimeProvider`

## Note

- Il chip DS3231 mantiene l'orario anche senza alimentazione grazie alla batteria a bottone CR2032.
- L'indirizzo I2C è fisso (0x68, `DS3231_RTC_I2C_ADDRESS`) e non configurabile via software.
