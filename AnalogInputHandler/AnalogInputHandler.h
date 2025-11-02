// AnalogInputHandler.h
#ifndef ANALOG_INPUT_HANDLER
#define ANALOG_INPUT_HANDLER

#include <driver/adc.h>
#include <TimeDiscreteFilter.h>
#include <LoggerHandler.h>


// Note: on esp32 ADC2 is shared with WiFi

class AnalogInputHandler {
    private:

        String             LogName                             = "AnalogInputHandler";

        String             Name                                = "";

        unsigned long      ClockTime                           = 100;   // milliseconds

        int                GPIO                                = 1;
        unsigned short     InputResolution                     = 4096;

        bool               InputFilterTimeConstantSet          = false;
        unsigned long      InputFilterTimeConstant             = 500;   // milliseconds
        TimeDiscreteFilter InputFilter                         = TimeDiscreteFilter(ClockTime, FIRST_ORDER_FILTER, InputFilterTimeConstant);

        bool               InputScalingSet                     = false;
        float              InputVoltageReference               = 3.3;
        float              InputScalingMinVoltage              = 0.0;
        float              InputScalingMaxVoltage              = 3.3;
        float              InputScalingMinEngineeringUnit      = 0.0;
        float              InputScalingMaxEngineeringUnit      = 100.0;

        bool               InputSaturationLimitsSet            = false;
        float              InputSaturationMinEngineeringUnit   = 0.0;
        float              InputSaturationMaxEngineeringUnit   = 100.0;

        float              VoltageToEngineeringUnitScaleFactor = 0.0;
        float              VoltageToEngineeringUnitOffset      = 0.0;

        float              InputADCValue                       = 0.0;
        float              InputFilteredADCValue               = 0.0;
        float              VoltageValue                        = 0.0;
        float              EngineeringUnitValue                = 0.0;

    public:
        AnalogInputHandler();

        void SetName(String name);
        void SetClockTime(unsigned long clockTime);
        void SetGPIO(int gpio);
        void SetInputFilterTimeConstant(unsigned long filterTimeConstant);
        void SetScaling(float voltageReference, float minVoltage, float maxVoltage, float minEngineeringUnit, float maxEngineeringUnit);
        void SetSaturations(float minEngineeringUnit, float maxEngineeringUnit);

        void UpdateInput();

        String GetName();
        float GetADCValue();
        float GetVoltage();
        float GetValue();
};

AnalogInputHandler::AnalogInputHandler() {
}

void AnalogInputHandler::SetName(String name) {
    Name = name;
    LogName = "AnalogInputHandler - " + Name;
    LOG(INFO, LogName, "Instance active");
}

void AnalogInputHandler::SetClockTime(unsigned long clockTime) {
    ClockTime = clockTime;
    InputFilter.SetClockTime(ClockTime);
}

