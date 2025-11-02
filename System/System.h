#pragma once

#include <Arduino.h>

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

String GetLibrariesVersion();
void Hibernate(unsigned long long int HibernationTime);
String GetWakeUpReason();
void SetCpuFrequency(unsigned int CpuFrequency);
unsigned int GetCpuFrequency();
