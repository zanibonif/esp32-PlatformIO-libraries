#pragma once

#include <Arduino.h>
#include <functional>
#include <LoggerHandler.h>
#include <System.h>

typedef void (*EdgeCallback)();

class DigitalSignalHandler {
    public:

        DigitalSignalHandler();
        ~DigitalSignalHandler();

        void SetClockTime(unsigned long ClockTime);
        void SetName(String name);

        void Enable();
        void Disable();
        bool IsEnabled() const;

        void SetActivationCallback(EdgeCallback ActivationCallback);
        void SetDeactivationCallback(EdgeCallback DeactivationCallback);

        void SetActivationDelay(unsigned long ActivationDelay);
        void SetDeactivationDelay(unsigned long DeactivationDelay);

        bool GetSignal() const;
        bool GetFilteredSignal() const;

        void Reset();
        void Update(bool CurrentValue);

    private:

        String _LogName = "DigitalSignalHandler";

        bool _Enabled = false;
        bool _Startup = true;
        bool _Reset   = true;

        bool _PreviousInputValue = false;

        bool _OutputValue = false;
        bool _PreviousOutputValue = false;

        EdgeCallback _ActivationCallback   = nullptr;
        EdgeCallback _DeactivationCallback = nullptr;

        unsigned long _Timer              = 0;     // milliseconds
        unsigned long _ClockTime          = 100;   // milliseconds
        unsigned long _ActivationDelay    = 0;     // milliseconds
        unsigned long _DeactivationDelay  = 0;     // milliseconds

};

