#include "DRV8874MotorHandler.h"
#include <LoggerHandler.h>
#include <math.h>

// --- Configurazione ---

void DRV8874MotorHandler::SetName (const String& Name) {
    _LogName = Name;
}

void DRV8874MotorHandler::SetClockTime (unsigned long Ms) {
    _ClockTime = Ms;
}

void DRV8874MotorHandler::SetControlPins (int EnablePin, int DirectionPin) {
    _EnablePin    = EnablePin;
    _DirectionPin = DirectionPin;
    _HwReady = false;
}

void DRV8874MotorHandler::SetPwm (int LedcChannel, unsigned int FreqHz, int ResolutionBits) {
    _PwmChannel    = LedcChannel;
    _PwmFreq       = FreqHz;
    _PwmResolution = ResolutionBits;
    _HwReady = false;
}

void DRV8874MotorHandler::SetSleepPin (int SleepPin) {
    _SleepPin = SleepPin;
    _HwReady = false;
}

void DRV8874MotorHandler::SetFaultPin (int FaultPin) {
    _FaultPin = FaultPin;
    _HwReady = false;
}

void DRV8874MotorHandler::SetCurrentSense (int AdcPin, float RIpropiOhm, float AIpropi) {
    _AdcPin  = AdcPin;
    _RIpropi = RIpropiOhm;
    _AIpropi = AIpropi;
    _HwReady = false;
}

void DRV8874MotorHandler::SetCurrentFilterTimeConstant (unsigned long Ms) {
    _CurrentFilterMs = Ms;
    _HwReady = false;
}

void DRV8874MotorHandler::SetSpeedRampTime (unsigned long Ms) {
    _RampTimeMs = Ms;
}

void DRV8874MotorHandler::SetStallCurve (std::initializer_list<LinearConversion::Point> Points) {
    _StallCurve.SetPoints(Points);
}

void DRV8874MotorHandler::SetStallDelay (unsigned long Ms) {
    _StallDelayMs = Ms;
}

void DRV8874MotorHandler::SetStallControlDelay (unsigned long Ms) {
    _StallControlDelayMs = Ms;
}

void DRV8874MotorHandler::EnableStallDetection () {
    _StallDetectionEnabled = true;
}

void DRV8874MotorHandler::DisableStallDetection () {
    _StallDetectionEnabled = false;
}

void DRV8874MotorHandler::SetOnStallCallback (std::function<void()> Callback) {
    _OnStall = Callback;
}

void DRV8874MotorHandler::SetOnFaultCallback (std::function<void()> Callback) {
    _OnFault = Callback;
}

// --- Controllo runtime: settano solo le variabili d'appoggio ---

void DRV8874MotorHandler::EnableDrive () {
    _Enabled = true;
    _Stalled = false;
}

void DRV8874MotorHandler::DisableDrive () {
    _Enabled = false;
}

void DRV8874MotorHandler::Start (int Percent) {
    _TargetSpeed   = constrain(Percent, -100, 100);
    _TargetChanged = true;
    _Stalled       = false;
    _RunRequest    = true;
    _Enabled       = true;   // auto-abilita
}

void DRV8874MotorHandler::NewSpeedSetPoint (int Percent) {
    _TargetSpeed   = constrain(Percent, -100, 100);
    _TargetChanged = true;
}

void DRV8874MotorHandler::Stop () {
    _RunRequest = false;
}

void DRV8874MotorHandler::ImmediateStop () {
    _ImmediateStopRequest = true;
}

// --- Diagnostica ---

DRV8874MotorHandler::MotorState DRV8874MotorHandler::GetState () const { return _State; }
const char* DRV8874MotorHandler::GetStateName () const                 { return _StateName(_State); }
int   DRV8874MotorHandler::GetTargetSpeed () const                     { return _TargetSpeed; }
int   DRV8874MotorHandler::GetSpeedSetPoint () const                   { return _AppliedSpeed; }
bool  DRV8874MotorHandler::TargetSpeedReached () const                 { return ((_State == MOTOR_RUNNING) && (_AppliedSpeed == _TargetSpeed)); }
float DRV8874MotorHandler::GetCurrent () const                         { return _Current; }
bool  DRV8874MotorHandler::IsDriveFault () const                       { return _Fault; }
bool  DRV8874MotorHandler::IsMotorStalled () const                     { return _Stalled; }
bool  DRV8874MotorHandler::IsDriveEnabled () const                     { return (_State != DRIVE_DISABLED); }
bool  DRV8874MotorHandler::IsMotorRunning () const                     { return (_State == MOTOR_RUNNING); }
bool  DRV8874MotorHandler::IsStallDetectionEnabled () const            { return _StallDetectionEnabled; }

// --- Macchina a stati ---

