# LittleFSHandler

Accesso al filesystem LittleFS in flash. Singleton con alias `FileSystem`.

## Alias globale

```cpp
extern LittleFSHandler& FileSystem;
```

## Setup

```cpp
FileSystem.Init();   // monta la partizione; se corrotta, la formatta automaticamente
```

## Operazioni file

```cpp
// Scrittura (sovrascrive se esiste)
FileSystem.WriteFile("/config.json", "{\"key\":\"value\"}");

// Lettura tramite File Arduino
if (FileSystem.FileExists("/config.json")) {
    File F = FileSystem.OpenFile("/config.json", "r");
    String Content = F.readString();
    F.close();
}

// Cancellazione
FileSystem.DeleteFile("/config.json");
```

## Apertura file con mode

```cpp
File F = FileSystem.OpenFile("/log.txt", "a");   // append
F.println("nuova riga");
F.close();
```

Mode standard C: `"r"` lettura, `"w"` scrittura (sovrascrive), `"a"` append.

## Diagnostica

```cpp
size_t Total = FileSystem.TotalBytes();
size_t Used  = FileSystem.UsedBytes();
FileSystem.PrintFiles();          // stampa via LOG tutta la struttura a partire da "/"
FileSystem.PrintFiles("/data");   // sottocartella specifica
```

## Formattazione

```cpp
FileSystem.Format();   // cancella tutto il filesystem
```

## Note

- Non ha `Loop()`: tutte le operazioni sono sincrone.
- Non va agganciato al DMPOScheduler.
- L'alias è `FileSystem` (non `LittleFS`) perché `LittleFS` è già occupato dall'oggetto globale del framework ESP32.

## Dipendenze

- `LittleFS` (incluso in Arduino ESP32)
- `LoggerHandler`