void AnalogInputHandler::SetGPIO(int gpio) {

    GPIO = gpio;
    // Usare uno switch per determinare se il pin appartiene ad ADC1 o ADC2
    switch (GPIO) {
        // Configurazione per ADC1
        case 36:  // ADC1_CHANNEL_0
        case 37:  // ADC1_CHANNEL_1
        case 38:  // ADC1_CHANNEL_2
        case 39:  // ADC1_CHANNEL_3
        case 32:  // ADC1_CHANNEL_4
        case 33:  // ADC1_CHANNEL_5
        case 34:  // ADC1_CHANNEL_6
        case 35:  // ADC1_CHANNEL_7
            adc1_config_width(static_cast<adc_bits_width_t>(ADC_WIDTH_BIT_12));
            adc1_channel_t adc1_channel;
            switch(GPIO) {
                case 36: adc1_channel = ADC1_CHANNEL_0; LOG(INFO, LogName, "GPIO pin set to 36 (ADC1_CHANNEL_0)"); break;
                case 37: adc1_channel = ADC1_CHANNEL_1; LOG(INFO, LogName, "GPIO pin set to 37 (ADC1_CHANNEL_1)"); break;
                case 38: adc1_channel = ADC1_CHANNEL_2; LOG(INFO, LogName, "GPIO pin set to 38 (ADC1_CHANNEL_2)"); break;
                case 39: adc1_channel = ADC1_CHANNEL_3; LOG(INFO, LogName, "GPIO pin set to 39 (ADC1_CHANNEL_3)"); break;
                case 32: adc1_channel = ADC1_CHANNEL_4; LOG(INFO, LogName, "GPIO pin set to 32 (ADC1_CHANNEL_4)"); break;
                case 33: adc1_channel = ADC1_CHANNEL_5; LOG(INFO, LogName, "GPIO pin set to 33 (ADC1_CHANNEL_5)"); break;
                case 34: adc1_channel = ADC1_CHANNEL_6; LOG(INFO, LogName, "GPIO pin set to 34 (ADC1_CHANNEL_6)"); break;
                case 35: adc1_channel = ADC1_CHANNEL_7; LOG(INFO, LogName, "GPIO pin set to 35 (ADC1_CHANNEL_7)"); break;
            }
            adc1_config_channel_atten(adc1_channel, ADC_ATTEN_DB_12);
            break;

        // Configurazione per ADC2
        case 4:   // ADC2_CHANNEL_0
        case 0:   // ADC2_CHANNEL_1
        case 2:   // ADC2_CHANNEL_2
        case 15:  // ADC2_CHANNEL_3
        case 13:  // ADC2_CHANNEL_4
        case 12:  // ADC2_CHANNEL_5
        case 14:  // ADC2_CHANNEL_6
        case 27:  // ADC2_CHANNEL_7
        case 25:  // ADC2_CHANNEL_8
        case 26:  // ADC2_CHANNEL_9
            adc2_channel_t adc2_channel;
            switch(GPIO) {
                case 4:   adc2_channel = ADC2_CHANNEL_0; LOG(INFO, LogName, "GPIO pin set to 4 (ADC2_CHANNEL_0)"); break;
                case 0:   adc2_channel = ADC2_CHANNEL_1; LOG(INFO, LogName, "GPIO pin set to 0 (ADC2_CHANNEL_1)"); break;
                case 2:   adc2_channel = ADC2_CHANNEL_2; LOG(INFO, LogName, "GPIO pin set to 2 (ADC2_CHANNEL_2)"); break;
                case 15:  adc2_channel = ADC2_CHANNEL_3; LOG(INFO, LogName, "GPIO pin set to 15 (ADC2_CHANNEL_3)"); break;
                case 13:  adc2_channel = ADC2_CHANNEL_4; LOG(INFO, LogName, "GPIO pin set to 13 (ADC2_CHANNEL_4)"); break;
                case 12:  adc2_channel = ADC2_CHANNEL_5; LOG(INFO, LogName, "GPIO pin set to 12 (ADC2_CHANNEL_5)"); break;
                case 14:  adc2_channel = ADC2_CHANNEL_6; LOG(INFO, LogName, "GPIO pin set to 14 (ADC2_CHANNEL_6)"); break;
                case 27:  adc2_channel = ADC2_CHANNEL_7; LOG(INFO, LogName, "GPIO pin set to 27 (ADC2_CHANNEL_7)"); break;
                case 25:  adc2_channel = ADC2_CHANNEL_8; LOG(INFO, LogName, "GPIO pin set to 25 (ADC2_CHANNEL_8)"); break;
                case 26:  adc2_channel = ADC2_CHANNEL_9; LOG(INFO, LogName, "GPIO pin set to 26 (ADC2_CHANNEL_9)"); break;
            }
            adc2_config_channel_atten(adc2_channel, ADC_ATTEN_DB_12);
            break;

        default:
            LOG(ERROR, LogName, "Invalid GPIO");
            return;
    }

    InputFilter.Reset();
}

