#include "NtpHandler.h"
#include "LoggerHandler.h"

NtpHandler* NtpHandler::StaticInstance = nullptr;

NtpHandler::NtpHandler()
    : NtpClient(Udp, "pool.ntp.org", 0, 60000) {
    LOG(INFO, LogName, "Instance created");
    xTaskCreatePinnedToCore(HandlerTaskStatic, "Ntp_HandlerTask", 8192, this, HandlerTaskPriority, &HandlerTaskPointer, 1);
    LOG(INFO, LogName, "Handler task created");
}

NtpHandler* NtpHandler::GetInstance() {
    if (StaticInstance == nullptr) {
        StaticInstance = new NtpHandler();
    }
    return StaticInstance;
}

void NtpHandler::Destroy() {
    if (StaticInstance) {
        delete StaticInstance;
        StaticInstance = nullptr;
    }
}

void NtpHandler::Enable() {
    Enabled = true;
    LOG(INFO, LogName, "Enabled");
}

void NtpHandler::Disable() {
    Enabled = false;
    LOG(INFO, LogName, "Disabled");
}

bool NtpHandler::IsConnected() {
    return Connected;
}

void NtpHandler::SetGmtOffset(int GmtOffsetHours) {
    NtpClient.setTimeOffset(GmtOffsetHours * 3600);
    LOG(INFO, LogName, "GMT offset set to " + String(GmtOffsetHours) + " hours");
}

void NtpHandler::SetOnSyncCallback(TimeSyncCallback Callback) {
    OnSyncCallback = Callback;
}

void NtpHandler::SetOnDesyncCallback(TimeSyncCallback Callback) {
    OnDesyncCallback = Callback;
}

String NtpHandler::GetFormattedTime(const String& Format) {
    unsigned long RawTime = NtpClient.getEpochTime();
    struct tm* TimeInfo = localtime(reinterpret_cast<time_t*>(&RawTime));
    char Buffer[64];
    strftime(Buffer, sizeof(Buffer), Format.c_str(), TimeInfo);
    return String(Buffer);
}

void NtpHandler::HandlerTaskStatic(void* pvParameters) {
    NtpHandler* instance = reinterpret_cast<NtpHandler*>(pvParameters);
    instance->HandlerTask();
}

void NtpHandler::HandlerTask() {
    TickType_t StartTick, ExecutionTick, TaskTickPeriod;
    TaskTickPeriod = TaskPeriodMs / portTICK_PERIOD_MS;

    static unsigned long Timer = ZERO_TIME;
    static unsigned long ClockTime = TaskPeriodMs;
    bool Timeout;

    NtpStateEnum State = NOT_CONNECTED;

    while (true) {
        StartTick = xTaskGetTickCount();

        if ((Timer - ClockTime) < ClockTime) {
            Timer = ZERO_TIME;
        } else {
            Timer -= ClockTime;
        }
        Timeout = (Timer == ZERO_TIME);

        switch (State) {
            case NOT_CONNECTED:
                if (Enabled) {
                    NtpClient.setUpdateInterval(UpdateInterval);
                    NtpClient.begin();
                    LOG(INFO, LogName, "Time sync starting");
                    Timer = ConnectionMaxTime;
                    State = CONNECTION_IN_PROGRESS;
                }
                break;

            case CONNECTION_IN_PROGRESS:
                if (Timeout) {
                    LOG(WARNING, LogName, "Time sync timeout");
                    NtpClient.end();
                    State = NOT_CONNECTED;
                } else if (!Enabled) {
                    LOG(INFO, LogName, "Time sync stopping");
                    NtpClient.end();
                    State = NOT_CONNECTED;
                } else if (NtpClient.forceUpdate()) {
                    LOG(INFO, LogName, "Time synchronized: current date and time is " + GetFormattedTime("%d/%m/%Y %H:%M:%S"));
                    if (OnSyncCallback) OnSyncCallback();
                    Timer = UpdateInterval + FiveSeconds;
                    State = CONNECTED;
                }
                break;

            case CONNECTED:
                if (!Enabled) {
                    if (OnDesyncCallback) OnDesyncCallback();
                    NtpClient.end();
                    LOG(INFO, LogName, "Time sync stopped");
                    State = NOT_CONNECTED;
                } else if (Timeout) {
                    LOG(WARNING, LogName, "Time synchronization lost");
                    if (OnDesyncCallback) OnDesyncCallback();
                    NtpClient.end();
                    State = NOT_CONNECTED;
                } else if (NtpClient.update()) {
                    Timer = UpdateInterval + FiveSeconds;
                }
                break;
        }

        Connected = (State == CONNECTED);

        ExecutionTick = xTaskGetTickCount() - StartTick;
        if (ExecutionTick < TaskTickPeriod) {
            vTaskDelay(TaskTickPeriod - ExecutionTick);
        }
    }
}
