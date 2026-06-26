#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <atomic>
#include <functional>
#include "DateTimeProvider.h"

// --- Define configurabili ---
#define LOGGER_FUNCTION_NAME_MAX_LEN  64
#define LOGGER_MESSAGE_MAX_LEN        192
#define LOGGER_FORMATTED_MAX_LEN      320
#define LOGGER_QUEUE_SIZE             100

// USB-CDC nativa (S3/C3): robustezza output su Serial. Override-abili da build_flags.
#ifndef LOGGER_USB_TX_BUFFER_SIZE
#define LOGGER_USB_TX_BUFFER_SIZE     2048   // byte — ring buffer TX
#endif
#ifndef LOGGER_USB_TX_TIMEOUT_MS
#define LOGGER_USB_TX_TIMEOUT_MS      100    // ms — attesa host prima di scartare
#endif

// Log su file (LittleFS): rotazione a 2 file. Override-abili da build_flags.
#ifndef LOGGER_FILE_PATH_0
#define LOGGER_FILE_PATH_0            "/log/log0.txt"   // corrente
#endif
#ifndef LOGGER_FILE_PATH_1
#define LOGGER_FILE_PATH_1            "/log/log1.txt"   // storico
#endif
#ifndef LOGGER_FILE_BUFFER_SIZE
#define LOGGER_FILE_BUFFER_SIZE       4096
#endif
#ifndef LOGGER_FILE_MAX_SIZE
#define LOGGER_FILE_MAX_SIZE          100000
#endif
#ifndef LOGGER_FILE_WRITE_INTERVAL_MS
#define LOGGER_FILE_WRITE_INTERVAL_MS 30000
#endif

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
    void SetClockTime (unsigned long Ms);

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

    // Log su file (LittleFS gia' montato esternamente)
    void EnableFileLog ();
    void DisableFileLog ();
    void WriteLogFile ();                                          // scrittura manuale del buffer
    void ReadFullLog (std::function<void(const char*)> OnLine);    // storico + corrente
    void ReadLogFile (int FileIndex, std::function<void(const char*)> OnLine); // 0 = corrente, 1 = storico
    void ClearLogFiles ();

    // Logging
    void Log (LogType Type, const String& FunctionName, const String& Message);
    void LogFromISR (LogType Type, const char* FunctionName, const char* Message);

    // Chiamato ciclicamente
    void Loop ();

private:
    LoggerHandler ();

    void _FormatLog (const LogEntry& Entry, char* Buffer, size_t BufferSize);
    void _PublishLog (const char* FormattedText);

    // Log su file
    void _AppendToBuffer (const char* Text);
    void _EnsureLogDir ();                  // crea la cartella del log se il path la prevede
    void _WriteBuffer (bool Blocking);     // prende il semaforo, poi scrive
    void _WriteBufferLocked ();            // scrive su file0 (semaforo gia' preso)
    void _RotateFiles ();                  // file0 -> file1 (semaforo gia' preso)
    void _ServiceFileLog ();
    void _ReadFile (const char* Path, std::function<void(const char*)>& OnLine);

    DateTimeProvider*  _TimeProvider              = nullptr;
    bool               _SerialEnabled             = true;
    AsyncWebServer*    _WebServer                 = nullptr;
    bool               _WebServerRunning          = false;
    bool               _WebSerialEnabled          = false;
    bool               _WebSerialBeginDone        = false;
    bool               _LogEnabled                = true;
    int                _MaxMessagesPerCycle       = 50;
    unsigned long      _ClockTime                 = 100;   // ms — periodo di Loop()

    unsigned long      _WebSerialSemaphoreMaxTime = 100;

    QueueHandle_t      _LogQueue;
    SemaphoreHandle_t  _WebSerialSemaphore;

    std::atomic<int>   _DroppedMessages{0};

    // Stato log su file (le operazioni su file sono protette da _FileLogSemaphore)
    bool               _FileLogEnabled      = false;
    char               _FileBuffer[LOGGER_FILE_BUFFER_SIZE];
    size_t             _FileBufferLen       = 0;
    size_t             _File0Size           = 0;
    unsigned long      _WriteTimer          = 0;
    bool               _ForceWrite          = false;   // ERROR/FATAL: scrittura immediata
    bool               _FileOverflowPending = false;   // righe perse: marcatore da scrivere
    SemaphoreHandle_t  _FileLogSemaphore    = nullptr;
};

extern LoggerHandler& Logger;

#define LOG(Type, FunctionName, Message)     LoggerHandler::GetInstance().Log(Type, FunctionName, Message)
#define LOG_ISR(Type, FunctionName, Message) LoggerHandler::GetInstance().LogFromISR(Type, FunctionName, Message)
