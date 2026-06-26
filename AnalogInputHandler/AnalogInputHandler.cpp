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

    // Mappa GPIO -> unità/canale, lineare e specifica per SoC
    bool Valid = false;
#if CONFIG_IDF_TARGET_ESP32S3
    if      (_GPIO >= 1  && _GPIO <= 10) { _AdcUnit = 1; _AdcChannel = _GPIO - 1;  Valid = true; }  // ADC1: GPIO1..10
    else if (_GPIO >= 11 && _GPIO <= 20) { _AdcUnit = 2; _AdcChannel = _GPIO - 11; Valid = true; }  // ADC2: GPIO11..20
#elif CONFIG_IDF_TARGET_ESP32C3
    if      (_GPIO >= 0  && _GPIO <= 4)  { _AdcUnit = 1; _AdcChannel = _GPIO;      Valid = true; }  // ADC1: GPIO0..4
    else if (_GPIO == 5)                 { _AdcUnit = 2; _AdcChannel = 0;          Valid = true; }  // ADC2: GPIO5
#else
    #error "AnalogInputHandler: SoC non supportato, aggiungere la mappa ADC"
#endif

    if (!Valid) {
        LOG(ERROR, _LogName, "GPIO " + String(_GPIO) + " non è un pin ADC valido su questo SoC");
        return;
    }

    if (_AdcUnit == 1) {
        adc1_config_width(static_cast<adc_bits_width_t>(ADC_WIDTH_BIT_12));
        adc1_config_channel_atten(static_cast<adc1_channel_t>(_AdcChannel), ADC_ATTEN_DB_12);
    } else {
        adc2_config_channel_atten(static_cast<adc2_channel_t>(_AdcChannel), ADC_ATTEN_DB_12);
    }

    _AdcConfigured = true;
    LOG(INFO, _LogName, "GPIO " + String(_GPIO) + " -> ADC" + String(_AdcUnit) + " CH" + String(_AdcChannel));
    _InputFilter.Reset();
}

void AnalogInputHandler::SetInputFilterTimeConstant (unsigned long FilterTimeConstant) {
    _InputFilterTimeConstant = FilterTimeConstant;
    _InputFilter.SetFilterTimeConstant(_InputFilterTimeConstant);
    _InputFilterTimeConstantSet = true;
}

void AnalogInputHandler::SetScaling (float VoltageReference, float MinVoltage, float MaxVoltage, float MinEngineeringUnit, float MaxEngineeringUnit) {
    _InputVoltageReference          = VoltageReference;
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

    if (_AdcUnit == 1) {
        int RawValue = adc1_get_raw(static_cast<adc1_channel_t>(_AdcChannel));
        if (RawValue < 0) { LOG(ERROR, _LogName, "ADC1 read error"); }
        else              { _InputADCValue = (float)RawValue; }
    } else {
        int       RawValue = 0;
        esp_err_t Result   = adc2_get_raw(static_cast<adc2_channel_t>(_AdcChannel), ADC_WIDTH_BIT_12, &RawValue);
        if      (Result == ESP_OK)          { _InputADCValue = (float)RawValue; }
        else if (Result == ESP_ERR_TIMEOUT) { LOG(ERROR, _LogName, "ADC read error: Wi-Fi conflict"); }
        else                                { LOG(ERROR, _LogName, "ADC read error: " + String(esp_err_to_name(Result))); }
    }

    _InputFilteredADCValue = _InputFilterTimeConstantSet
        ? _InputFilter.Filter(_InputADCValue)
        : _InputADCValue;

    if (_InputScalingSet) {
        _VoltageValue = (_InputFilteredADCValue / (float)_InputResolution) * _InputVoltageReference;

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
    return _InputFilteredADCValue;
}

float AnalogInputHandler::GetVoltage () {
    return _VoltageValue;
}

float AnalogInputHandler::GetValue () {
    return _EngineeringUnitValue;
}
