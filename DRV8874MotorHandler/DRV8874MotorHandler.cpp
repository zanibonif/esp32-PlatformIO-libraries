#include "DRV8874MotorHandler.h"
#include <LoggerHandler.h>
#include <math.h>
#include <driver/ledc.h>
#include <esp32-hal-periman.h>

// Risoluzione PWM massima realizzabile a FreqHz: e' quella per cui il timer LEDC conta ancora
// 2^bit passi del proprio clock in un periodo, cioe' floor(log2(clock / freq)), limitata dalla
// larghezza del timer del SoC. Il core Arduino 3.x usa il clock XTAL dove il SoC lo supporta
// (C3: 40 MHz), altrimenti APB. Ritorna 0 se la frequenza e' troppo alta per qualsiasi risoluzione.
static int MaxLedcResolution (unsigned int FreqHz) {
    if (FreqHz == 0) return 0;
    unsigned int ClockHz = getApbFrequency();
#ifdef SOC_LEDC_SUPPORT_XTAL_CLOCK
    if (ledcGetClockSource() == LEDC_USE_XTAL_CLK) ClockHz = getXtalFrequencyMhz() * 1000000UL;
#endif
    unsigned int Bits = ledc_find_suitable_duty_resolution(ClockHz, FreqHz);
    // Un bit sotto la larghezza del timer: alla larghezza piena il duty "sempre acceso" (2^bit)
    // non sta nel registro e torna a zero. Verificato a banco: a 14 bit sul C3 la pompa andava
    // solo durante le rampe e si spegneva al 100 %.
    if (Bits > (SOC_LEDC_TIMER_BIT_WIDTH - 1)) Bits = SOC_LEDC_TIMER_BIT_WIDTH - 1;
    return (int)Bits;
}

// Frequenza effettiva del timer LEDC a cui e' collegato il pin, 0 se il pin non e' collegato.
// Non si usa ledcReadFreq() del core perche' ritorna 0 quando il duty e' 0 (lo deduce da
// ledcRead), cioe' proprio a carico fermo, che e' quando la si verifica.
static unsigned int ReadLedcTimerFrequency (int Pin) {
    ledc_channel_handle_t* Bus = (ledc_channel_handle_t*)perimanGetPinBus(Pin, ESP32_BUS_TYPE_LEDC);
    if (Bus == nullptr) return 0;
    return ledc_get_freq((ledc_mode_t)(Bus->channel / SOC_LEDC_CHANNEL_NUM), (ledc_timer_t)Bus->timer_num);
}

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

