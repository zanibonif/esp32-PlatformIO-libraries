# System

Header-only con costanti globali e funzioni di utilità per ESP32. Non è un singleton — non ha alias globale.

## Include

```cpp
#include <System.h>
```

Incluso automaticamente dalla maggior parte delle altre librerie. Raramente va incluso esplicitamente nel codice applicativo.

## Costanti

```cpp
ZERO_TIME                     // 0 — valore iniziale per i timer interni alle librerie
SECONDS_TO_MILLISECONDS       // 1000
MILLISECONDS_TO_MICROSECONDS  // 1000
SECONDS_TO_MICROSECONDS       // 1000000
```

## Versione librerie

```cpp
String Version = GetLibrariesVersion();  // es. "1.0.0"
```

## Gestione CPU

```cpp
SetCpuFrequency(80);   // MHz — valori tipici: 80, 160, 240
unsigned int Freq = GetCpuFrequency();
```

## Uptime

```cpp
unsigned long long UptimeUs = GetUptimeUs();  // microsecondi dall'avvio
unsigned long long UptimeS  = GetUptimeUs() / SECONDS_TO_MICROSECONDS;
```

## Deep sleep

```cpp
Hibernate(30 * SECONDS_TO_MICROSECONDS);   // 30 secondi in deep sleep

// Al risveglio:
String Reason = GetWakeUpReason();   // es. "Timer", "External pin"
```

## Aritmetica sicura (overflow)

Incremento e decremento con saturazione: invece di andare in overflow, il valore si ferma al limite del tipo.

```cpp
SafeIncrement(Value);         // Value++ con saturazione a MAX
SafeIncrement(Value, Step);   // Value += Step con saturazione

SafeDecrement(Value);         // Value-- con saturazione a 0 / MIN
SafeDecrement(Value, Step);
```

Overload disponibili per: `int`, `unsigned int`, `long`, `unsigned long`.

- Signed: satura a `INT_MAX` / `INT_MIN` (o `LONG_MAX` / `LONG_MIN`).
- Unsigned: satura a `UINT_MAX` / `ULONG_MAX`; decremento satura a `0`.
- Float non incluso: overflow float produce ±∞ (comportamento definito IEEE 754).

Uso tipico — contatori persistenti:

```cpp
unsigned long Counter = Parameters.Get(BOOT_COUNTER);
SafeIncrement(Counter);
Parameters.Set(BOOT_COUNTER, Counter);
```
