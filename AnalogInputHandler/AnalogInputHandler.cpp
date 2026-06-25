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
    _GPIO = Gpio;

    switch (_GPIO) {
        case 36:
        case 37:
        case 38:
        case 39:
        case 32:
        case 33:
        case 34:
        case 35:
            adc1_config_width(static_cast<adc_bits_width_t>(ADC_WIDTH_BIT_12));
            adc1_channel_t Adc1Channel;
            switch (_GPIO) {
                case 36: Adc1Channel = ADC1_CHANNEL_0; LOG(INFO, _LogName, "GPIO 36 (ADC1_CHANNEL_0)"); break;
                case 37: Adc1Channel = ADC1_CHANNEL_1; LOG(INFO, _LogName, "GPIO 37 (ADC1_CHANNEL_1)"); break;
                case 38: Adc1Channel = ADC1_CHANNEL_2; LOG(INFO, _LogName, "GPIO 38 (ADC1_CHANNEL_2)"); break;
                case 39: Adc1Channel = ADC1_CHANNEL_3; LOG(INFO, _LogName, "GPIO 39 (ADC1_CHANNEL_3)"); break;
                case 32: Adc1Channel = ADC1_CHANNEL_4; LOG(INFO, _LogName, "GPIO 32 (ADC1_CHANNEL_4)"); break;
                case 33: Adc1Channel = ADC1_CHANNEL_5; LOG(INFO, _LogName, "GPIO 33 (ADC1_CHANNEL_5)"); break;
                case 34: Adc1Channel = ADC1_CHANNEL_6; LOG(INFO, _LogName, "GPIO 34 (ADC1_CHANNEL_6)"); break;
                case 35: Adc1Channel = ADC1_CHANNEL_7; LOG(INFO, _LogName, "GPIO 35 (ADC1_CHANNEL_7)"); break;
            }
            adc1_config_channel_atten(Adc1Channel, ADC_ATTEN_DB_12);
            break;

        case 4:
        case 0:
        case 2:
        case 15:
        case 13:
        case 12:
        case 14:
        case 27:
        case 25:
        case 26:
            adc2_channel_t Adc2Channel;
            switch (_GPIO) {
                case 4:  Adc2Channel = ADC2_CHANNEL_0; LOG(INFO, _LogName, "GPIO 4  (ADC2_CHANNEL_0)"); break;
                case 0:  Adc2Channel = ADC2_CHANNEL_1; LOG(INFO, _LogName, "GPIO 0  (ADC2_CHANNEL_1)"); break;
                case 2:  Adc2Channel = ADC2_CHANNEL_2; LOG(INFO, _LogName, "GPIO 2  (ADC2_CHANNEL_2)"); break;
                case 15: Adc2Channel = ADC2_CHANNEL_3; LOG(INFO, _LogName, "GPIO 15 (ADC2_CHANNEL_3)"); break;
                case 13: Adc2Channel = ADC2_CHANNEL_4; LOG(INFO, _LogName, "GPIO 13 (ADC2_CHANNEL_4)"); break;
                case 12: Adc2Channel = ADC2_CHANNEL_5; LOG(INFO, _LogName, "GPIO 12 (ADC2_CHANNEL_5)"); break;
                case 14: Adc2Channel = ADC2_CHANNEL_6; LOG(INFO, _LogName, "GPIO 14 (ADC2_CHANNEL_6)"); break;
                case 27: Adc2Channel = ADC2_CHANNEL_7; LOG(INFO, _LogName, "GPIO 27 (ADC2_CHANNEL_7)"); break;
                case 25: Adc2Channel = ADC2_CHANNEL_8; LOG(INFO, _LogName, "GPIO 25 (ADC2_CHANNEL_8)"); break;
                case 26: Adc2Channel = ADC2_CHANNEL_9; LOG(INFO, _LogName, "GPIO 26 (ADC2_CHANNEL_9)"); break;
            }
            adc2_config_channel_atten(Adc2Channel, ADC_ATTEN_DB_12);
            break;

        default:
            LOG(ERROR, _LogName, "Invalid GPIO");
            return;
    }

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
    int       Adc2Value = 0;
    esp_err_t Result;

    switch (_GPIO) {
        case 36: _InputADCValue = (float)adc1_get_raw(ADC1_CHANNEL_0); break;
        case 37: _InputADCValue = (float)adc1_get_raw(ADC1_CHANNEL_1); break;
        case 38: _InputADCValue = (float)adc1_get_raw(ADC1_CHANNEL_2); break;
        case 39: _InputADCValue = (float)adc1_get_raw(ADC1_CHANNEL_3); break;
        case 32: _InputADCValue = (float)adc1_get_raw(ADC1_CHANNEL_4); break;
        case 33: _InputADCValue = (float)adc1_get_raw(ADC1_CHANNEL_5); break;
        case 34: _InputADCValue = (float)adc1_get_raw(ADC1_CHANNEL_6); break;
        case 35: _InputADCValue = (float)adc1_get_raw(ADC1_CHANNEL_7); break;

        case 4:
            Result = adc2_get_raw(ADC2_CHANNEL_0, ADC_WIDTH_BIT_12, &Adc2Value);
            if      (Result == ESP_OK)          { _InputADCValue = (float)Adc2Value; }
            else if (Result == ESP_ERR_TIMEOUT) { LOG(ERROR, _LogName, "ADC2 read error: Wi-Fi conflict"); }
            break;
        case 0:
            Result = adc2_get_raw(ADC2_CHANNEL_1, ADC_WIDTH_BIT_12, &Adc2Value);
            if      (Result == ESP_OK)          { _InputADCValue = (float)Adc2Value; }
            else if (Result == ESP_ERR_TIMEOUT) { LOG(ERROR, _LogName, "ADC2 read error: Wi-Fi conflict"); }
            break;
        case 2:
            Result = adc2_get_raw(ADC2_CHANNEL_2, ADC_WIDTH_BIT_12, &Adc2Value);
            if      (Result == ESP_OK)          { _InputADCValue = (float)Adc2Value; }
            else if (Result == ESP_ERR_TIMEOUT) { LOG(ERROR, _LogName, "ADC2 read error: Wi-Fi conflict"); }
            break;
        case 15:
            Result = adc2_get_raw(ADC2_CHANNEL_3, ADC_WIDTH_BIT_12, &Adc2Value);
            if      (Result == ESP_OK)          { _InputADCValue = (float)Adc2Value; }
            else if (Result == ESP_ERR_TIMEOUT) { LOG(ERROR, _LogName, "ADC2 read error: Wi-Fi conflict"); }
            break;
        case 13:
            Result = adc2_get_raw(ADC2_CHANNEL_4, ADC_WIDTH_BIT_12, &Adc2Value);
            if      (Result == ESP_OK)          { _InputADCValue = (float)Adc2Value; }
            else if (Result == ESP_ERR_TIMEOUT) { LOG(ERROR, _LogName, "ADC2 read error: Wi-Fi conflict"); }
            break;
        case 12:
            Result = adc2_get_raw(ADC2_CHANNEL_5, ADC_WIDTH_BIT_12, &Adc2Value);
            if      (Result == ESP_OK)          { _InputADCValue = (float)Adc2Value; }
            else if (Result == ESP_ERR_TIMEOUT) { LOG(ERROR, _LogName, "ADC2 read error: Wi-Fi conflict"); }
            break;
        case 14:
            Result = adc2_get_raw(ADC2_CHANNEL_6, ADC_WIDTH_BIT_12, &Adc2Value);
            if      (Result == ESP_OK)          { _InputADCValue = (float)Adc2Value; }
            else if (Result == ESP_ERR_TIMEOUT) { LOG(ERROR, _LogName, "ADC2 read error: Wi-Fi conflict"); }
            break;
        case 27:
            Result = adc2_get_raw(ADC2_CHANNEL_7, ADC_WIDTH_BIT_12, &Adc2Value);
            if      (Result == ESP_OK)          { _InputADCValue = (float)Adc2Value; }
            else if (Result == ESP_ERR_TIMEOUT) { LOG(ERROR, _LogName, "ADC2 read error: Wi-Fi conflict"); }
            break;
        case 25:
            Result = adc2_get_raw(ADC2_CHANNEL_8, ADC_WIDTH_BIT_12, &Adc2Value);
            if      (Result == ESP_OK)          { _InputADCValue = (float)Adc2Value; }
            else if (Result == ESP_ERR_TIMEOUT) { LOG(ERROR, _LogName, "ADC2 read error: Wi-Fi conflict"); }
            break;
        case 26:
            Result = adc2_get_raw(ADC2_CHANNEL_9, ADC_WIDTH_BIT_12, &Adc2Value);
            if      (Result == ESP_OK)          { _InputADCValue = (float)Adc2Value; }
            else if (Result == ESP_ERR_TIMEOUT) { LOG(ERROR, _LogName, "ADC2 read error: Wi-Fi conflict"); }
            break;

        default:
            LOG(ERROR, _LogName, "ADC read error: unknown pin");
            return;
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
