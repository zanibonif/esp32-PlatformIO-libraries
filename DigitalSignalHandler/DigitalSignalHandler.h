#pragma once

#include <Arduino.h>
#include <functional>
#include <LoggerHandler.h>

typedef void (*EdgeCallback)();

class DigitalSignalHandler {
    public:

        DigitalSignalHandler();
        ~DigitalSignalHandler();

        void SetName(String name);

        void Enable();
        void Disable();
        bool IsEnabled() const;

        void OnRisingEdge(EdgeCallback callback);
        void OnFallingEdge(EdgeCallback callback);

        void SetRisingEdgeFilterTime(unsigned long filterTime);
        void SetFallingEdgeFilterTime(unsigned long filterTime);

        bool GetSignal() const;
        bool GetFilteredSignal() const;

        void Update(bool CurrentValue);

    private:

        String LogName = "DigitalSignalHandler";
        String Name = "";

        bool Enabled = false;

        bool PreviousInputValue = false;
        bool FilteredOutputValue = false;

        bool Startup = true;

        EdgeCallback RisingEdgeCallback = nullptr;
        EdgeCallback FallingEdgeCallback = nullptr;

        unsigned long RisingEdgeFilterTime;
        unsigned long FallingEdgeFilterTime;

        unsigned long LastInputValueActiveTime;
        unsigned long LastInputValueInactiveTime;

};

