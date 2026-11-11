#pragma once

#include <Arduino.h>
#include <TimeDiscreteFilter.h>
#include <LoggerHandler.h>

// Reads through the Arduino 3.x ADC API: the voltage comes from analogReadMilliVolts(), which
// applies the per-chip calibration stored in eFuse (curve fitting on C3/S3, line fitting on
// ESP32/S2). If the calibration turns out to be unavailable the input falls back to the
// nominal linear scale and logs it once.

class AnalogInputHandler {
public:
    AnalogInputHandler ();

    void SetName (String Name);
    void SetClockTime (unsigned long ClockTime);
    void SetGPIO (int Gpio);
    void SetInputFilterTimeConstant (unsigned long FilterTimeConstant);
    void SetScaling (float MinVoltage, float MaxVoltage, float MinEngineeringUnit, float MaxEngineeringUnit);
    void SetSaturations (float MinEngineeringUnit, float MaxEngineeringUnit);

    void Update ();

    String GetName ();
    float  GetADCValue ();     // raw counts of the last sample, 0..4095 (diagnostics)
    float  GetVoltage ();      // V, calibrated and filtered
    float  GetValue ();        // engineering units
    bool   IsCalibrated () const;   // false once the fallback to the nominal scale has kicked in

private:
    // Raw counts above which a 0 mV reading can only mean a missing calibration (~80 mV).
    static const int   CALIBRATION_CHECK_MIN_RAW            = 100;
    // Nominal full scale used only as a fallback without calibration (12 dB attenuation).
    static constexpr float NOMINAL_FULL_SCALE_MILLIVOLTS    = 3300.0f;

    String             _LogName                             = "AnalogInputHandler";
    String             _Name                                = "";
    unsigned long      _ClockTime                           = 100;   // milliseconds
    int                _GPIO                                = 1;
    unsigned short     _InputResolution                     = 4095;

    bool               _AdcConfigured                       = false;
    bool               _CalibrationAvailable                = true;

    bool               _InputFilterTimeConstantSet          = false;
    unsigned long      _InputFilterTimeConstant             = 500;   // milliseconds
    TimeDiscreteFilter _InputFilter;

    bool               _InputScalingSet                     = false;
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
    float              _InputMilliVolts                     = 0.0f;
    float              _InputFilteredMilliVolts             = 0.0f;
    float              _VoltageValue                        = 0.0f;
    float              _EngineeringUnitValue                = 0.0f;
};
