# DRV8874MotorHandler

Driver per motore DC pilotato dal **DRV8874** in **modo PH/EN**, organizzato come **macchina a stati**. Gestisce velocità con segno (direzione inclusa), soft-start / soft-cambio a rampa, lettura della corrente da IPROPI e protezione **anti-blocco a tempo**. Non è un singleton: istanzia un oggetto per ogni motore.

## I/O gestiti

- **Output**: `EN` (velocità, PWM LEDC), `PH` (direzione), `nSLEEP` (abilitazione/sleep, opzionale).
- **Input**: `nFAULT` (fault, opzionale), `IPROPI` (corrente, via `AnalogInputHandler` interno).

## Stati e transizioni

```
DRIVE_DISABLED   → drive spento (nSLEEP=0, uscite off)
DRIVE_ENABLED    → drive abilitato, fermo (nSLEEP=1, EN=0)
MOTOR_RUNNING    → in moto: la velocità applicata rampa verso il target
MOTOR_STOPPING   → rampa morbida verso 0 (poi → DRIVE_ENABLED da solo)
```

| Metodo | Da | A |
|---|---|---|
| `EnableDrive()` | DRIVE_DISABLED | DRIVE_ENABLED |
| `DisableDrive()` | qualsiasi | DRIVE_DISABLED |
| `Start(v)` | DRIVE_ENABLED / MOTOR_STOPPING / DRIVE_DISABLED* | MOTOR_RUNNING |
| `NewSpeedSetPoint(v)` | MOTOR_RUNNING | MOTOR_RUNNING (aggiorna il target) |
| `Stop()` | MOTOR_RUNNING | MOTOR_STOPPING |
| `ImmediateStop()` | MOTOR_RUNNING / MOTOR_STOPPING | DRIVE_ENABLED |

\* `Start` da DRIVE_DISABLED **auto-abilita**. I metodi pubblici **non cambiano lo stato**: settano solo variabili d'appoggio (`_Enabled`, `_RunRequest`, target, ...); è lo **`switch` nel `Loop()`** l'unico a far evolvere lo stato e a toccare l'hardware. Una richiesta "in stato sbagliato" semplicemente ha effetto quando lo stato lo consente.

`TargetSpeedReached()` = `MOTOR_RUNNING && SpeedSetPoint == TargetSpeed` (a regime).

## Velocità e rampa

