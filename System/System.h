#pragma once

#include <Arduino.h>
#include <sys/time.h>
#include "DateTimeProvider.h"

// ----------------------------
//    Global constant
// ----------------------------

#define LIBRARIES_VERSION_1            1
#define LIBRARIES_VERSION_2            0
#define LIBRARIES_VERSION_3            0

#define ZERO_TIME                     0

#define SECONDS_TO_MILLISECONDS       1000
#define MILLISECONDS_TO_MICROSECONDS  1000
#define SECONDS_TO_MICROSECONDS       (SECONDS_TO_MILLISECONDS * MILLISECONDS_TO_MICROSECONDS)

// ----------------------------
//    Function declarations
// ----------------------------

String             GetLibrariesVersion ();
void               Hibernate (unsigned long long int HibernationTime);
String             GetWakeUpReason ();
void               SetCpuFrequency (unsigned int CpuFrequency);
unsigned int       GetCpuFrequency ();
void               SetSystemTime (DateTimeProvider& Provider);
unsigned long long GetUptimeUs ();

// ----------------------------
//    Safe arithmetic
// ----------------------------

void SafeIncrement (int&           Value, int           Step = 1);
void SafeIncrement (unsigned int&  Value, unsigned int  Step = 1);
void SafeIncrement (long&          Value, long          Step = 1);
void SafeIncrement (unsigned long& Value, unsigned long Step = 1);

void SafeDecrement (int&           Value, int           Step = 1);
void SafeDecrement (unsigned int&  Value, unsigned int  Step = 1);
void SafeDecrement (long&          Value, long          Step = 1);
void SafeDecrement (unsigned long& Value, unsigned long Step = 1);
