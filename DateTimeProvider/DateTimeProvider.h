#pragma once

#include <Arduino.h>

class DateTimeProvider {
    public:
        virtual String GetFormattedTime(const String& format) = 0;

        virtual ~DateTimeProvider() {}
};

