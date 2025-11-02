#ifndef COMMAND_OTA_HANDLER_H
#define COMMAND_OTA_HANDLER_H

#include <ArduinoOTA.h>
#include <System.h>
#include <LoggerHandler.h>

class CommandOtaHandler {
    private:
        String LogName = "CommandOtaHandler";
        TaskHandle_t CommandOtaHandlerTaskPointer = NULL;
        int CommandOtaHandlerTaskPriority = 1;
        String OtaHostname = "UndefinedHostname";
        String OtaPassword = "";
        unsigned int OtaPort = 3232;
        unsigned long ClockTime = 100; // milliseconds

        // Private variables
        bool UploadInProgress = false;

        unsigned int LastUpdateProgressSent = 0;

        // Private function: Task loop for OTA management
        void CommandOtaHandlerTask(void* pvParameters);

    public:
        // Constructor and destructor
        CommandOtaHandler();
        ~CommandOtaHandler();

        // Public configuration functions
        void SetHostname(const String& hostname);
        void SetPassword(const String& password);
        void SetPort(unsigned int port);

        // Public functions
        void Start();
        void Stop();
        bool IsUploadInProgress();
};

// Constructor
CommandOtaHandler::CommandOtaHandler() {
    LOG(INFO, LogName, "Instance created");
}

// Destructor
CommandOtaHandler::~CommandOtaHandler() {
    Stop();
    LOG(INFO, LogName, "Instance deleted");
}

void CommandOtaHandler::SetHostname(const String& hostname) {
    OtaHostname = hostname;
    LOG(INFO, LogName, "Hostname is " + OtaHostname);
}

void CommandOtaHandler::SetPassword(const String& password) {
    OtaPassword = password;
    LOG(INFO, LogName, "Password is " + OtaPassword);
}

void CommandOtaHandler::SetPort(unsigned int port) {
    OtaPort = port;
    LOG(INFO, LogName, "Port is " + OtaPort);
}

bool CommandOtaHandler::IsUploadInProgress()  {
    return UploadInProgress;
}

// Start OTA management
void CommandOtaHandler::Start() {

    ArduinoOTA.setHostname(OtaHostname.c_str());
    if (!OtaPassword.isEmpty()) {
        ArduinoOTA.setPassword(OtaPassword.c_str());
    }

    ArduinoOTA.setPort(OtaPort);

    ArduinoOTA.onStart([this]() {
        UploadInProgress = true;
        LastUpdateProgressSent = 0;
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        LOG(INFO, LogName, "Update started" + type);
    });

    ArduinoOTA.onEnd([this]() {
        UploadInProgress = false;
        LOG(INFO, LogName, "Update completed");
    });

    ArduinoOTA.onProgress([this](unsigned int progress, unsigned int total) {
        unsigned int CurrentProgress = (progress * 100) / total;
        if (((CurrentProgress % 5) == 0) && (CurrentProgress > LastUpdateProgressSent)) {
            LastUpdateProgressSent = CurrentProgress;
            LOG(INFO, LogName, "Update progress: " + String(CurrentProgress));
        }
    });

    ArduinoOTA.onError([this](ota_error_t error) {
        LOG(INFO, LogName, "Error[" + String(error) + "]");
        if      (error == OTA_AUTH_ERROR)    LOG(ERROR, LogName, "Auth Failed");
        else if (error == OTA_BEGIN_ERROR)   LOG(ERROR, LogName, "Begin Failed");
        else if (error == OTA_CONNECT_ERROR) LOG(ERROR, LogName, "Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) LOG(ERROR, LogName, "Receive Failed");
        else if (error == OTA_END_ERROR)     LOG(ERROR, LogName, "End Failed");
    });

    LOG(INFO, LogName, "Configured");
    ArduinoOTA.begin();

    BaseType_t Task;
    Task = xTaskCreatePinnedToCore([](void* pvParameters) {
        CommandOtaHandler* _this = reinterpret_cast<CommandOtaHandler*>(pvParameters);
        _this->CommandOtaHandlerTask(pvParameters);
    }, "CommandOtaHandlerTask", 10240, this, CommandOtaHandlerTaskPriority, &CommandOtaHandlerTaskPointer, 0);

    if (Task == pdPASS) {
        LOG(INFO, LogName, "Task created");
    } else {
        LOG(ERROR, LogName, "Failed to create task");
    }
}

// Stop OTA management
void CommandOtaHandler::Stop() {
    if (CommandOtaHandlerTaskPointer != NULL) {
        LOG(INFO, LogName, "Task deleted");
        vTaskDelete(CommandOtaHandlerTaskPointer);
    }
}

// OTA handler task
void CommandOtaHandler::CommandOtaHandlerTask(void* pvParameters) {
    CommandOtaHandler* _this = reinterpret_cast<CommandOtaHandler*>(pvParameters);
    TickType_t StartTick, ExecutionTick, TaskTickPeriod;
    TaskTickPeriod = _this->ClockTime / portTICK_PERIOD_MS;

    while (true) {
        StartTick = xTaskGetTickCount();

        ArduinoOTA.handle(); // Check for OTA updates

        if (!UploadInProgress){
            ExecutionTick = xTaskGetTickCount() - StartTick;
            if (ExecutionTick < TaskTickPeriod) {
                vTaskDelay(TaskTickPeriod - ExecutionTick);
            }
        }
    }
}

#endif // COMMAND_OTA_HANDLER_H