- `Start(int)` / `NewSpeedSetPoint(int)` accettano **-100..+100**: il **segno è la direzione**, il modulo la velocità.
- La velocità applicata insegue il target con un **limitatore di pendenza (slew-rate)**: niente overshoot (l'ultimo passo si aggancia esatto al target). `SetSpeedRampTime(ms)` (0 = istantaneo).
- **Oggi** la rampa è a **tempo costante** (qualunque Δ raggiunge il target in `rampTime`). Domani diventerà ad **accelerazione costante** con un solo setter aggiuntivo — stesso meccanismo.
- **Cambio di segno** (es. 50→-20): la velocità applicata rampa **50 → 0 → -20**; `PH` si ribalta **allo zero**, `EN=|applicata|` → breve brake nel passaggio, niente scatto in retro. Le uscite seguono sempre la velocità **applicata**, non il target.

`Stop()` = soft-stop (rampa a 0). `ImmediateStop()` = stop immediato (azzera l'uscita).

## Anti-blocco e fault

Lo stallo (corrente alta sostenuta) sta **sotto l'OCP** del DRV8874, quindi va gestito a tempo in firmware. La soglia di corrente dipende dalla velocità: si definisce con una **spezzata** (`LinearConversion`).

- Controllato **solo a regime** (velocità assestata), non durante la rampa.
- `SetStallControlDelay(ms)` = attesa dopo l'assestamento prima di armare (default 100).
- `SetStallDelay(ms)` = quanto la corrente deve restare sopra soglia per dichiarare lo stallo (timer + debounce via `DigitalSignalHandler`).
- `EnableStallDetection()` / `DisableStallDetection()` = riarma/disarma il rilevamento. Disarmato, il motore **non** si ferma da sé sullo stallo: serve alle prove di taratura (es. rotore bloccato per misurare la corrente di stallo). Il fault hardware (`nFAULT`) resta comunque attivo.

**Reazione (tenuta in libreria)**: allo stallo o al fault la libreria fa `ImmediateStop()` da sé **+** scatta la callback (`OnStall` / `OnFault`).
- `IsMotorStalled()` è **latchato** (si azzera al successivo `Start()`/`EnableDrive()`).
- `IsDriveFault()` è **vivo** (riflette `nFAULT`, condizione hardware che persiste finché non rientra).

> ⚠️ La callback `OnStall` è il punto dove l'app aggiunge la sua logica (avviso, retry, inversione per disinceppare). La libreria ha già fermato il motore.

## Setup

```cpp
DRV8874MotorHandler Motore;

Motore.SetName("MotoreLame");
Motore.SetClockTime(HIGH_RATE_TASK_PERIOD / MILLISECONDS_TO_MICROSECONDS);
Motore.SetControlPins(/*EN=*/6, /*PH=*/7);
Motore.SetPwm(/*canaleLEDC=*/0, 20000, 8);   // canale, 20 kHz, 8 bit
Motore.SetSleepPin(20);                       // opzionale
Motore.SetFaultPin(5);                        // opzionale
Motore.SetCurrentSense(/*ADC=*/4, /*R_IPROPI=*/1000.0f, /*A_IPROPI=*/0.00045f);
Motore.SetCurrentFilterTimeConstant(20);
Motore.SetSpeedRampTime(500);                 // soft-start/cambio in ~0,5 s

Motore.SetStallCurve({ {0, 0.2f}, {25, 0.8f}, {50, 1.6f}, {100, 3.2f} });  // |vel%| -> A
Motore.SetStallDelay(700);
Motore.SetStallControlDelay(100);
Motore.SetOnStallCallback([]() { /* es. notifica utente */ });
```

`Loop()` va agganciato a un task abbastanza veloce (lo stallo lavora bene a 1-10 ms):

```cpp
Scheduler.AddFunction(HighRateTask, []() { Motore.Loop(); });
```

## Controllo runtime

```cpp
Motore.Start(70);            // abilita (se serve) e va a 70% avanti
Motore.NewSpeedSetPoint(-40);// 40% indietro (rampa attraverso lo zero)
Motore.Stop();               // rampa a 0
Motore.ImmediateStop();      // stop immediato
Motore.DisableDrive();       // sleep

DRV8874MotorHandler::MotorState st = Motore.GetState();
const char* n = Motore.GetStateName();
bool enabled  = Motore.IsDriveEnabled();
bool running  = Motore.IsMotorRunning();
bool reached  = Motore.TargetSpeedReached();
int  target   = Motore.GetTargetSpeed();
int  applied  = Motore.GetSpeedSetPoint();
float current = Motore.GetCurrent();
bool  fault   = Motore.IsDriveFault();
bool  stalled = Motore.IsMotorStalled();
```

> Senza `SetSpeedDeadband` (rimossa): la stabilità del set-point è responsabilità dell'app. Chiama `NewSpeedSetPoint` solo a cambio reale (isteresi sul potenziometro), altrimenti il "a regime" non si assesta mai e il controllo stallo non si arma.

## Note

- **PWM/LEDC**: con Arduino 2.0.17 i canali LEDC sono espliciti e limitati (C3: 6). Il canale lo passi tu in `SetPwm(canale, freq, ris)` ed è **responsabilità del main** assegnare canali distinti ai vari handler PWM. In Arduino 3 il nuovo `ledcAttach(pin, freq, ris)` assegna il canale da sé e questo parametro sparirà.
- **Fail-safe**: al reset/hang i GPIO vanno Hi-Z e `EN` (pull-down interno 100k) scende → ponte in brake = motore fermo, anche senza `nSLEEP`.
- **Corrente**: gestita da un `AnalogInputHandler` interno (ADC + filtro 1° ordine + scaling a Ampere). `GetCurrent()` è già in A, filtrata.

## Dipendenze

- `LinearConversion`, `DigitalSignalHandler`, `AnalogInputHandler`, `LoggerHandler`
