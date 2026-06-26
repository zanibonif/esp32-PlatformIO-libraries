# SerialConsoleHandler

Console testuale su seriale per scegliere cosa stampare. Di default la seriale mostra il log live; premendo `M` si apre un menu testuale con voci built-in (log live, log da file, configurazione) ed eventuali voci custom registrate dall'applicazione.

## Alias globale

```cpp
extern SerialConsoleHandler& SerialConsole;
```

## Dipendenze

- `LoggerHandler` — controllo della seriale (`Enable/DisableSerial`) e lettura del log da file (`ReadFullLog`)
- `ParametersHandler` — dump della configurazione (`ForEachParameter`)

## Macchina a stati

| Stato | Log live su seriale | Input |
|---|---|---|
| `NORMAL` (default) | ON | `M` → apre il `MENU` |
| `MENU` | OFF | numero + `INVIO` → seleziona una voce; `ESC` → `NORMAL` |
| `VIEW_LIVE` | ON | `ESC` → torna al `MENU` |

- `M` apre il menu (tasto singolo, immediato): lo trova solo chi lo conosce.
- Nel menu la voce si seleziona digitando il **numero + INVIO** (con echo e backspace).
- `ESC` è il "back" universale: dal log live torna al menu, dal menu torna al log live.
- Entrando nel menu il log live sulla seriale viene sospeso (`Logger.DisableSerial()`): **i messaggi non si perdono**, continuano su file e WebSerial.

Le voci "one-shot" (log da file, configurazione, custom) eseguono e **ristampano il menu** da sole. L'unica vista persistente è il log live, da cui si esce con `ESC`.

## Setup minimo

```cpp
SerialConsole.Enable();
```

`Loop()` va agganciato allo **stesso task del Logger** (aperiodico), subito dopo `Logger.Loop()`, così c'è un solo scrittore sulla seriale:

```cpp
Scheduler.AddFunction(AperiodicTask, []() { Logger.Loop(); });
Scheduler.AddFunction(AperiodicTask, []() { SerialConsole.Loop(); });
```

## Voci custom

Si "pinnano" funzioni proprie come voci aggiuntive del menu (tasti 4, 5, …). Sono one-shot: eseguono e tornano al menu.

```cpp
SerialConsole.AddMenuItem("Abilita WiFi",   []() { Wifi.Enable(); Serial.println("WiFi abilitato"); });
SerialConsole.AddMenuItem("Stampa IP WiFi", []() { Serial.println(Wifi.GetIPAddress()); });
```

La callback può stampare liberamente su `Serial`: in modalità menu il log live è sospeso, quindi nessun output interlacciato. I `LOG(...)` emessi dalla callback non si perdono (vanno su file/WebSerial).

## API

```cpp
void Enable ();
void Disable ();
void Loop ();
void AddMenuItem (const String& Label, std::function<void()> Action);
```

## Controllo runtime

| Macro | Default | Override |
|---|---|---|
| `SERIAL_CONSOLE_MENU_KEY` | `'M'` | `-D SERIAL_CONSOLE_MENU_KEY=...` |
| `SERIAL_CONSOLE_EXIT_KEY` | `0x1B` (ESC) | `-D SERIAL_CONSOLE_EXIT_KEY=...` |

## Note

- La lettura del log da file tiene il semaforo del Logger per tutta la durata del dump: sotto logging molto fitto qualche riga può andare persa durante la stampa (vedi `LoggerHandler`).
- Console event-driven sull'input seriale: nessun timer, nessun `SetClockTime`.
