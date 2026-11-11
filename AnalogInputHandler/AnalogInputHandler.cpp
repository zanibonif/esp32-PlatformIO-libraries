#include "AnalogInputHandler.h"

AnalogInputHandler::AnalogInputHandler () {
    _InputFilter.SetClockTime(_ClockTime);
    _InputFilter.SetFilterType(FIRST_ORDER_FILTER);
    _InputFilter.SetFilterTimeConstant(_InputFilterTimeConstant);
}

void AnalogInputHandler::SetName (String Name) {
    _Name    = Name;
    _LogName = "AnalogInputHandler - " + _Name;
    LOG(INFO, _LogName, "Instance active");
}

void AnalogInputHandler::SetClockTime (unsigned long ClockTime) {
    _ClockTime = ClockTime;
    _InputFilter.SetClockTime(_ClockTime);
}

void AnalogInputHandler::SetGPIO (int Gpio) {
    _GPIO          = Gpio;
    _AdcConfigured = false;

    // The core knows the GPIO -> ADC unit/channel map of every SoC: no table to keep here.
    int Channel = digitalPinToAnalogChannel(_GPIO);
    if (Channel < 0) {
        LOG(ERROR, _LogName, "GPIO " + String(_GPIO) + " non è un pin ADC valido su questo SoC");
        return;
    }

    // 12 dB (ADC_11db is its historical name): the widest range. On the C3 the reading is
    // reliable up to about 2.5 V and saturates towards 3 V above that.
    analogSetPinAttenuation(_GPIO, ADC_11db);

    _AdcConfigured = true;
    LOG(INFO, _LogName, "GPIO " + String(_GPIO) + " -> ADC channel " + String(Channel));
    _InputFilter.Reset();
}

void AnalogInputHandler::SetInputFilterTimeConstant (unsigned long FilterTimeConstant) {
    _InputFilterTimeConstant = FilterTimeConstant;
    _InputFilter.SetFilterTimeConstant(_InputFilterTimeConstant);
    _InputFilterTimeConstantSet = true;
}

void AnalogInputHandler::SetScaling (float MinVoltage, float MaxVoltage, float MinEngineeringUnit, float MaxEngineeringUnit) {
    _InputScalingMinVoltage         = MinVoltage;
    _InputScalingMaxVoltage         = MaxVoltage;
    _InputScalingMinEngineeringUnit = MinEngineeringUnit;
    _InputScalingMaxEngineeringUnit = MaxEngineeringUnit;

    _VoltageToEngineeringUnitScaleFactor = (_InputScalingMaxEngineeringUnit - _InputScalingMinEngineeringUnit) / (_InputScalingMaxVoltage - _InputScalingMinVoltage);
    _VoltageToEngineeringUnitOffset      = _InputScalingMinEngineeringUnit - _VoltageToEngineeringUnitScaleFactor * _InputScalingMinVoltage;

    _InputScalingSet = true;
}

void AnalogInputHandler::SetSaturations (float MinEngineeringUnit, float MaxEngineeringUnit) {
    _InputSaturationMinEngineeringUnit = MinEngineeringUnit;
    _InputSaturationMaxEngineeringUnit = MaxEngineeringUnit;
    _InputSaturationLimitsSet          = true;
}

void AnalogInputHandler::Update () {
    if (!_AdcConfigured) {
        LOG(ERROR, _LogName, "ADC non configurato: chiamare SetGPIO con un pin valido");
        return;
    }

    int RawValue   = analogRead(_GPIO);
    int MilliVolts = analogReadMilliVolts(_GPIO);
    _InputADCValue = (float)RawValue;

    // Every error path of analogReadMilliVolts() returns 0, so a missing calibration looks
    // exactly like a grounded input. A real signal on the raw counts with 0 mV can only be
    // that: from then on the input uses the nominal linear scale. Without this check a
    // current input would read 0 A for ever, and a stall would never be detected.
    if (_CalibrationAvailable && (RawValue > CALIBRATION_CHECK_MIN_RAW) && (MilliVolts == 0)) {
        _CalibrationAvailable = false;
        LOG(ERROR, _LogName, "Calibrazione ADC non disponibile (raw " + String(RawValue) + ", 0 mV): si usa la scala nominale");
    }

    _InputMilliVolts = _CalibrationAvailable
        ? (float)MilliVolts
        : ((float)RawValue / (float)_InputResolution) * NOMINAL_FULL_SCALE_MILLIVOLTS;

    _InputFilteredMilliVolts = _InputFilterTimeConstantSet
        ? _InputFilter.Filter(_InputMilliVolts)
        : _InputMilliVolts;

    if (_InputScalingSet) {
        _VoltageValue = _InputFilteredMilliVolts / 1000.0f;

        float EngineeringUnitValue = _VoltageValue * _VoltageToEngineeringUnitScaleFactor + _VoltageToEngineeringUnitOffset;

        if (_InputSaturationLimitsSet) {
            if      (EngineeringUnitValue < _InputSaturationMinEngineeringUnit) { EngineeringUnitValue = _InputSaturationMinEngineeringUnit; }
            else if (EngineeringUnitValue > _InputSaturationMaxEngineeringUnit) { EngineeringUnitValue = _InputSaturationMaxEngineeringUnit; }
        }

        _EngineeringUnitValue = EngineeringUnitValue;
    } else {
        _VoltageValue         = 0.0f;
        _EngineeringUnitValue = 0.0f;
    }
}

String AnalogInputHandler::GetName () {
    return _Name;
}

float AnalogInputHandler::GetADCValue () {
    return _InputADCValue;
}

float AnalogInputHandler::GetVoltage () {
    return _VoltageValue;
}

float AnalogInputHandler::GetValue () {
    return _EngineeringUnitValue;
}

bool AnalogInputHandler::IsCalibrated () const {
    return _CalibrationAvailable;
}