void DRV8874MotorHandler::SetPwm (unsigned int FreqHz) {
    _PwmFreq       = FreqHz;
    _PwmResolution = MaxLedcResolution(FreqHz);
    _PwmConfigured = true;
    _HwReady = false;
    if (_PwmResolution > 0) {
        LOG(INFO, _LogName, "PWM: " + String(_PwmFreq) + " Hz, risoluzione " + String(_PwmResolution) + " bit");
    } else {
        LOG(ERROR, _LogName, "PWM: " + String(_PwmFreq) + " Hz non realizzabile con nessuna risoluzione");
    }
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
bool  DRV8874MotorHandler::IsCurrentCalibrated () const                { return _CurrentInput.IsCalibrated(); }
bool  DRV8874MotorHandler::IsDriveFault () const                       { return _Fault; }
bool  DRV8874MotorHandler::IsMotorStalled () const                     { return _Stalled; }
bool  DRV8874MotorHandler::IsDriveEnabled () const                     { return (_State != DRIVE_DISABLED); }
bool  DRV8874MotorHandler::IsMotorRunning () const                     { return (_State == MOTOR_RUNNING); }
bool  DRV8874MotorHandler::IsStallDetectionEnabled () const            { return _StallDetectionEnabled; }
unsigned int DRV8874MotorHandler::GetPwmFrequency () const             { return _PwmReady ? ReadLedcTimerFrequency(_EnablePin) : 0; }
int   DRV8874MotorHandler::GetPwmResolution () const                   { return _PwmResolution; }

// --- Macchina a stati ---

void DRV8874MotorHandler::Loop () {

    _ReadFault();   // aggiorna _Fault (vivo) + callback sul fronte

    switch (_State) {

        case DRIVE_DISABLED:
            if (_Enabled && !_HwReady) { _InitHardware(); _HwReady = true; }
            // Con il PWM non disponibile il driver resta qui, in sleep: IsDriveFault() lo
            // segnala. Senza questa condizione nSLEEP oscillerebbe a ogni giro fra qui e
            // DRIVE_ENABLED, che esce su _Fault.
            if (_Enabled && !_PwmFault()) {
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

    _InitPwm();

    if (_AdcPin >= 0) {
        // I = V / (R_IPROPI * A_IPROPI): la scala e' una retta per l'origine, il fondo scala
        // serve solo a fissarne la pendenza. La tensione arriva gia' calibrata.
        const float FullScaleVoltage = 3.3f;
        float MaxAmps = FullScaleVoltage / (_RIpropi * _AIpropi);
        _CurrentInput.SetName(_LogName + "I");
        _CurrentInput.SetClockTime(_ClockTime);
        _CurrentInput.SetGPIO(_AdcPin);
        _CurrentInput.SetScaling(0.0f, FullScaleVoltage, 0.0f, MaxAmps);
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

void DRV8874MotorHandler::_InitPwm () {
    _PwmReady = false;
    if (_EnablePin < 0) return;

    if (!_PwmConfigured) {
        _DisablePwm("PWM non configurato (usare SetPwm)");
        return;
    }

    // ledcAttach fonde configurazione del timer e collegamento del pin: se fallisce non e'
    // garantito in che stato resti il pin, quindi _DisablePwm lo riporta esplicitamente in Hi-Z.
    if (!ledcAttach(_EnablePin, _PwmFreq, _PwmResolution)) {
        _DisablePwm("LEDC: " + String(_PwmFreq) + " Hz a " + String(_PwmResolution) + " bit non realizzabile");
        return;
    }

    // Il core sceglie il timer in base alla coppia (frequenza, risoluzione): una frequenza
    // effettiva diversa puo' venire solo da un timer condiviso o da un errore di configurazione.
    unsigned int ActualFrequency = ReadLedcTimerFrequency(_EnablePin);
    if ((ActualFrequency == 0) || (abs((int)ActualFrequency - (int)_PwmFreq) > (int)(_PwmFreq / 20))) {   // tolleranza 5 %
        _DisablePwm("LEDC: frequenza effettiva " + String(ActualFrequency) + " Hz invece di " + String(_PwmFreq) + " Hz");
        return;
    }

    ledcWrite(_EnablePin, 0);   // EN basso finche' non parte il motore
    _PwmReady = true;
}

// Messa in sicurezza: pin EN in Hi-Z (il pull-down interno del DRV8874 tiene EN basso, sulla
// rete non c'e' una resistenza esterna) e driver in sleep, che porta le uscite in Hi-Z
// indipendentemente da EN. Il guasto e' poi esposto da IsDriveFault().
void DRV8874MotorHandler::_DisablePwm (const String& Reason) {
    LOG(ERROR, _LogName, Reason + ": motore disabilitato");
    ledcDetach(_EnablePin);
    pinMode(_EnablePin, INPUT);
    if (_SleepPin >= 0) digitalWrite(_SleepPin, LOW);
    _PwmReady = false;
}

bool DRV8874MotorHandler::_PwmFault () const {
    return _HwReady && (_EnablePin >= 0) && !_PwmReady;
}

void DRV8874MotorHandler::_StopOutputs () {
    _AppliedSpeedF = 0.0f;
    _AppliedSpeed  = 0;
    _ApplyOutputs();   // EN=0
}

void DRV8874MotorHandler::_ReadFault () {
    if (!_HwReady) return;

    bool DriverFault = (_FaultPin >= 0) && (digitalRead(_FaultPin) == LOW);   // open-drain active-low
    bool NowFault    = DriverFault || _PwmFault();
    if (NowFault && !_Fault) {
        LOG(WARNING, _LogName, DriverFault ? "FAULT (nFAULT: OCP/termico/UVLO)" : "FAULT (PWM non disponibile)");
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
    if (_PwmReady) {
        int Duty = abs(_AppliedSpeed) * _MaxDuty() / 100;   // |vel%| -> duty
        ledcWrite(_EnablePin, Duty);
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
