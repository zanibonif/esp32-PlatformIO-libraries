#pragma once
#include <ESPAsyncWebServer.h>

class WebServerHandler {
public:
    static WebServerHandler& GetInstance ();
    WebServerHandler (const WebServerHandler&)            = delete;
    WebServerHandler& operator= (const WebServerHandler&) = delete;

    // Configurazione
    void SetPort (uint16_t Port);

    // Controllo runtime
    void Start ();
    void Stop ();
    AsyncWebServer* GetServer ();
    bool IsRunning ();

private:
    WebServerHandler ();

    AsyncWebServer* _Server    = nullptr;
    uint16_t        _Port      = 80;
    bool            _IsStarted = false;
};

extern WebServerHandler& WebServer;