void AnalogInputHandler::SetInputFilterTimeConstant(unsigned long filterTimeConstant) {
    InputFilterTimeConstant = filterTimeConstant;
    InputFilter.UpdateFilterTimeConstant(InputFilterTimeConstant);

    InputFilterTimeConstantSet = true;
}

void AnalogInputHandler::SetScaling(float voltageReference, float minVoltage, float maxVoltage, float minEngineeringUnit, float maxEngineeringUnit) {
    InputVoltageReference          = voltageReference;
    InputScalingMinVoltage         = minVoltage;
    InputScalingMaxVoltage         = maxVoltage;
    InputScalingMinEngineeringUnit = minEngineeringUnit;
    InputScalingMaxEngineeringUnit = maxEngineeringUnit;

    VoltageToEngineeringUnitScaleFactor = (InputScalingMaxEngineeringUnit - InputScalingMinEngineeringUnit)/(InputScalingMaxVoltage - InputScalingMinVoltage);
    VoltageToEngineeringUnitOffset      = InputScalingMinEngineeringUnit - VoltageToEngineeringUnitScaleFactor * InputScalingMinVoltage;

    InputScalingSet                = true;
}

void AnalogInputHandler::SetSaturations(float minEngineeringUnit, float maxEngineeringUnit)  {
    InputSaturationMinEngineeringUnit = minEngineeringUnit;
    InputSaturationMaxEngineeringUnit = maxEngineeringUnit;

    InputSaturationLimitsSet          = true;
}


