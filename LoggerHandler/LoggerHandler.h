#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <atomic>
#include "DateTimeProvider.h"

// --- Define configurabili ---
#define LOGGER_FUNCTION_NAME_MAX_LEN  64
#define LOGGER_MESSAGE_MAX_LEN        192
#define LOGGER_FORMATTED_MAX_LEN      320
#define LOGGER_QUEUE_SIZE             100

enum class LogTarget { SerialOnly, WebSerialOnly, Both };
enum class LogType   { Debug, Info, Warning, Error, FatalError };

#define DEBUG       LogType::Debug
#define INFO        LogType::Info
#define WARNING     LogType::Warning
#define ERROR       LogType::Error
#define FATAL_ERROR LogType::FatalError

struct LogEntry {
    LogType Type;
    char    FunctionName[LOGGER_FUNCTION_NAME_MAX_LEN];
    char    Message[LOGGER_MESSAGE_MAX_LEN];
};

class LoggerHandler {
public:
    static LoggerHandler& Instance();

    // Configurazione - solo in fase di setup
    void SetDateTimeProvider(DateTimeProvider* Provider);
    void SetWebServer(AsyncWebServer* Server);
    void SetSerialSpeed(unsigned long BaudRate);
    void SetTarget(LogTarget Target);
    void SetMaxMessagesPerCycle(int MaxMessages);

    // Controllo runtime
    void Enable();
    void Disable();
    void SetWebServerRunning();
    void SetWebServerNotRunning();

    // Logging
    void Log(LogType Type, const String& FunctionName, const String& Message);
    void LogFromISR(LogType Type, const char* FunctionName, const char* Message);

    // Chiamato ciclicamente
    void Loop();

private:
    LoggerHandler();

    void _FormatLog(const LogEntry& Entry, char* Buffer, size_t BufferSize);
    void _PublishLog(const char* FormattedText);

    DateTimeProvider*  _TimeProvider            = nullptr;
    AsyncWebServer*    _WebServer               = nullptr;
    bool               _WebServerRunning        = false;
    LogTarget          _Target                  = LogTarget::SerialOnly;
    bool               _LogEnabled              = true;
    int                _MaxMessagesPerCycle     = 50;

    unsigned long      _WebSerialSemaphoreMaxTime = 100;

    QueueHandle_t      _LogQueue;
    SemaphoreHandle_t  _WebSerialSemaphore;

    std::atomic<int>   _DroppedMessages{0};
};

#define LOG(Type, FunctionName, Message) LoggerHandler::Instance().Log(Type, FunctionName, Message)
#define LOG_ISR(Type, FunctionName, Message) LoggerHandler::Instance().LogFromISR(Type, FunctionName, Message)