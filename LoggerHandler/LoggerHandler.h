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

enum class LogType { Debug, Info, Warning, Error, FatalError };

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
    static LoggerHandler& GetInstance ();

    // Configurazione generale
    void SetDateTimeProvider (DateTimeProvider* Provider);
    void SetSerialSpeed (unsigned long BaudRate);
    void SetMaxMessagesPerCycle (int MaxMessages);

    // Configurazione WebSerial
    void SetWebServer (AsyncWebServer* Server);
    void SetWebServerRunning ();
    void SetWebServerNotRunning ();

    // Controllo runtime
    void Enable ();
    void Disable ();
    void EnableSerial ();
    void DisableSerial ();
    void EnableWebSerial ();
    void DisableWebSerial ();

    // Logging
    void Log (LogType Type, const String& FunctionName, const String& Message);
    void LogFromISR (LogType Type, const char* FunctionName, const char* Message);

    // Chiamato ciclicamente
    void Loop ();

private:
    LoggerHandler ();

    void _FormatLog (const LogEntry& Entry, char* Buffer, size_t BufferSize);
    void _PublishLog (const char* FormattedText);

    DateTimeProvider*  _TimeProvider              = nullptr;
    bool               _SerialEnabled             = true;
    AsyncWebServer*    _WebServer                 = nullptr;
    bool               _WebServerRunning          = false;
    bool               _WebSerialEnabled          = false;
    bool               _WebSerialBeginDone        = false;
    bool               _LogEnabled                = true;
    int                _MaxMessagesPerCycle       = 50;

    unsigned long      _WebSerialSemaphoreMaxTime = 100;

    QueueHandle_t      _LogQueue;
    SemaphoreHandle_t  _WebSerialSemaphore;

    std::atomic<int>   _DroppedMessages{0};
};

extern LoggerHandler& Logger;

#define LOG(Type, FunctionName, Message)     LoggerHandler::GetInstance().Log(Type, FunctionName, Message)
#define LOG_ISR(Type, FunctionName, Message) LoggerHandler::GetInstance().LogFromISR(Type, FunctionName, Message)
