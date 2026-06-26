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

// Scaling: ADC → tensione → unità ingegneristiche
Pressure.SetScaling(
    3.3f,    // tensione di riferimento (V)
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
float ADC     = Pressure.GetADCValue();    // valore grezzo 0–4095
float Voltage = Pressure.GetVoltage();     // tensione in V
float Value   = Pressure.GetValue();       // unità ingegneristiche
```

## Dipendenze

- `TimeDiscreteFilter` (usato internamente)
- `LoggerHandler`
- `driver/adc.h` (ESP-IDF)

## Pin ADC validi per chip

`SetGPIO()` accetta solo i pin ADC del SoC; un pin non valido viene rifiutato con LOG di errore.

| SoC | ADC1 | ADC2 |
|-----|------|------|
| ESP32-S3 | GPIO 1–10 | GPIO 11–20 |
| ESP32-C3 | GPIO 0–4  | GPIO 5     |

## Note

- **ADC2 è condiviso con il WiFi**: con WiFi attivo le letture su ADC2 possono fallire con `ESP_ERR_TIMEOUT` (loggato come "Wi-Fi conflict"). Preferire i pin ADC1.
- La risoluzione ADC è 12 bit (0–4095); attenuazione 12 dB (fondoscala ~3.3 V).
- Driver: legacy `driver/adc.h` (ESP-IDF 4.4). Nessuna calibrazione hardware: la tensione è stimata in modo lineare via `SetScaling`.
