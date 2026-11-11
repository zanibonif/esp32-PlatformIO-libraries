# AnalogInputHandler

Lettura ingresso analogico ADC con scaling verso unità ingegneristiche e filtro digitale integrato. **Non è un singleton**: va istanziato una volta per ogni ingresso.

## Istanziazione

```cpp
AnalogInputHandler Pressure;
AnalogInputHandler Temperature;
```

## Setup

```cpp
Pressure.SetName("Pressure");
Pressure.SetClockTime(100);        // ms — periodo con cui Update() viene chiamato
Pressure.SetGPIO(1);              // pin ADC1 (vedi "Pin ADC validi per chip")

// Scaling: tensione calibrata → unità ingegneristiche
Pressure.SetScaling(
    0.5f,    // tensione minima del trasduttore (V)
    4.5f,    // tensione massima del trasduttore (V)
    0.0f,    // valore minimo in unità ingegneristiche (es. bar)
    10.0f    // valore massimo in unità ingegneristiche (es. bar)
);

// Saturazioni opzionali
Pressure.SetSaturations(0.0f, 10.0f);

// Filtro opzionale
Pressure.SetInputFilterTimeConstant(500);   // ms — filtro del primo ordine
```

## Wiring con DMPOScheduler

```cpp
DMPOScheduler::TaskConfig ADCTask;
ADCTask.Name        = "ADC";
ADCTask.PeriodUs    = 100000;   // 100ms
ADCTask.AppCritical = false;
ADCTask.StackSize   = 2048;
Scheduler.AddTask(ADCTask);
Scheduler.AddFunction(ADCTask, []() {
    Pressure.Update();
    Temperature.Update();
});
```

## Lettura valori

```cpp
float ADC        = Pressure.GetADCValue();    // conteggi grezzi dell'ultima lettura, 0–4095
float Voltage    = Pressure.GetVoltage();     // tensione calibrata e filtrata, in V
float Value      = Pressure.GetValue();       // unità ingegneristiche
bool  Calibrated = Pressure.IsCalibrated();   // false se è scattato il ripiego sulla scala nominale
```

## Dipendenze

- `TimeDiscreteFilter` (usato internamente)
- `LoggerHandler`
- **Arduino-ESP32 3.x** (`analogRead`, `analogReadMilliVolts`, `analogSetPinAttenuation`)

## Pin ADC validi

`SetGPIO()` accetta qualunque pin che il core riconosce come ingresso analogico
(`digitalPinToAnalogChannel()`), su qualunque SoC; un pin non valido viene rifiutato con LOG di
errore.

## Note

- **Calibrazione**: la tensione viene da `analogReadMilliVolts()`, che applica la calibrazione
  dell'esemplare scritta in eFuse (curve fitting su C3/S3, line fitting su ESP32/S2). Il filtro
  lavora sui millivolt calibrati.
- **Calibrazione assente**: tutti gli errori di `analogReadMilliVolts()` ritornano 0, che
  sembrerebbe un ingresso a massa. Se i conteggi grezzi superano 100 mentre i mV sono 0,
  l'ingresso passa alla scala nominale (conteggi / 4095 × 3,3 V), lo scrive a log una volta e
  `IsCalibrated()` ritorna false.
- Risoluzione 12 bit (0–4095), attenuazione 12 dB. Sul C3 la lettura è affidabile fino a circa
  2,5 V e oltre satura verso i 3 V: il fondo scala di un potenziometro va misurato, non assunto
  a 3,3 V.
- **ADC2 è condiviso con il WiFi**: preferire i pin ADC1.
- Il driver ADC legacy (`driver/adc.h`) non può convivere nello stesso firmware con quello che
  usa il core: questa libreria non va mescolata con codice che usa il driver legacy.
