#include "WebServerHandler.h"
#include <LoggerHandler.h>

WebServerHandler& WebServerHandler::GetInstance () {
    static WebServerHandler Instance;
    return Instance;
}
WebServerHandler& WebServer = WebServerHandler::GetInstance();

WebServerHandler::WebServerHandler () {}

// --- Configurazione ---

void WebServerHandler::SetPort (uint16_t Port) {
    if (_IsStarted) {
        LOG(WARNING, "WebServerHandler::SetPort", "Cannot change port while server is running");
        return;
    }
    _Port = Port;
}

// --- Controllo runtime ---

void WebServerHandler::Start () {
    if (_IsStarted) return;

    _Server = new AsyncWebServer(_Port);
    _Server->begin();
    _IsStarted = true;
    LOG(INFO, "WebServerHandler::Start", "Server started on port " + String(_Port));
}

void WebServerHandler::Stop () {
    if (!_IsStarted) return;

    _Server->end();
    delete _Server;
    _Server    = nullptr;
    _IsStarted = false;
    LOG(INFO, "WebServerHandler::Stop", "Server stopped");
}

AsyncWebServer* WebServerHandler::GetServer () {
    return _Server;
}

bool WebServerHandler::IsRunning () {
    return _IsStarted;
}
