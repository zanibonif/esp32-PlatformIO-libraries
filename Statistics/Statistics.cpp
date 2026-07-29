#include "Statistics.h"
#include <math.h>

// --- Configurazione ---

void Statistics::SetMinMeanForRSD (double MinMean) {
    _MinMeanForRSD = MinMean;
}

double Statistics::GetMinMeanForRSD () const {
    return _MinMeanForRSD;
}

// --- Aggiornamento ---

void Statistics::Reset () {
    _Count      = 0;
    _LastValue  = 0.0;
    _Sum        = 0.0;
    _SquaredSum = 0.0;
    _Min        = 0.0;
    _Max        = 0.0;
}

void Statistics::Update (float  Value) { _Accumulate((double)Value); }
void Statistics::Update (double Value) { _Accumulate(Value); }
void Statistics::Update (int    Value) { _Accumulate((double)Value); }
void Statistics::Update (long   Value) { _Accumulate((double)Value); }

// --- Lettura (calcolo su richiesta) ---

unsigned long Statistics::GetCount () const { return _Count; }
double Statistics::GetLast () const         { return _LastValue; }
double Statistics::GetMin () const          { return (_Count > 0) ? _Min : 0.0; }
double Statistics::GetMax () const          { return (_Count > 0) ? _Max : 0.0; }

double Statistics::GetMean () const {
    return (_Count > 0) ? (_Sum / (double)_Count) : 0.0;
}

double Statistics::GetSigma () const {
    if (_Count < 2) return 0.0;
    double Mean = _Sum / (double)_Count;
    // varianza campionaria (N-1); l'ABS assorbe la cancellazione numerica (radice di un negativo ~0)
    double Variance = fabs(_SquaredSum - Mean * Mean * (double)_Count) / (double)(_Count - 1);
    return sqrt(Variance);
}

double Statistics::GetRSD () const {
    double Mean = GetMean();
    if (_Count < 2 || fabs(Mean) < _MinMeanForRSD) return 0.0;
    return GetSigma() / fabs(Mean) * 100.0;
}

double Statistics::GetRMS () const {
    return (_Count > 0) ? sqrt(_SquaredSum / (double)_Count) : 0.0;
}

// --- Internals ---

void Statistics::_Accumulate (double Value) {
    _LastValue = Value;
    _Count++;
    if (_Count == 1) {
        _Min = Value;
        _Max = Value;
    } else {
        if (Value < _Min) _Min = Value;
        if (Value > _Max) _Max = Value;
    }
    _Sum        += Value;
    _SquaredSum += Value * Value;
}
