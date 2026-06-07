#include "TimeDiscreteFilter.h"

TimeDiscreteFilter::TimeDiscreteFilter () {
    Reset();
}

TimeDiscreteFilter::~TimeDiscreteFilter () {
    if (_Samples) {
        delete[] _Samples;
        _Samples = nullptr;
    }
}

void TimeDiscreteFilter::SetClockTime (unsigned long ClockTime) {
    _ClockTime = ClockTime;
    Reset();
}

void TimeDiscreteFilter::SetFilterType (TimeDiscreteFilterType FilterType) {
    _FilterType = FilterType;
    Reset();
}

void TimeDiscreteFilter::SetFilterTimeConstant (unsigned long FilterTimeConstant) {
    _FilterTimeConstant = FilterTimeConstant;
    _Alpha              = (float)_ClockTime / ((float)_ClockTime + (float)_FilterTimeConstant);
    _OneMinusAlpha      = 1.0f - _Alpha;
    Reset();
}

void TimeDiscreteFilter::SetSamplesNumber (int SamplesNumber) {
    if (SamplesNumber < 1) SamplesNumber = 1;

    if (_Samples) {
        delete[] _Samples;
        _Samples = nullptr;
    }

    _SamplesNumber = SamplesNumber;
    _Samples       = new float[_SamplesNumber];
    Reset();
}

void TimeDiscreteFilter::Reset () {
    _ResetRequest = true;
}

float TimeDiscreteFilter::Filter (float Value) {
    if (_ResetRequest) {
        _FilteredValue = Value;
        _ResetRequest  = false;

        if (_FilterType == MOVING_AVERAGE_FILTER && _Samples) {
            for (int i = 0; i < _SamplesNumber; i++) _Samples[i] = Value;
            _SamplesSum     = Value * (float)_SamplesNumber;
            _NewestSampleId = _SamplesNumber - 1;
        }

        return _FilteredValue;
    }

    if (_FilterType == FIRST_ORDER_FILTER) {
        _FilteredValue = _Alpha * Value + _OneMinusAlpha * _FilteredValue;

    } else if (_FilterType == MOVING_AVERAGE_FILTER && _Samples) {
        int NextId       = (_NewestSampleId + 1) % _SamplesNumber;
        _SamplesSum     += Value - _Samples[NextId];
        _Samples[NextId] = Value;
        _NewestSampleId  = NextId;
        _FilteredValue   = _SamplesSum / (float)_SamplesNumber;

    } else if (_FilterType == NO_FILTER) {
        _FilteredValue = Value;

    } else {
        _FilteredValue = 0.0f;
    }

    return _FilteredValue;
}

float TimeDiscreteFilter::Filter (int Value) {
    return Filter(static_cast<float>(Value));
}
