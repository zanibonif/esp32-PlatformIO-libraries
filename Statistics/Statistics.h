#pragma once

#include <Arduino.h>

class Statistics {
public:
    void Reset ();   // svuota gli accumulatori

    // Aggiornamento con un nuovo campione (overload per vari tipi)
    void Update (float  Value);
    void Update (double Value);
    void Update (int    Value);
    void Update (long   Value);

    // Soglia sulla |media| sotto cui la RSD non e' definita (evita /0)
    void   SetMinMeanForRSD (double MinMean);
    double GetMinMeanForRSD () const;

    // Lettura (sui dati accumulati dall'ultimo Reset)
    unsigned long GetCount () const;
    double GetLast () const;    // ultimo campione visto
    double GetMin () const;
    double GetMax () const;
    double GetMean () const;
    double GetSigma () const;   // deviazione standard campionaria (N-1)
    double GetRSD () const;     // sigma / |media| * 100 [%]; 0 se |media| < soglia
    double GetRMS () const;     // sqrt(media(x^2))

private:
    void _Accumulate (double Value);

    double _MinMeanForRSD = 1e-9;   // soglia per la RSD

    unsigned long _Count      = 0;
    double        _LastValue  = 0.0;
    double        _Sum        = 0.0;   // somma dei campioni
    double        _SquaredSum = 0.0;   // somma dei quadrati
    double        _Min        = 0.0;
    double        _Max        = 0.0;
};
