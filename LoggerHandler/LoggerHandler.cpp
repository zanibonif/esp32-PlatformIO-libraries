
#include <WebSerial.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "LoggerHandler.h"

LoggerHandler& LoggerHandler::Instance() {
    static LoggerHandler instance;
    return instance;
}

LoggerHandler::LoggerHandler()
    : WebServer(nullptr), WebServerRunning(false), Target(LogTarget::Both), LogEnabled(true) {

    WebSerialSemaphore = xSemaphoreCreateMutex();
    LogQueue = xQueueCreate(LoggerQueueSize, sizeof(LogEntry));
    xTaskCreatePinnedToCore(LoggerTask,           "LoggerTask",    4096, this, LoggerTaskPriority,           &LoggerTaskHandle,           0);
    xTaskCreatePinnedToCore(WebSerialServiceTask, "WebSerialTask", 4096, this, WebSerialServiceTaskPriority, &WebSerialServiceTaskHandle, 0);
}

void LoggerHandler::SetWebServer(AsyncWebServer* server) {
    WebServer = server;
    WebSerial.begin(server);
}

void LoggerHandler::SetDateTimeProvider(DateTimeProvider* provider) { TimeProvider = provider; }
void LoggerHandler::SetWebServerRunning() { WebServerRunning = true; }
void LoggerHandler::SetWebServerNotRunning() { WebServerRunning = false; }
void LoggerHandler::SetSerialSpeed(unsigned long BaudRate) { Serial.begin(BaudRate); }
void LoggerHandler::SetTarget(LogTarget target) { Target = target; }
void LoggerHandler::Enable()  { LogEnabled = true; }
void LoggerHandler::Disable() { LogEnabled = false; }

void LoggerHandler::Log(LogType type, const String& functionName, const String& message) {
    if (!LogEnabled) return;

    LogEntry* entry = new LogEntry;
    entry->Type = type;
    entry->FunctionName = functionName;
    entry->Message = message;

    if (xQueueSend(LogQueue, &entry, 0) != pdTRUE) {
        delete entry; // queue full
    }
}

void LoggerHandler::LoggerTask(void* pvParams) {
    LoggerHandler* self = reinterpret_cast<LoggerHandler*>(pvParams);
    LogEntry* entry;

    while (true) {
        if (xQueueReceive(self->LogQueue, &entry, portMAX_DELAY) == pdTRUE) {
            String text = self->FormatLog(*entry);

            if ((self->Target == LogTarget::SerialOnly || self->Target == LogTarget::Both)) {
                Serial.println(text);
            }

            if ((self->Target == LogTarget::WebSerialOnly || self->Target == LogTarget::Both) &&
                self->WebServer && self->WebServerRunning) {

                if (xSemaphoreTake(self->WebSerialSemaphore, pdMS_TO_TICKS(self->WebSerialSemaphoreMaxTime)) == pdTRUE) {
                    WebSerial.println(text);
                    xSemaphoreGive(self->WebSerialSemaphore);
                }
            }

            delete entry;
            vTaskDelay(pdMS_TO_TICKS(self->LoggerTaskDelay));
        }
    }
}

void LoggerHandler::WebSerialServiceTask(void* pvParams) {
    LoggerHandler* self = reinterpret_cast<LoggerHandler*>(pvParams);

    while (true) {
        if (self->WebServer && self->WebServerRunning) {
            if (xSemaphoreTake(self->WebSerialSemaphore, pdMS_TO_TICKS(self->WebSerialSemaphoreMaxTime)) == pdTRUE) {
                WebSerial.loop(); // o WebSerial.handle()
                xSemaphoreGive(self->WebSerialSemaphore);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(self->WebSerialServiceTaskPeriod));
    }
}

String LoggerHandler::FormatLog(const LogEntry& entry) {

    String TimeString = "";
    if (TimeProvider) {
        TimeString = TimeProvider->GetFormattedTime("%d/%m/%Y %H:%M:%S") + " | ";
    }

    String TypeString;
    switch (entry.Type) {
        case LogType::Debug:      TypeString = "DEBUG | ";   break;
        case LogType::Info:       TypeString = "INFO | ";    break;
        case LogType::Warning:    TypeString = "WARNING | "; break;
        case LogType::Error:      TypeString = "ERROR | ";   break;
        case LogType::FatalError: TypeString = "FATAL | ";   break;
        default:                  TypeString = "UNKNOWN | "; break;
    }

    return TimeString + TypeString + entry.FunctionName + ": " + entry.Message;

}
