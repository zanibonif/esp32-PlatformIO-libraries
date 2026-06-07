#include "DigitalSignalHandler.h"

DigitalSignalHandler::DigitalSignalHandler() {
    LOG(INFO, _LogName, "Instance created");
    _ActivationDelay = 0;
    _DeactivationDelay = 0;
    _Startup = true;
}

DigitalSignalHandler::~DigitalSignalHandler() {
    LOG(INFO, _LogName, "Instance deleted");
}

void DigitalSignalHandler::SetClockTime(unsigned long ClockTime) {
    _ClockTime = ClockTime;
    LOG(INFO, _LogName, "ClockTime set to " + String (_ClockTime) + " ms");
}

void DigitalSignalHandler::SetName(String Name) {
    _LogName = "DigitalSignalHandler - " + Name;
    LOG(INFO, _LogName, "Instance active");
}

void DigitalSignalHandler::Enable() {
    LOG(INFO, _LogName, "Enabled");
    _Enabled = true;
    _Startup = true;
}

void DigitalSignalHandler::Disable() {
    LOG(INFO, _LogName, "Disabled");
    _Enabled = false;
}

void DigitalSignalHandler::SetActivationDelay(unsigned long ActivationDelay) {
    _ActivationDelay = ActivationDelay;
    LOG(INFO, _LogName, "Activation delay set to " + String(_ActivationDelay) + " ms");
}

void DigitalSignalHandler::SetDeactivationDelay(unsigned long DeactivationDelay) {
    _DeactivationDelay = DeactivationDelay;
    LOG(INFO, _LogName, "Deactivation delay set to " + String(_DeactivationDelay) + " ms");
}

bool DigitalSignalHandler::IsEnabled() const {
    return _Enabled;
}

void DigitalSignalHandler::SetActivationCallback(EdgeCallback ActivationCallback) {
    _ActivationCallback = ActivationCallback;
    LOG(INFO, _LogName, "ActivationCallback set");
}

void DigitalSignalHandler::SetDeactivationCallback(EdgeCallback DeactivationCallback) {
    _DeactivationCallback = DeactivationCallback;
    LOG(INFO, _LogName, "DeactivationCallback set");
}

bool DigitalSignalHandler::GetSignal() const {
    return _PreviousInputValue;
}

bool DigitalSignalHandler::GetFilteredSignal() const {
    return _OutputValue;
}

void DigitalSignalHandler::Reset() {
    _Reset = true;
    return;
}

void DigitalSignalHandler::Update(bool InputValue) {
    if (!_Enabled) {
        return;
    }

    if (_Startup) {
        _PreviousInputValue = InputValue;
        _OutputValue = InputValue;
        _PreviousOutputValue = _OutputValue;

        _Timer = ZERO_TIME;

        LOG(INFO, _LogName, "Startup - Input: " + String(InputValue) + ", Output: " + String(_OutputValue));

        if (InputValue && (_ActivationCallback != nullptr)) {
            _ActivationCallback();
        } else if (!InputValue && (_DeactivationCallback != nullptr)) {
            _DeactivationCallback();
        }

        _Startup = false;
        return;
    }

    if (InputValue && !_PreviousInputValue) {
        LOG(DEBUG, _LogName, "Raw signal activation");
    } else if (!InputValue && _PreviousInputValue) {
        LOG(DEBUG, _LogName, "Raw signal deactivation");
    }
    _PreviousInputValue = InputValue;

    if (_Reset) {
        LOG(INFO, _LogName, "Reset");
        _Timer = ZERO_TIME;
        _Reset = false;
    }

    if (_Timer == ZERO_TIME) {
        _OutputValue = InputValue;
    }

    if (_Timer > _ClockTime) {
        _Timer = _Timer - _ClockTime;
    } else {
        _Timer = ZERO_TIME;
    }

    if (_OutputValue == InputValue) {
        if (InputValue) {
            _Timer = _DeactivationDelay;
        } else {
            _Timer = _ActivationDelay;
        }
    }

    if (_OutputValue && !_PreviousOutputValue) {
        LOG(INFO, _LogName, "Filtered signal activation");
        if (_ActivationCallback != nullptr)
            _ActivationCallback();
    } else if (!_OutputValue && _PreviousOutputValue) {
        LOG(INFO, _LogName, "Filtered signal deactivation");
        if (_DeactivationCallback != nullptr)
            _DeactivationCallback();
    }
    _PreviousOutputValue = _OutputValue;
}
