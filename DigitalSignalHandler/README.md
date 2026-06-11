# DigitalSignalHandler

Filtro digitale per segnali booleani con ritardi di attivazione/disattivazione configurabili e callback su fronte. **Non è un singleton**: va istanziato una volta per ogni segnale.

## Istanziazione

```cpp
DigitalSignalHandler ButtonA;
DigitalSignalHandler LimitSwitch;
```

## Setup

```cpp
ButtonA.SetName("ButtonA");
ButtonA.SetClockTime(10);           // ms — periodo con cui Update() viene chiamato
ButtonA.SetActivationDelay(50);     // ms di segnale stabile alto prima di attivare
ButtonA.SetDeactivationDelay(20);   // ms di segnale stabile basso prima di disattivare
ButtonA.Enable();

ButtonA.SetActivationCallback([]() {
    LOG(INFO, "Main", "Pulsante premuto");
});
ButtonA.SetDeactivationCallback([]() {
    LOG(INFO, "Main", "Pulsante rilasciato");
});
```

## Wiring con DMPOScheduler

`Update()` riceve il valore raw del GPIO e guida la state machine interna:

```cpp
DMPOScheduler::TaskConfig IOTask;
IOTask.Name        = "IO";
IOTask.PeriodUs    = 10000;   // 10ms
IOTask.AppCritical = false;
IOTask.StackSize   = 2048;
Scheduler.AddTask(IOTask);
Scheduler.AddFunction(IOTask.ID, []() {
    ButtonA.Update(digitalRead(PIN_BUTTON_A));
    LimitSwitch.Update(digitalRead(PIN_LIMIT));
});
```

## Lettura segnale

```cpp
bool Raw      = ButtonA.GetSignal();         // valore raw passato a Update()
bool Filtered = ButtonA.GetFilteredSignal(); // valore con ritardi applicati
```

## Controllo runtime

```cpp
ButtonA.Enable();
ButtonA.Disable();
ButtonA.Reset();   // azzera lo stato interno
bool Active = ButtonA.IsEnabled();
```

## Dipendenze

- `LoggerHandler`
- `System`
