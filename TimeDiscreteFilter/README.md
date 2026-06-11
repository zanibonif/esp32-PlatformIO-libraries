# TimeDiscreteFilter

Filtro digitale tempo-discreto. **Non è un singleton**: va istanziato una volta per ogni segnale da filtrare. Usato internamente da `AnalogInputHandler`; può essere usato direttamente per filtrare qualsiasi valore float/int.

## Istanziazione

```cpp
TimeDiscreteFilter SpeedFilter;
TimeDiscreteFilter TempFilter;
```

## Tipi di filtro

```cpp
NO_FILTER              // passthrough, nessuna elaborazione
FIRST_ORDER_FILTER     // filtro IIR del primo ordine (passa-basso esponenziale)
MOVING_AVERAGE_FILTER  // media mobile su N campioni
```

## Setup — filtro del primo ordine

```cpp
SpeedFilter.SetClockTime(10);                          // ms — periodo di campionamento
SpeedFilter.SetFilterType(FIRST_ORDER_FILTER);
SpeedFilter.SetFilterTimeConstant(200);               // costante di tempo in ms
```

La costante di tempo determina la velocità di risposta: valori alti → filtro più aggressivo (risposta più lenta).

## Setup — media mobile

```cpp
TempFilter.SetClockTime(100);
TempFilter.SetFilterType(MOVING_AVERAGE_FILTER);
TempFilter.SetSamplesNumber(10);   // numero di campioni nella finestra
```

## Utilizzo

```cpp
// Chiamare ogni tick (es. nel Loop() del task)
float Filtered = SpeedFilter.Filter(RawValue);   // accetta int o float
```

## Reset

```cpp
SpeedFilter.Reset();   // azzera lo stato interno (media, ultimo valore)
```

## Dipendenze

- `System`
