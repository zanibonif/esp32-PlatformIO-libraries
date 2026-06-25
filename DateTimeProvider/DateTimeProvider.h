#pragma once

#include <Arduino.h>

class DateTimeProvider {
public:
    virtual String        GetFormattedTime (const String& Format) = 0;
    virtual unsigned long GetEpochTime     () = 0;

    virtual ~DateTimeProvider () {}
};
