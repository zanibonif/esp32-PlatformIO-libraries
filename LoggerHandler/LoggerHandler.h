#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

enum class LogTarget { SerialOnly, WebSerialOnly, Both };
enum class LogType { Debug, Info, Warning, Error, FatalError };

#define DEBUG         LogType::Debug
#define INFO          LogType::Info
#define WARNING       LogType::Warning
#define ERROR         LogType::Error
#define FATAL_ERROR   LogType::FatalError

struct LogEntry {
    LogType Type;
    String FunctionName;
    String Message;
};

class LoggerHandler {
    public:
        static LoggerHandler& Instance();

        void SetWebServer(AsyncWebServer* server);
        void SetWebServerRunning();
        void SetWebServerNotRunning();
        void SetSerialSpeed(unsigned long BaudRate);
        void SetTarget(LogTarget target);
        void Enable();
        void Disable();
        void Log(LogType type, const String& functionName, const String& message);

    private:
        LoggerHandler();

        static void LoggerTask(void* pvParams);
        static void WebSerialServiceTask(void* pvParams);
        String FormatLog(const LogEntry& entry);

        SemaphoreHandle_t WebSerialSemaphore;
        unsigned long WebSerialSemaphoreMaxTime = 100; // ms

        int LoggerTaskPriority = 1;
        unsigned long LoggerTaskDelay = 20; // ms

        int WebSerialServiceTaskPriority = 1;

        int LoggerQueueSize = 100;

        unsigned long WebSerialServiceTaskPeriod = 200;

        AsyncWebServer* WebServer;
        bool WebServerRunning;
        LogTarget Target;
        bool LogEnabled;

        QueueHandle_t LogQueue;
        TaskHandle_t LoggerTaskHandle;
        TaskHandle_t WebSerialServiceTaskHandle;
};

#define LOG(Type, FunctionName, Message) LoggerHandler::Instance().Log(Type, FunctionName, Message)
