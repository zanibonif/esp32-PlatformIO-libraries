# DMPOScheduler

Scheduler FreeRTOS che implementa l'algoritmo **Deadline Monotonic Priority Order**: i task con deadline più breve ricevono priorità più alta. Tutto il lifecycle dei task dell'applicazione passa per lui.

## Alias globale

```cpp
extern DMPOScheduler& Scheduler;
```

## Concetti chiave

- **Task**: un FreeRTOS task con periodo e deadline. Si registra con `AddTask()`.
- **Function**: una `std::function<void()>` agganciata a un task. Più funzioni sullo stesso task vengono eseguite in sequenza ogni tick.
- **AppCritical**: se `true`, il task viene pinnato su **Core 1** (isolato da WiFi e stack di rete su Core 0) e sincronizzato via `esp_timer` hardware. Se `false`, usa `vTaskDelayUntil` su Core 0. Su SoC single-core (es. C3) il Core 1 non esiste → vedi *Single-core* sotto.

## Setup e avvio

```cpp
// 1. Configurare il task
DMPOScheduler::TaskConfig LogTask;
LogTask.Name        = "Logger";
LogTask.PeriodUs    = 50000;   // 50ms
LogTask.AppCritical = false;
LogTask.StackSize   = 4096;
// LogTask.DeadlineUs = 0;     // se 0, si assume uguale a PeriodUs

// 2. Aggiungere il task
Scheduler.AddTask(LogTask);   // scrive LogTask.ID

// 3. Aggiungere le funzioni
Scheduler.AddFunction(LogTask, []() { Logger.Loop(); });
Scheduler.AddFunction(LogTask, []() { Ntp.Loop(); });

// 4. Avviare (non modificare dopo Begin())
Scheduler.Begin();
```

## Task AppCritical — real-time su Core 1

```cpp
DMPOScheduler::TaskConfig MotorTask;
MotorTask.Name        = "MotorControl";
MotorTask.PeriodUs    = 1000;   // 1ms
MotorTask.AppCritical = true;
MotorTask.StackSize   = 8192;
```

I task `AppCritical` sono sincronizzati via semaforo rilasciato da `esp_timer`: jitter nell'ordine dei microsecondi. I task non-`AppCritical` usano `vTaskDelayUntil` con risoluzione di 1 tick FreeRTOS (tipicamente 1ms). Task non-`AppCritical` con `PeriodUs < 1000` vengono rifiutati.

### Single-core (es. ESP32-C3)

Sui SoC single-core (`CONFIG_FREERTOS_UNICORE`, es. C3) non esiste il Core 1: i task `AppCritical` vengono pinnati sul **Core 0**, l'unico disponibile, condiviso col WiFi e lo stack di rete. La sincronizzazione `esp_timer` + semaforo resta valida, ma le garanzie real-time sono più deboli (niente isolamento dal WiFi). Su dual-core (es. S3) il comportamento è invariato.

## Priorità DMPO

Le priorità vengono assegnate automaticamente in `Begin()`: deadline più breve → priorità più alta. Task su Core 0 e Core 1 hanno scale di priorità separate.

## Controllo runtime

```cpp
Scheduler.EnableTask("Logger");
Scheduler.DisableTask("Logger");
Scheduler.EnableTask(TaskId);
Scheduler.DisableTask(TaskId);
Scheduler.EnableAllTasks();
Scheduler.DisableAllTasks();
```

## Diagnostica

```cpp
uint32_t CycleUs = Scheduler.GetLastCycleTimeUs("Logger");
uint32_t Misses  = Scheduler.GetMissedDeadlineCount("Logger");
Scheduler.PrintStatus();   // LOG di tutti i task con stato e statistiche
```

## Callback deadline miss

```cpp
LogTask.OnMissedDeadline = [](const std::string& Name, uint32_t OverrunUs) {
    LOG(WARNING, "Main", ("Deadline miss: " + String(Name.c_str()) +
        " | overrun: " + String(OverrunUs) + " us").c_str());
};
```

## Dipendenze

- `LoggerHandler`
- FreeRTOS (incluso in ESP-IDF)
- `esp_timer` (incluso in ESP-IDF)
