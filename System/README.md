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

## Deep sleep

```cpp
Hibernate(30 * SECONDS_TO_MICROSECONDS);   // 30 secondi in deep sleep

// Al risveglio:
String Reason = GetWakeUpReason();   // es. "Timer", "External pin"
```
