# I2cBusHandler

Possiede il bus I2C come risorsa condivisa: porta, pin, frequenza, inizializzazione differita e mutex. Offre il sondaggio di un indirizzo (`Probe()`) e la scansione dell'intero bus (`Scan()`).

**Non gestisce la comunicazione**: read/write restano ai driver dei singoli dispositivi (RTClib, ecc.), che serializzano le sequenze indivisibili con `TakeBus()`/`GiveBus()`.

**Non tiene un registro dei dispositivi e non ne monitora la presenza.** Dalla 2.0.0 quel compito è del driver: è lui che sa se le proprie transazioni riescono, e può usare `Probe()` finché crede il proprio dispositivo assente. Vedi `DS3231_RtcHandler` per il pattern di riferimento.

`Enable()`/`Disable()` sono flag-only: `Wire.begin()` sui pin configurati avviene nella `Loop()` al primo ciclo da abilitato. `I2cBusHandler` resta l'**unico punto del sistema** che inizializza il bus.

## Perché la presenza è stata spostata nei driver

Dall'ESP-IDF 5.4 il core Arduino compila `esp32-hal-i2c-ng.c`, costruito sul driver `i2c_master`. Lì le due operazioni prendono strade diverse:

| Operazione | API IDF | Oggetto |
|---|---|---|
| scrittura a zero byte (il probe) | `i2c_master_probe()` | handle del **bus** |
| ogni altra transazione | `i2c_master_transmit[_receive]()` | handle di **dispositivo**, uno per indirizzo |

Un probe periodico sullo stesso indirizzo che un driver sta usando significa alternare le due API molte volte al secondo. Con la divisione dei compiti attuale questo non può più accadere: **chi sonda non transa, e chi transa non sonda**.

## Alias globale

```cpp
extern I2cBusHandler& I2cBus;
```

## Setup

```cpp
I2cBus.SetSdaPin(21);
I2cBus.SetSclPin(22);
I2cBus.SetFrequency(100000);   // opzionale, default 100 kHz
I2cBus.Enable();               // flag-only: Wire.begin() avviene alla prima Loop()
```

Se i pin non sono configurati, `Loop()` non gira e logga una sola volta.

## Wiring con DMPOScheduler

La `Loop()` fa solo l'inizializzazione differita, quindi è leggerissima, ma va comunque nel task **low-rate** insieme ai driver che usano il bus — e **prima** di loro, così alla prima iterazione il bus è pronto.

```cpp
Scheduler.AddFunction(LowRateTaskConfiguration, []() { I2cBus.Loop(); });
Scheduler.AddFunction(LowRateTaskConfiguration, []() { Rtc.Loop(); });
```

## Uso dai driver dei dispositivi

```cpp
if (I2cBus.TakeBus()) {        // mutex di bus, timeout default 50 ms
    DateTime Now = _Rtc.now(); // transazione I2C
    I2cBus.GiveBus();
}
```

`Wire` ha già un lock proprio che copre sia la singola transazione sia la sequenza non-stop (`endTransmission(false)` lo tiene fino alla `requestFrom`). `TakeBus()`/`GiveBus()` serve per raggruppare operazioni **logicamente** indivisibili fatte di chiamate `Wire` separate, e per tenere fuori la scansione mentre un driver sta lavorando.

Per sapere se il proprio dispositivo c'è, quando il driver lo crede assente:

```cpp
bool Present = I2cBus.Probe(0x68);   // prende il mutex da sola
```

Nota: il probe dice che il dispositivo *c'è*, non che i dati sono validi — la validazione resta responsabilità del driver.

## Diagnostica

```cpp
I2cBus.GetSdaPin();      // pin e frequenza effettivi, non quelli che credi di aver impostato
I2cBus.GetSclPin();
I2cBus.GetFrequency();   // Hz
I2cBus.IsReady();        // abilitato e bus inizializzato
```

Scansione dal vivo dell'intero bus:

```cpp
uint8_t FoundCount = 0;
bool    Done       = I2cBus.Scan([](uint8_t Address) {
    Serial.println("0x" + String(Address, HEX));
}, &FoundCount);

if      (!Done)           Serial.println("bus non pronto o occupato");
else if (FoundCount == 0) Serial.println("nessun dispositivo trovato");
```

`Scan()` sonda da `I2C_BUS_HANDLER_FIRST_SCAN_ADDRESS` (0x08) a `I2C_BUS_HANDLER_LAST_SCAN_ADDRESS` (0x77) — sotto e sopra sono riservati dalla specifica I2C. Tiene il mutex **per tutta la spazzata**, così nessun driver si infila a metà rendendo incoerente la fotografia. È un comando manuale: non va messo in un task ciclico.

`FoundCount` è opzionale ma quasi sempre serve: il callback è un puntatore a funzione, quindi non può catturare un contatore del chiamante.

## Controllo runtime

```cpp
I2cBus.Enable();
I2cBus.Disable();   // il bus resta inizializzato
```

## Limiti

- Una istanza, un bus: gestisce `Wire` (o il `TwoWire` passato con `SetWirePort()`); il secondo controller dell'ESP32 (`Wire1`) richiederebbe un'evoluzione della libreria.
- Nessun limite al numero di dispositivi, perché non li conosce.

## Migrazione dalla 1.x

Rimossi `AddDevice()`, `IsAvailable()`, `SetAvailableCallback()`, `SetUnavailableCallback()`, `SetAvailableDelay()`, `SetUnavailableDelay()` e `SetClockTime()`. Un driver che li usava deve ora tenersi un `DigitalSignalHandler` proprio, alimentarlo con l'esito delle proprie transazioni e sondare con `Probe()` quando crede il dispositivo assente. L'API vista dall'applicazione non cambia, se il driver continua a esporre il proprio `IsAvailable()`.

## Dipendenze

- `Wire`
- `LoggerHandler`
- FreeRTOS (mutex)