void AnalogInputHandler::UpdateInput() {

    int adc2_value = 0;
    esp_err_t r;

    switch (GPIO) {
        case 36: InputADCValue = (float) adc1_get_raw(ADC1_CHANNEL_0); break;
        case 37: InputADCValue = (float) adc1_get_raw(ADC1_CHANNEL_1); break;
        case 38: InputADCValue = (float) adc1_get_raw(ADC1_CHANNEL_2); break;
        case 39: InputADCValue = (float) adc1_get_raw(ADC1_CHANNEL_3); break;
        case 32: InputADCValue = (float) adc1_get_raw(ADC1_CHANNEL_4); break;
        case 33: InputADCValue = (float) adc1_get_raw(ADC1_CHANNEL_5); break;
        case 34: InputADCValue = (float) adc1_get_raw(ADC1_CHANNEL_6); break;
        case 35: InputADCValue = (float) adc1_get_raw(ADC1_CHANNEL_7); break;

        case 4:  r = adc2_get_raw(ADC2_CHANNEL_0, ADC_WIDTH_BIT_12, &adc2_value); if ( r == ESP_OK ) { InputADCValue = (float) adc2_value; } else if ( r == ESP_ERR_TIMEOUT ) { LOG(ERROR, LogName, " Read error because ADC2 may be used by Wi-Fi"); }break;
        case 0:  r = adc2_get_raw(ADC2_CHANNEL_1, ADC_WIDTH_BIT_12, &adc2_value); if ( r == ESP_OK ) { InputADCValue = (float) adc2_value; } else if ( r == ESP_ERR_TIMEOUT ) { LOG(ERROR, LogName, " Read error because ADC2 may be used by Wi-Fi"); }break;
        case 2:  r = adc2_get_raw(ADC2_CHANNEL_2, ADC_WIDTH_BIT_12, &adc2_value); if ( r == ESP_OK ) { InputADCValue = (float) adc2_value; } else if ( r == ESP_ERR_TIMEOUT ) { LOG(ERROR, LogName, " Read error because ADC2 may be used by Wi-Fi"); }break;
        case 15: r = adc2_get_raw(ADC2_CHANNEL_3, ADC_WIDTH_BIT_12, &adc2_value); if ( r == ESP_OK ) { InputADCValue = (float) adc2_value; } else if ( r == ESP_ERR_TIMEOUT ) { LOG(ERROR, LogName, " Read error because ADC2 may be used by Wi-Fi"); }break;
        case 13: r = adc2_get_raw(ADC2_CHANNEL_4, ADC_WIDTH_BIT_12, &adc2_value); if ( r == ESP_OK ) { InputADCValue = (float) adc2_value; } else if ( r == ESP_ERR_TIMEOUT ) { LOG(ERROR, LogName, " Read error because ADC2 may be used by Wi-Fi"); }break;
        case 12: r = adc2_get_raw(ADC2_CHANNEL_5, ADC_WIDTH_BIT_12, &adc2_value); if ( r == ESP_OK ) { InputADCValue = (float) adc2_value; } else if ( r == ESP_ERR_TIMEOUT ) { LOG(ERROR, LogName, " Read error because ADC2 may be used by Wi-Fi"); }break;
        case 14: r = adc2_get_raw(ADC2_CHANNEL_6, ADC_WIDTH_BIT_12, &adc2_value); if ( r == ESP_OK ) { InputADCValue = (float) adc2_value; } else if ( r == ESP_ERR_TIMEOUT ) { LOG(ERROR, LogName, " Read error because ADC2 may be used by Wi-Fi"); }break;
        case 27: r = adc2_get_raw(ADC2_CHANNEL_7, ADC_WIDTH_BIT_12, &adc2_value); if ( r == ESP_OK ) { InputADCValue = (float) adc2_value; } else if ( r == ESP_ERR_TIMEOUT ) { LOG(ERROR, LogName, " Read error because ADC2 may be used by Wi-Fi"); }break;
        case 25: r = adc2_get_raw(ADC2_CHANNEL_8, ADC_WIDTH_BIT_12, &adc2_value); if ( r == ESP_OK ) { InputADCValue = (float) adc2_value; } else if ( r == ESP_ERR_TIMEOUT ) { LOG(ERROR, LogName, " Read error because ADC2 may be used by Wi-Fi"); }break;
        case 26: r = adc2_get_raw(ADC2_CHANNEL_9, ADC_WIDTH_BIT_12, &adc2_value); if ( r == ESP_OK ) { InputADCValue = (float) adc2_value; } else if ( r == ESP_ERR_TIMEOUT ) { LOG(ERROR, LogName, " Read error because ADC2 may be used by Wi-Fi"); }break;

        default:
            LOG(ERROR, LogName, "ADC read error because of unknown pin");
            return;
    }

    if (InputFilterTimeConstantSet) {
        InputFilteredADCValue = InputFilter.Filter(InputADCValue);
    } else {
        InputFilteredADCValue = InputADCValue;
    }

    if (InputScalingSet) {

        VoltageValue = (InputFilteredADCValue / (float)InputResolution) * InputVoltageReference;

        float engineeringUnitValue = VoltageValue * VoltageToEngineeringUnitScaleFactor + VoltageToEngineeringUnitOffset;

        if (InputSaturationLimitsSet) {
            if      (engineeringUnitValue < InputSaturationMinEngineeringUnit) { EngineeringUnitValue = InputSaturationMinEngineeringUnit; }
            else if (engineeringUnitValue > InputSaturationMaxEngineeringUnit) { EngineeringUnitValue = InputSaturationMaxEngineeringUnit; }
            else                                                               { EngineeringUnitValue = engineeringUnitValue; }
        } else {
            EngineeringUnitValue = 0.0;
        }
    } else {
        VoltageValue = 0.0;
        EngineeringUnitValue = 0.0;
    }
}


String AnalogInputHandler::GetName() {
    return Name;
}

float AnalogInputHandler::GetADCValue() {
    return InputFilteredADCValue;
}

float AnalogInputHandler::GetVoltage() {
    return VoltageValue;
}

float AnalogInputHandler::GetValue() {
    return EngineeringUnitValue;
}

#endif // ANALOG_INPUT_HANDLER