void DRV8874MotorHandler::Loop () {

    _ReadFault();   // aggiorna _Fault (vivo) + callback sul fronte

    switch (_State) {

        case DRIVE_DISABLED:
            if (_Enabled) {
                if (!_HwReady) { _InitHardware(); _HwReady = true; }
                if (_SleepPin >= 0) digitalWrite(_SleepPin, HIGH);   // nSLEEP attivo
                LOG(DEBUG, _LogName, "stato: DRIVE_ENABLED");   // DEBUG (rimuovere)
                _State = DRIVE_ENABLED;
            }
            break;

        case DRIVE_ENABLED:
            if (!_Enabled || _Fault) {
                if (_SleepPin >= 0) digitalWrite(_SleepPin, LOW);    // sleep (Hi-Z)
                LOG(DEBUG, _LogName, "stato: DRIVE_DISABLED");   // DEBUG (rimuovere)
                _State = DRIVE_DISABLED;
            } else if (_RunRequest) {
                _ControlTimer = 0;
                _StallDetector.Reset();
                _RecomputeMaxStep(_TargetSpeed);
                _TargetChanged = false;
                LOG(DEBUG, _LogName, "stato: MOTOR_RUNNING");   // DEBUG (rimuovere)
                _State = MOTOR_RUNNING;
            }
            break;

        case MOTOR_RUNNING:
            if (!_Enabled) {
                _StopOutputs();
                if (_SleepPin >= 0) digitalWrite(_SleepPin, LOW);
                _RunRequest = false;
                LOG(DEBUG, _LogName, "stato: DRIVE_DISABLED");   // DEBUG (rimuovere)
                _State = DRIVE_DISABLED;
            } else if (_ImmediateStopRequest || _Fault || _Stalled) {
                _ImmediateStopRequest = false;
                _RunRequest           = false;
                _StopOutputs();
                LOG(DEBUG, _LogName, "stato: DRIVE_ENABLED (stop immediato)");   // DEBUG (rimuovere)
                _State = DRIVE_ENABLED;
            } else if (!_RunRequest) {
                _RecomputeMaxStep(0);
                LOG(DEBUG, _LogName, "stato: MOTOR_STOPPING");   // DEBUG (rimuovere)
                _State = MOTOR_STOPPING;
            } else {
                _ReadCurrent();
                if (_TargetChanged) { _RecomputeMaxStep(_TargetSpeed); _TargetChanged = false; }
                _SlewToward(_TargetSpeed);
                _ApplyOutputs();
                _ServiceStall();
            }
            break;

        case MOTOR_STOPPING:
            if (!_Enabled) {
                _StopOutputs();
                if (_SleepPin >= 0) digitalWrite(_SleepPin, LOW);
                LOG(DEBUG, _LogName, "stato: DRIVE_DISABLED");   // DEBUG (rimuovere)
                _State = DRIVE_DISABLED;
            } else if (_ImmediateStopRequest || _Fault) {
                _ImmediateStopRequest = false;
                _StopOutputs();
                LOG(DEBUG, _LogName, "stato: DRIVE_ENABLED");   // DEBUG (rimuovere)
                _State = DRIVE_ENABLED;
            } else if (_RunRequest) {
                _ControlTimer = 0;
                _StallDetector.Reset();
                _RecomputeMaxStep(_TargetSpeed);
                _TargetChanged = false;
                LOG(DEBUG, _LogName, "stato: MOTOR_RUNNING");   // DEBUG (rimuovere)
                _State = MOTOR_RUNNING;
            } else {
                _SlewToward(0);
                _ApplyOutputs();
                if (_AppliedSpeed == 0) {
                    LOG(DEBUG, _LogName, "stato: DRIVE_ENABLED");   // DEBUG (rimuovere)
                    _State = DRIVE_ENABLED;
                }
            }
            break;
    }
}

// --- Internals (azioni, non logica di stato) ---

void DRV8874MotorHandler::_InitHardware () {
    if (_DirectionPin >= 0) pinMode(_DirectionPin, OUTPUT);
    if (_SleepPin >= 0)     pinMode(_SleepPin, OUTPUT);
    if (_FaultPin >= 0)     pinMode(_FaultPin, INPUT);   // pull-up esterno 47k

    if (_EnablePin >= 0 && _PwmChannel >= 0) {
        ledcSetup(_PwmChannel, _PwmFreq, _PwmResolution);
        ledcAttachPin(_EnablePin, _PwmChannel);
    } else if (_EnablePin >= 0) {
        LOG(ERROR, _LogName, "PWM non configurato: manca il canale LEDC (usare SetPwm)");
    }

    if (_AdcPin >= 0) {
        const float Vref = 3.3f;
        float MaxAmps = Vref / (_RIpropi * _AIpropi);
        _CurrentInput.SetName(_LogName + "I");
        _CurrentInput.SetClockTime(_ClockTime);
        _CurrentInput.SetGPIO(_AdcPin);
        _CurrentInput.SetScaling(Vref, 0.0f, Vref, 0.0f, MaxAmps);
        _CurrentInput.SetSaturations(0.0f, MaxAmps);
        _CurrentInput.SetInputFilterTimeConstant(_CurrentFilterMs);
    }

    _StallDetector.SetName(_LogName + "Stall");
    _StallDetector.SetClockTime(_ClockTime);
    _StallDetector.SetActivationDelay(_StallDelayMs);
    _StallDetector.SetDeactivationDelay(0);
    _StallDetector.Reset();
    _StallDetector.Enable();
}

