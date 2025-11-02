#ifndef WEB_SERVER_HANDLER_H
#define WEB_SERVER_HANDLER_H

#include <ESPAsyncWebServer.h>

class WebServerHandler {
    private:
        String LogName = "WebServerHandler";
        AsyncWebServer* Server = nullptr;
        bool IsStarted = false;

    public:
        WebServerHandler();
        ~WebServerHandler();

        void Start();
        void Stop();

        AsyncWebServer* GetServer();
        bool IsRunning();
};

WebServerHandler::WebServerHandler() {
    Server = new AsyncWebServer(80);
    LOG(INFO, LogName, "Instance created");
}

WebServerHandler::~WebServerHandler() {
    Stop();
    delete Server;
    Server = nullptr;
    LOG(INFO, LogName, "Instance destroyed");
}

void WebServerHandler::Start() {
    if (IsStarted) return;

    Server->begin();
    IsStarted = true;
    LOG(INFO, LogName, "Server started");
}

void WebServerHandler::Stop() {
    if (!IsStarted) return;

    Server->end();
    IsStarted = false;
    LOG(INFO, LogName, "Server stopped");
}

AsyncWebServer* WebServerHandler::GetServer() {
    return Server;
}

bool WebServerHandler::IsRunning() {
    return IsStarted;
}

#endif // WEB_SERVER_HANDLER_H
