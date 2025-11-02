#ifndef TIMEDISCRETEFILTER_H
#define TIMEDISCRETEFILTER_H

#include <System.h>

enum TimeDiscreteFilterType {
  NO_FILTER,
  FIRST_ORDER_FILTER,
};

class TimeDiscreteFilter {
    private:

      const float ZERO_DOT_ZERO = 0.0;
      const float ONE_DOT_ZERO = 1.0;

      // General configuration
      unsigned long ClockTime;
      TimeDiscreteFilterType FilterType;

      // FIRST_ORDER_FILTER
      unsigned long FilterTimeConstant;
      float Alpha;
      float OneMinusAlpha;

      // Output
      float FilteredValue;

      // Internal state
      bool ResetRequest;

public:
      TimeDiscreteFilter(unsigned long _ClockTime,
                        TimeDiscreteFilterType _FilterType = FIRST_ORDER_FILTER,
                        unsigned long _FilterTimeConstant = 1);

      void SetClockTime (unsigned long _ClockTime);
      void Reset ();
      void UpdateFilterTimeConstant (unsigned long _FilterTimeConstant);
      float Filter (int _Value);
};


TimeDiscreteFilter::TimeDiscreteFilter(unsigned long _ClockTime,
                                       TimeDiscreteFilterType _FilterType,
                                       unsigned long _FilterTimeConstant) {

    // General configuration
    ClockTime = _ClockTime;
    FilterType = _FilterType;

    // FIRST_ORDER_FILTER
    FilterTimeConstant = _FilterTimeConstant;
    UpdateFilterTimeConstant(FilterTimeConstant);

    Reset();

}

void TimeDiscreteFilter::SetClockTime (unsigned long _ClockTime) {
    ClockTime = _ClockTime;
    Reset();
}

void TimeDiscreteFilter::Reset () {
    ResetRequest = true;
}

void TimeDiscreteFilter::UpdateFilterTimeConstant (unsigned long _FilterTimeConstant) {
    FilterTimeConstant = _FilterTimeConstant;
    Alpha = (float) ClockTime / (ClockTime + FilterTimeConstant);
    OneMinusAlpha = ONE_DOT_ZERO - Alpha;
    ResetRequest = true;
}

float TimeDiscreteFilter::Filter (int _Value) {
    if (ResetRequest) {
        FilteredValue = (float) _Value;
        ResetRequest = false;
    } else if (FilterType == FIRST_ORDER_FILTER) {
        FilteredValue = Alpha * (float) _Value + OneMinusAlpha * FilteredValue;
    } else if (FilterType == NO_FILTER){
        FilteredValue = (float) _Value;
    } else {
        FilteredValue = ZERO_DOT_ZERO;
    }
    return FilteredValue;
}

#endif // TIMEDISCRETEFILTER_H