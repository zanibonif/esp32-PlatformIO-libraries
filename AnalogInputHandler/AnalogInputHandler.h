#pragma once

#include <driver/adc.h>
#include <TimeDiscreteFilter.h>
#include <LoggerHandler.h>

// Note: on ESP32, ADC2 is shared with WiFi

class AnalogInputHandler {
public:
    AnalogInputHandler ();

    void SetName (String Name);
    void SetClockTime (unsigned long ClockTime);
    void SetGPIO (int Gpio);
    void SetInputFilterTimeConstant (unsigned long FilterTimeConstant);
    void SetScaling (float VoltageReference, float MinVoltage, float MaxVoltage, float MinEngineeringUnit, float MaxEngineeringUnit);
    void SetSaturations (float MinEngineeringUnit, float MaxEngineeringUnit);

    void Update ();

    String GetName ();
    float  GetADCValue ();
    float  GetVoltage ();
    float  GetValue ();

private:
    String             _LogName                             = "AnalogInputHandler";
    String             _Name                                = "";
    unsigned long      _ClockTime                           = 100;   // milliseconds
    int                _GPIO                                = 1;
    unsigned short     _InputResolution                     = 4095;

    bool               _InputFilterTimeConstantSet          = false;
    unsigned long      _InputFilterTimeConstant             = 500;   // milliseconds
    TimeDiscreteFilter _InputFilter;

    bool               _InputScalingSet                     = false;
    float              _InputVoltageReference               = 3.3f;
    float              _InputScalingMinVoltage              = 0.0f;
    float              _InputScalingMaxVoltage              = 3.3f;
    float              _InputScalingMinEngineeringUnit      = 0.0f;
    float              _InputScalingMaxEngineeringUnit      = 100.0f;

    bool               _InputSaturationLimitsSet            = false;
    float              _InputSaturationMinEngineeringUnit   = 0.0f;
    float              _InputSaturationMaxEngineeringUnit   = 100.0f;

    float              _VoltageToEngineeringUnitScaleFactor = 0.0f;
    float              _VoltageToEngineeringUnitOffset      = 0.0f;

    float              _InputADCValue                       = 0.0f;
    float              _InputFilteredADCValue               = 0.0f;
    float              _VoltageValue                        = 0.0f;
    float              _EngineeringUnitValue                = 0.0f;
};
