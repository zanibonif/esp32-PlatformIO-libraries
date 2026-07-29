#include "LinearConversion.h"

// --- Configurazione ---

void LinearConversion::AddPoint (float X, float Y) {
    Point P { X, Y };
    size_t I = 0;
    while (I < _Points.size() && _Points[I].X < X) I++;
    _Points.insert(_Points.begin() + I, P);   // resta sempre ordinato per X
}

void LinearConversion::SetPoints (std::initializer_list<Point> Points) {
    _Points.clear();
    for (const Point& P : Points) AddPoint(P.X, P.Y);
}

void LinearConversion::SetOutputLimits (float Lower, float Upper) {
    _LowerLimit = Lower;
    _UpperLimit = Upper;
}

void LinearConversion::Clear () {
    _Points.clear();
}

// --- Uso ---

float LinearConversion::Convert (float X) const {
    if (_Points.empty())     return _Saturate(0.0f);
    if (_Points.size() == 1) return _Saturate(_Points.front().Y);

    // scegli il segmento: agli estremi usa il primo/ultimo (per "tirare dritto"),
    // altrimenti quello che contiene X
    size_t I;
    if (X <= _Points.front().X)      I = 0;
    else if (X >= _Points.back().X)  I = _Points.size() - 2;
    else {
        I = 0;
        while (I + 1 < _Points.size() && X > _Points[I + 1].X) I++;
    }

    const Point& A = _Points[I];
    const Point& B = _Points[I + 1];
    // la retta del segmento vale anche per X fuori intervallo (estrapola da sola)
    float Y = (B.X == A.X) ? A.Y : A.Y + (B.Y - A.Y) * (X - A.X) / (B.X - A.X);
    return _Saturate(Y);
}

bool LinearConversion::IsReady () const {
    return !_Points.empty();
}

// --- Internals ---

float LinearConversion::_Saturate (float Y) const {
    if (Y < _LowerLimit) return _LowerLimit;
    if (Y > _UpperLimit) return _UpperLimit;
    return Y;
}
