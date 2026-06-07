#pragma once
#include <System.h>

enum TimeDiscreteFilterType {
    NO_FILTER,
    FIRST_ORDER_FILTER,
    MOVING_AVERAGE_FILTER,
};

class TimeDiscreteFilter {
public:
    TimeDiscreteFilter ();
    ~TimeDiscreteFilter ();

    void  SetClockTime (unsigned long ClockTime);
    void  SetFilterType (TimeDiscreteFilterType FilterType);
    void  SetFilterTimeConstant (unsigned long FilterTimeConstant);
    void  SetSamplesNumber (int SamplesNumber);
    void  Reset ();
    float Filter (int Value);
    float Filter (float Value);

private:
    unsigned long          _ClockTime          = 100;
    TimeDiscreteFilterType _FilterType         = NO_FILTER;

    // FIRST_ORDER_FILTER
    unsigned long          _FilterTimeConstant = 0;
    float                  _Alpha              = 0.0f;
    float                  _OneMinusAlpha      = 1.0f;

    // MOVING_AVERAGE_FILTER
    int                    _SamplesNumber  = 1;
    float*                 _Samples        = nullptr;
    int                    _NewestSampleId = 0;
    float                  _SamplesSum     = 0.0f;

    // Stato comune
    float                  _FilteredValue  = 0.0f;
    bool                   _ResetRequest   = true;
};
