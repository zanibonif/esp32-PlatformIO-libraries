#pragma once

#include <Arduino.h>
#include <functional>
#include <LinearConversion.h>
#include <DigitalSignalHandler.h>
#include <AnalogInputHandler.h>

class DRV8874MotorHandler {
public:
    enum MotorState { DRIVE_DISABLED, DRIVE_ENABLED, MOTOR_RUNNING, MOTOR_STOPPING };

    // Configurazione (setup)
    void SetName (const String& Name);
    void SetClockTime (unsigned long Ms);
    void SetControlPins (int EnablePin, int DirectionPin);    // EN(PWM), PH
    void SetPwm (int LedcChannel, unsigned int FreqHz, int ResolutionBits);   // canale LEDC esplicito (lo gestisce il main)
    void SetSleepPin (int SleepPin);                          // opz. (nSLEEP); -1 se a 3,3V
    void SetFaultPin (int FaultPin);                          // opz. (nFAULT, active-low)
    void SetCurrentSense (int AdcPin, float RIpropiOhm, float AIpropi);  // IPROPI (AnalogInput interno)
    void SetCurrentFilterTimeConstant (unsigned long Ms);
    void SetSpeedRampTime (unsigned long Ms);                 // rampa soft-start/cambio; 0 = istantaneo

    // Anti-blocco (a tempo, soglia da spezzata su |velocità|)
    void SetStallCurve (std::initializer_list<LinearConversion::Point> Points);  // |velocità%| -> A
    void SetStallDelay (unsigned long Ms);                    // sopra soglia per X ms -> stallo
    void SetStallControlDelay (unsigned long Ms);             // attesa "a regime" prima di armare (def. 100)
    void SetOnStallCallback (std::function<void()> Callback);
    void SetOnFaultCallback (std::function<void()> Callback);

    // Controllo runtime: settano variabili d'appoggio; lo stato lo cambia SOLO il Loop().
    void EnableDrive ();                  // _Enabled = true
    void DisableDrive ();                 // _Enabled = false
    void Start (int Percent);             // _RunRequest = true (+ auto-abilita), target = Percent
    void NewSpeedSetPoint (int Percent);  // nuovo target mentre gira
    void Stop ();                         // _RunRequest = false (rampa morbida a 0)
    void ImmediateStop ();                // _ImmediateStopRequest = true (ferma all'istante)
    void EnableStallDetection ();         // abilita il rilevamento dello stallo
    void DisableStallDetection ();        // disabilita il rilevamento dello stallo

    // Diagnostica (non bloccante)
    MotorState  GetState () const;
    const char* GetStateName () const;
    int         GetTargetSpeed () const;      // -100..+100
    int         GetSpeedSetPoint () const;    // -100..+100 (applicato/rampato)
    bool        TargetSpeedReached () const;  // MOTOR_RUNNING && applicato == target
    float       GetCurrent () const;          // A
    bool        IsDriveFault () const;             // vivo (nFAULT)
    bool        IsMotorStalled () const;           // latchato (azzerato da Start/EnableDrive)
    bool        IsDriveEnabled () const;
    bool        IsMotorRunning () const;
    bool        IsStallDetectionEnabled () const;

    // Chiamato ciclicamente
    void Loop ();

private:
    void        _InitHardware ();
    void        _StopOutputs ();
    void        _ReadFault ();
    void        _ReadCurrent ();
    void        _ServiceStall ();
    void        _SlewToward (int Target);
    void        _RecomputeMaxStep (int Target);
    void        _ApplyOutputs ();
    int         _MaxDuty () const;
    const char* _StateName (MotorState S) const;

    String _LogName = "DRV8874Motor";

    unsigned long _ClockTime    = 100;
    int           _EnablePin    = -1;
    int           _DirectionPin = -1;
    int           _SleepPin     = -1;
    int           _FaultPin     = -1;
    int           _AdcPin       = -1;

    unsigned int  _PwmFreq       = 20000;
    int           _PwmResolution = 8;
    int           _PwmChannel    = -1;

    float         _RIpropi         = 1000.0f;
    float         _AIpropi         = 0.00045f;
    unsigned long _CurrentFilterMs = 20;

    unsigned long _RampTimeMs            = 0;
    unsigned long _StallDelayMs          = 0;
    unsigned long _StallControlDelayMs   = 100;
    bool          _StallDetectionEnabled = true;

    std::function<void()> _OnStall = nullptr;
    std::function<void()> _OnFault = nullptr;

    // Variabili d'appoggio settate dai metodi pubblici, lette dal Loop
    bool          _Enabled              = false;
    bool          _RunRequest           = false;
    bool          _ImmediateStopRequest = false;

    MotorState    _State    = DRIVE_DISABLED;   // scritto SOLO dallo switch nel Loop()
    bool          _HwReady  = false;

    int           _TargetSpeed   = 0;        // -100..+100 (desiderato)
    float         _AppliedSpeedF = 0.0f;     // valore rampato (accumulatore float)
    int           _AppliedSpeed  = 0;        // -100..+100 (arrotondato, per uscite e getter)
    float         _MaxStep       = 0.0f;     // variazione massima per tick (slew-rate)
    bool          _TargetChanged = false;

    float         _Current      = 0.0f;      // A
    bool          _Fault        = false;     // vivo
    bool          _Stalled      = false;     // latchato
    unsigned long _ControlTimer = 0;         // ms accumulati "a regime"

    LinearConversion     _StallCurve;
    DigitalSignalHandler _StallDetector;
    AnalogInputHandler   _CurrentInput;
};
