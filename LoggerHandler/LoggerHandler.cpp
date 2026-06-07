#include <WebSerial.h>
#include "LoggerHandler.h"

LoggerHandler& LoggerHandler::Instance()
{
    static LoggerHandler Instance;
    return Instance;
}

LoggerHandler::LoggerHandler()
    : _WebServer(nullptr),
      _WebServerRunning(false),
      _Target(LogTarget::Both),
      _LogEnabled(true),
      _LogQueue(nullptr),
      _DroppedMessages(0)
{
    _WebSerialSemaphore = xSemaphoreCreateMutex();
    _LogQueue           = xQueueCreate(LOGGER_QUEUE_SIZE, sizeof(LogEntry));
}

void LoggerHandler::SetDateTimeProvider(DateTimeProvider* Provider)
{
    _TimeProvider = Provider;
}

void LoggerHandler::SetWebServer(AsyncWebServer* Server)
{
    _WebServer = Server;
    WebSerial.begin(Server);
}

void LoggerHandler::SetSerialSpeed(unsigned long BaudRate)
{
    Serial.begin(BaudRate);
}

void LoggerHandler::SetTarget(LogTarget Target)
{
    _Target = Target;
}

void LoggerHandler::SetMaxMessagesPerCycle(int MaxMessages)
{
    _MaxMessagesPerCycle = MaxMessages;
}

void LoggerHandler::Enable()
{
    _LogEnabled = true;
}

void LoggerHandler::Disable()
{
    _LogEnabled = false;
}

void LoggerHandler::SetWebServerRunning()
{
    _WebServerRunning = true;
}

void LoggerHandler::SetWebServerNotRunning()
{
    _WebServerRunning = false;
}

void LoggerHandler::Log(LogType Type, const String& FunctionName, const String& Message)
{
    if (!_LogEnabled) return;
    if (_LogQueue == nullptr) return;

    LogEntry Entry;
    Entry.Type = Type;

    strncpy(Entry.FunctionName, FunctionName.c_str(), LOGGER_FUNCTION_NAME_MAX_LEN - 1);
    Entry.FunctionName[LOGGER_FUNCTION_NAME_MAX_LEN - 1] = '\0';

    strncpy(Entry.Message, Message.c_str(), LOGGER_MESSAGE_MAX_LEN - 1);
    Entry.Message[LOGGER_MESSAGE_MAX_LEN - 1] = '\0';

    // Troncamento visibile
    if (FunctionName.length() >= LOGGER_FUNCTION_NAME_MAX_LEN)
        strcpy(&Entry.FunctionName[LOGGER_FUNCTION_NAME_MAX_LEN - 4], "...");

    if (Message.length() >= LOGGER_MESSAGE_MAX_LEN)
        strcpy(&Entry.Message[LOGGER_MESSAGE_MAX_LEN - 4], "...");

    if (xQueueSend(_LogQueue, &Entry, 0) != pdTRUE)
        _DroppedMessages++;
}

void LoggerHandler::LogFromISR(LogType Type, const char* FunctionName, const char* Message)
{
    if (!_LogEnabled) return;
    if (_LogQueue == nullptr) return;

    LogEntry Entry;
    Entry.Type = Type;

    strncpy(Entry.FunctionName, FunctionName, LOGGER_FUNCTION_NAME_MAX_LEN - 1);
    Entry.FunctionName[LOGGER_FUNCTION_NAME_MAX_LEN - 1] = '\0';

    strncpy(Entry.Message, Message, LOGGER_MESSAGE_MAX_LEN - 1);
    Entry.Message[LOGGER_MESSAGE_MAX_LEN - 1] = '\0';

    BaseType_t HigherPriorityTaskWoken = pdFALSE;
    if (xQueueSendFromISR(_LogQueue, &Entry, &HigherPriorityTaskWoken) != pdTRUE)
        _DroppedMessages++;

    portYIELD_FROM_ISR(HigherPriorityTaskWoken);
}

void LoggerHandler::_PublishLog(const char* FormattedText)
{
    if (_Target == LogTarget::SerialOnly || _Target == LogTarget::Both)
    {
        Serial.println(FormattedText);
    }

    if ((_Target == LogTarget::WebSerialOnly || _Target == LogTarget::Both) &&
        _WebServer && _WebServerRunning)
    {
        if (xSemaphoreTake(_WebSerialSemaphore, pdMS_TO_TICKS(_WebSerialSemaphoreMaxTime)) == pdTRUE)
        {
            WebSerial.println(FormattedText);
            xSemaphoreGive(_WebSerialSemaphore);
        }
    }
}

void LoggerHandler::Loop()
{
    if (_LogQueue == nullptr) return;

    // --- Notifica messaggi persi ---
    int Dropped = _DroppedMessages.load();
    if (Dropped > 0)
    {
        LogEntry Warning;
        Warning.Type = LogType::Warning;
        strncpy(Warning.FunctionName, "LoggerHandler", LOGGER_FUNCTION_NAME_MAX_LEN - 1);
        snprintf(Warning.Message, LOGGER_MESSAGE_MAX_LEN,
                 "Messaggi persi per queue piena: %d", Dropped);

        if (xQueueSend(_LogQueue, &Warning, 0) == pdTRUE)
            _DroppedMessages.fetch_sub(Dropped);
    }

    // --- Svuotamento queue ---
    LogEntry Entry;
    char FormattedBuffer[LOGGER_FORMATTED_MAX_LEN];
    int ProcessedMessages = 0;

    while (ProcessedMessages < _MaxMessagesPerCycle &&
           xQueueReceive(_LogQueue, &Entry, 0) == pdTRUE)
    {
        _FormatLog(Entry, FormattedBuffer, sizeof(FormattedBuffer));
        _PublishLog(FormattedBuffer);
        ProcessedMessages++;
    }

    // --- WebSerial loop ---
    if (_WebServer && _WebServerRunning)
    {
        if (xSemaphoreTake(_WebSerialSemaphore, pdMS_TO_TICKS(_WebSerialSemaphoreMaxTime)) == pdTRUE)
        {
            WebSerial.loop();
            xSemaphoreGive(_WebSerialSemaphore);
        }
    }
}

void LoggerHandler::_FormatLog(const LogEntry& Entry, char* Buffer, size_t BufferSize)
{
    char TimeString[32] = "";
    if (_TimeProvider)
    {
        String T = _TimeProvider->GetFormattedTime("%d/%m/%Y %H:%M:%S");
        strncpy(TimeString, T.c_str(), sizeof(TimeString) - 1);
    }

    const char* TypeString;
    switch (Entry.Type)
    {
        case LogType::Debug:      TypeString = "DEBUG   "; break;
        case LogType::Info:       TypeString = "INFO    "; break;
        case LogType::Warning:    TypeString = "WARNING "; break;
        case LogType::Error:      TypeString = "ERROR   "; break;
        case LogType::FatalError: TypeString = "FATAL   "; break;
        default:                  TypeString = "UNKNOWN "; break;
    }

    if (strlen(TimeString) > 0)
        snprintf(Buffer, BufferSize, "%s | %s | %s: %s",
                 TimeString, TypeString, Entry.FunctionName, Entry.Message);
    else
        snprintf(Buffer, BufferSize, "%s | %s: %s",
                 TypeString, Entry.FunctionName, Entry.Message);
}