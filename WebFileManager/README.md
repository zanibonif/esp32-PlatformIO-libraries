# WebFileManager

File manager web su LittleFS accessibile via browser. Espone una UI HTML e un'API REST per navigare, caricare, scaricare, cancellare file e creare cartelle.

## Alias globale

```cpp
extern WebFileManager& FileManager;
```

## Setup

`LittleFSHandler` deve essere inizializzato prima di `Begin()`. Tipicamente si chiama nelle callback WiFi:

```cpp
void OnWifiConnected () {
    WebServer.Start();
    FileManager.Begin(WebServer.GetServer());
}

void OnAPStarted () {
    WebServer.Start();
    FileManager.Begin(WebServer.GetServer());
}
```

`Begin()` è **idempotente**: se chiamato più volte registra le route una sola volta sola.

Per cambiare il path radice (default `/www`):

```cpp
FileManager.SetBasePath("/data");  // chiamare prima di Begin()
```

## UI web

Raggiungibile a `http://<ip>/files`. Funzionalità:

- Navigazione cartelle con breadcrumb e tasto `..` per salire
- Upload nella cartella corrente (bottone disabilitato finché non si seleziona un file)
- Download file
- Cancellazione file e cartelle (ricorsiva)
- Creazione nuova cartella

## API REST

| Route | Metodo | Descrizione |
|---|---|---|
| `/files` | GET | Serve la UI HTML |
| `/files/api/list?path=` | GET | JSON: `[{name, size, isDir}]` |
| `/files/api/upload?path=` | POST multipart | Carica un file nella cartella indicata |
| `/files/api/download?path=` | GET | Scarica un file (`Content-Disposition: attachment`) |
| `/files/api/delete?path=` | DELETE | Cancella file o cartella (ricorsiva) |
| `/files/api/mkdir?path=` | POST | Crea una cartella |

## Controllo runtime

```cpp
if (FileManager.IsStarted()) { ... }
```

## Dipendenze

- `ESPAsyncWebServer`
- `LittleFSHandler`
- `WebServerHandler`
