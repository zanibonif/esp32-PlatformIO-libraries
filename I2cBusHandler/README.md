# I2cBusHandler

Gestione del bus I2C come risorsa condivisa: inizializzazione (pin, frequenza), mutex di bus e monitoraggio della disponibilità dei dispositivi registrati tramite probe periodico (`beginTransmission` + `endTransmission`).

**Non gestisce la comunicazione**: read/write restano a carico dei driver dei singoli dispositivi (RTClib, ecc.), che però possono consultare `IsAvailable()` prima di transare — evitando letture spazzatura da dispositivi assenti — e serializzare l'accesso al bus con `TakeBus()`/`GiveBus()`.

Ogni dispositivo registrato ha un `DigitalSignalHandler` interno: la disponibilità è filtrata con un ritardo di attivazione (default **250 ms**, ovvero qualche ciclo di probe stabile prima di dichiarare il dispositivo di nuovo disponibile) ed espone callback sui fronti di connessione/disconnessione.

`Enable()`/`Disable()` sono flag-only: l'inizializzazione del bus (`Wire.begin()` sui pin configurati) avviene nella `Loop()` al primo ciclo da abilitato. `I2cBusHandler` deve restare l'**unico punto del sistema** che inizializza il bus — nessun'altra libreria o codice applicativo deve chiamare `Wire.begin()`.

## Alias globale

```cpp
extern I2cBusHandler& I2cBus;
```

## Setup

```cpp
I2cBus.SetSdaPin(21);
I2cBus.SetSclPin(22);
I2cBus.SetFrequency(100000);   // opzionale, default 100 kHz
I2cBus.SetClockTime(100);      // ms — periodo con cui Loop() viene chiamata

I2cBus.AddDevice(0x68, "DS3231");
I2cBus.SetAvailableDelay(0x68, 250);     // ms, opzionale (default 250)
I2cBus.SetUnavailableDelay(0x68, 0);     // ms, opzionale (default 0)
I2cBus.SetAvailableCallback(0x68, []() {
    LOG(INFO, "Main", "DS3231 disponibile");
});
I2cBus.SetUnavailableCallback(0x68, []() {
    LOG(WARNING, "Main", "DS3231 non disponibile");
});

I2cBus.Enable();   // flag-only: Wire.begin() avviene alla prima Loop()
```

Se i pin non sono configurati, `Loop()` non gira e logga una sola volta.

## Wiring con DMPOScheduler

Il probe è veloce con dispositivo assente (NACK immediato) ma un bus fisicamente bloccato può causare timeout: la `Loop()` va nel task **low-rate**, mai in quello high-rate.

```cpp
Scheduler.AddFunction(LowRateTaskConfiguration.ID, []() { I2cBus.Loop(); });

I2cBus.SetClockTime(100);   // ms — deve corrispondere al PeriodUs del task / 1000
```

## Uso dai driver dei dispositivi

```cpp
if (I2cBus.IsAvailable(0x68)) {
    if (I2cBus.TakeBus()) {        // mutex di bus, timeout default 50 ms
        DateTime Now = _Rtc.now(); // transazione I2C
        I2cBus.GiveBus();
    }
}
```

Un indirizzo non registrato con `AddDevice()` risulta sempre non disponibile.

Nota: il probe dice che il dispositivo *c'è*, non che i dati sono validi — la validazione dei dati resta responsabilità del driver (le due protezioni sono complementari).

## Controllo runtime

```cpp
I2cBus.Enable();
I2cBus.Disable();   // sospende il monitoraggio; il bus resta inizializzato
```

A bus disabilitato i segnali di disponibilità restano congelati all'ultimo stato noto (`IsAvailable()` compreso).

## Limiti

- Fino a `I2C_BUS_HANDLER_MAX_DEVICES` (8) dispositivi registrabili.
- Una istanza, un bus: gestisce `Wire` (o il `TwoWire` passato con `SetWirePort()`); il secondo controller dell'ESP32 (`Wire1`) richiederebbe un'evoluzione della libreria.

## Dipendenze

- `Wire`
- `DigitalSignalHandler`
- `LoggerHandler`
- FreeRTOS (mutex)
