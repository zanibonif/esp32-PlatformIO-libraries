// AnalogInputsHandler.h
#ifndef ANALOG_INPUTS_HANDLER
#define ANALOG_INPUTS_HANDLER

#include <vector>
#include <AnalogInputHandler.h>
#include <LoggerHandler.h>

// Note: on esp32 ADC2 is shared with WiFi

class AnalogInputsHandler {
    private:
        String LogName = "AnalogInputsHandler";

        std::vector<AnalogInputHandler*> AnalogInputs;

        TaskHandle_t                     HandlerTaskPointer  = nullptr;
        int                              HandlerTaskPriority = 2;
        unsigned long                    HandlerTaskPeriod   = 200; // milliseconds

        static void HandlerTaskStatic(void *pvParameters);
        void HandlerTask();

        void ProcessInputs();

    public:
        AnalogInputsHandler();

        void SetUpdatePeriod(unsigned long Period);
        void AddInput(AnalogInputHandler* AnalogInput);

};

AnalogInputsHandler::AnalogInputsHandler() {
    LOG(INFO, LogName, "Instance created.");
    xTaskCreatePinnedToCore(HandlerTaskStatic, "AnalogInputsHandlerTask", 8192, this, HandlerTaskPriority, &HandlerTaskPointer, 1);
    LOG(INFO, LogName, "Task created.");
}

void AnalogInputsHandler::HandlerTaskStatic(void *pvParameters) {
    AnalogInputsHandler *Instance = reinterpret_cast<AnalogInputsHandler *>(pvParameters);
    Instance->HandlerTask();
}

void AnalogInputsHandler::HandlerTask() {
    TickType_t StartTick, ExecutionTick, TaskTickPeriod;

    while (true) {

        StartTick = xTaskGetTickCount();
        TaskTickPeriod = HandlerTaskPeriod / portTICK_PERIOD_MS;

        ProcessInputs();

        ExecutionTick = xTaskGetTickCount() - StartTick;
        if (ExecutionTick < TaskTickPeriod) {
          vTaskDelay(TaskTickPeriod - ExecutionTick);
        }
    }
}

void AnalogInputsHandler::ProcessInputs() {
    for (auto input : AnalogInputs) {
        if (input) {
            input->UpdateInput();
        }
    }
}

void AnalogInputsHandler::SetUpdatePeriod(unsigned long Period) {
    HandlerTaskPeriod = Period;
}

void AnalogInputsHandler::AddInput(AnalogInputHandler* AnalogInput) {
    if (AnalogInput) {
        AnalogInputs.push_back(AnalogInput);
        LOG(INFO, LogName, AnalogInput->GetName() + " added");
    }
}

#endif // ANALOG_INPUTS_HANDLER
