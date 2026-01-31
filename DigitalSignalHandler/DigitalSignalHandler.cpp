#include "DigitalSignalHandler.h"

DigitalSignalHandler::DigitalSignalHandler() {
    LOG(INFO, LogName, "Instance created");
    RisingEdgeFilterTime = 0;
    FallingEdgeFilterTime = 0;
    LastInputValueActiveTime = 0;
    LastInputValueInactiveTime = 0;
}

DigitalSignalHandler::~DigitalSignalHandler() {
    LOG(INFO, LogName, "Instance deleted");
}

void DigitalSignalHandler::SetName(String name) {
    Name = name;
    LogName = "DigitalSignalHandler - " + Name;
    LOG(INFO, LogName, "Instance active");
}

void DigitalSignalHandler::Enable() {
    LOG(INFO, LogName, "Enabled");
    Enabled = true;
    Startup = true;
}

void DigitalSignalHandler::Disable() {
    LOG(INFO, LogName, "Disabled");
    Enabled = false;
}

void DigitalSignalHandler::SetRisingEdgeFilterTime(unsigned long filterTime) {
    RisingEdgeFilterTime = filterTime;
    LOG(INFO, LogName, "Rising edge filter set to " + String(filterTime) + " ms");
}

void DigitalSignalHandler::SetFallingEdgeFilterTime(unsigned long filterTime) {
    FallingEdgeFilterTime = filterTime;
    LOG(INFO, LogName, "Falling edge filter set to " + String(filterTime) + " ms");
}

bool DigitalSignalHandler::IsEnabled() const {
    return Enabled;
}

void DigitalSignalHandler::OnRisingEdge(EdgeCallback callback) {
    RisingEdgeCallback = callback;
}

void DigitalSignalHandler::OnFallingEdge(EdgeCallback callback) {
    FallingEdgeCallback = callback;
}

bool DigitalSignalHandler::GetSignal() const {
    return PreviousInputValue;
}

bool DigitalSignalHandler::GetFilteredSignal() const {
    return FilteredOutputValue;
}

void DigitalSignalHandler::Update(bool inputValue) {
    if (!Enabled) {
        return;
    }

    unsigned long currentTime = millis();

    if (Startup) {
        PreviousInputValue = inputValue;
        FilteredOutputValue = inputValue;

        if (inputValue) {
            LastInputValueActiveTime = currentTime;
            if (RisingEdgeCallback != nullptr) {
                RisingEdgeCallback();
            }
        } else {
            LastInputValueInactiveTime = currentTime;
            if (FallingEdgeCallback != nullptr) {
                FallingEdgeCallback();
            }
        }

        LOG(INFO, LogName, "Startup - Input: " + String(inputValue) + ", Output: " + String(FilteredOutputValue));
        Startup = false;
        return;
    }

    if (inputValue && !PreviousInputValue) {
        LOG(INFO, LogName, "INPUT Rising edge detected");
    } else if (!inputValue && PreviousInputValue) {
        LOG(INFO, LogName, "INPUT Falling edge detected");
    }

    PreviousInputValue = inputValue;

    if (inputValue) {
        LastInputValueActiveTime = currentTime;
        if (!FilteredOutputValue && ((currentTime - LastInputValueInactiveTime) >= RisingEdgeFilterTime)) {
            FilteredOutputValue = true;
            if (RisingEdgeCallback != nullptr) {
                RisingEdgeCallback();
            }
            LOG(INFO, LogName, "OUTPUT Rising edge detected");
        }
    } else {
        LastInputValueInactiveTime = currentTime;
        if (FilteredOutputValue && ((currentTime - LastInputValueActiveTime) >= FallingEdgeFilterTime)) {
            FilteredOutputValue = false;
            if (FallingEdgeCallback != nullptr) {
                FallingEdgeCallback();
            }
            LOG(INFO, LogName, "OUTPUT Falling edge detected");
        }
    }
}
