#pragma once

#include <Arduino.h>
#include <vector>
#include <float.h>

class LinearConversion {
public:
    struct Point { float X; float Y; };

    // Configurazione
    void AddPoint (float X, float Y);                      // inserisce ordinato per X
    void SetPoints (std::initializer_list<Point> Points);  // sostituisce tutti i punti
    void SetOutputLimits (float Lower, float Upper);       // saturazione dell'output
    void Clear ();

    // Uso
    float Convert (float X) const;   // interpolazione + estrapolazione "dritta" + saturazione
    bool  IsReady () const;          // almeno un punto

private:
    float _Saturate (float Y) const;

    std::vector<Point> _Points;             // ordinati per X crescente
    float              _LowerLimit = -FLT_MAX;   // default: nessuna saturazione
    float              _UpperLimit =  FLT_MAX;
};
