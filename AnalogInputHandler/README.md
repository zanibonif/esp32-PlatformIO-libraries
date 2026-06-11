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
Pressure.SetGPIO(36);             // pin ADC (usa ADC1 — vedi nota WiFi)

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
Scheduler.AddFunction(ADCTask.ID, []() {
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

## Note

- **ADC2 è condiviso con il WiFi**: usare sempre pin ADC1 (GPIO 32–39) quando WiFi è attivo.
- La risoluzione ADC è 12 bit (0–4095).