void DRV8874MotorHandler::_StopOutputs () {
    _AppliedSpeedF = 0.0f;
    _AppliedSpeed  = 0;
    _ApplyOutputs();   // EN=0
}

void DRV8874MotorHandler::_ReadFault () {
    if (_FaultPin < 0 || !_HwReady) return;

    bool NowFault = (digitalRead(_FaultPin) == LOW);   // open-drain active-low
    if (NowFault && !_Fault) {
        LOG(WARNING, _LogName, "FAULT (nFAULT: OCP/termico/UVLO)");
        if (_OnFault) _OnFault();
    }
    _Fault = NowFault;
}

void DRV8874MotorHandler::_ReadCurrent () {
    if (_AdcPin < 0 || !_HwReady) return;
    _CurrentInput.Update();
    _Current = _CurrentInput.GetValue();   // gia' in Ampere, filtrata
}

void DRV8874MotorHandler::_ServiceStall () {
    if (!_StallDetectionEnabled || (_StallDelayMs == 0) || !_StallCurve.IsReady()) {
        _StallDetector.Update(false);
        return;
    }

    bool Settled = (_AppliedSpeed == _TargetSpeed);   // a regime
    if (Settled) {
        if (_ControlTimer < _StallControlDelayMs) _ControlTimer += _ClockTime;
    } else {
        _ControlTimer = 0;
    }
    bool Armed = Settled && (_ControlTimer >= _StallControlDelayMs);

    float Threshold = _StallCurve.Convert((float)abs(_AppliedSpeed));
    bool  Over = Armed && (_Current > Threshold) && (abs(_AppliedSpeed) > 0);
    _StallDetector.Update(Over);

    if (_StallDetector.GetFilteredSignal() && !_Stalled) {
        _Stalled = true;   // latchato; lo switch fermera' il motore al prossimo giro
        LOG(WARNING, _LogName, "STALLO: I=" + String(_Current, 2) + "A soglia=" + String(Threshold, 2) + "A vel=" + String(_AppliedSpeed));
        if (_OnStall) _OnStall();
    }
}

void DRV8874MotorHandler::_SlewToward (int Target) {
    float Delta = (float)Target - _AppliedSpeedF;
    float Step  = constrain(Delta, -_MaxStep, _MaxStep);   // clamp sul residuo: nessun overshoot
    _AppliedSpeedF += Step;
    _AppliedSpeed   = (int)lroundf(_AppliedSpeedF);
}

void DRV8874MotorHandler::_RecomputeMaxStep (int Target) {
    if (_RampTimeMs == 0) {
        _MaxStep = 200.0f;   // istantaneo: copre l'intero range in un tick
        return;
    }
    float Ticks = (float)_RampTimeMs / (float)_ClockTime;
    if (Ticks < 1.0f) Ticks = 1.0f;
    float Delta = fabsf((float)Target - _AppliedSpeedF);
    _MaxStep = Delta / Ticks;
    if (_MaxStep < 0.0001f) _MaxStep = 200.0f;   // Δ≈0: niente da rampare
}

void DRV8874MotorHandler::_ApplyOutputs () {
    if (_DirectionPin >= 0) digitalWrite(_DirectionPin, (_AppliedSpeed >= 0) ? HIGH : LOW);
    if (_EnablePin >= 0 && _PwmChannel >= 0) {
        int Duty = abs(_AppliedSpeed) * _MaxDuty() / 100;   // |vel%| -> duty
        ledcWrite(_PwmChannel, Duty);
    }
}

int DRV8874MotorHandler::_MaxDuty () const {
    return (1 << _PwmResolution) - 1;
}

const char* DRV8874MotorHandler::_StateName (MotorState S) const {
    switch (S) {
        case DRIVE_DISABLED: return "DRIVE_DISABLED";
        case DRIVE_ENABLED:  return "DRIVE_ENABLED";
        case MOTOR_RUNNING:  return "MOTOR_RUNNING";
        case MOTOR_STOPPING: return "MOTOR_STOPPING";
    }
    return "?";
}
