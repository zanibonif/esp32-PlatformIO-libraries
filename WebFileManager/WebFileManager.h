#pragma once
#include <ESPAsyncWebServer.h>

class WebFileManager {
public:
    static WebFileManager& GetInstance ();
    WebFileManager (const WebFileManager&)            = delete;
    WebFileManager& operator= (const WebFileManager&) = delete;

    // Configurazione
    void SetBasePath (const String& BasePath);

    // Controllo runtime
    void Begin (AsyncWebServer* Server);
    bool IsStarted ();

private:
    WebFileManager ();

    void _RegisterRoutes (AsyncWebServer* Server);

    String _BasePath  = "/www";
    bool   _IsStarted = false;
};

extern WebFileManager& FileManager;
