# LinearConversion

Conversione **lineare a tratti** (spezzata / look-up table interpolata). Dati N punti `{X, Y}`, restituisce `Y` per una qualunque `X` con interpolazione lineare tra i breakpoint, **estrapolazione** oltre gli estremi e **saturazione** dell'output entro due limiti.

Non è un singleton: istanziala dove serve.

## Comportamento

- **Dentro i breakpoint** → interpolazione lineare tra i due punti adiacenti.
- **Sotto il primo punto / sopra l'ultimo** → estrapola "tirando dritto" con la pendenza del primo (risp. ultimo) segmento — **non** appiattisce sull'ultimo `Y`.
- **Output** → saturato a `[Lower, Upper]` (default ±∞ = nessuna saturazione finché non li imposti).

I punti vengono tenuti **ordinati per X** automaticamente; punti con stessa X (gradino verticale) sono ammessi.

## API

```cpp
void  AddPoint (float X, float Y);                      // inserisce ordinato
void  SetPoints (std::initializer_list<Point> Points);  // sostituisce tutti i punti
void  SetOutputLimits (float Lower, float Upper);       // saturazione output
void  Clear ();
float Convert (float X) const;                          // X -> Y
bool  IsReady () const;                                 // almeno un punto
```

## Esempio

```cpp
LinearConversion StallCurve;
StallCurve.SetPoints({ {0, 0.2f}, {25, 0.8f}, {50, 1.6f}, {100, 3.2f} });  // velocità% -> A
StallCurve.SetOutputLimits(0.2f, 4.0f);                                     // mai sotto 0.2A né sopra 4A

float Soglia = StallCurve.Convert(70.0f);   // interpola tra 50 e 100
float Oltre  = StallCurve.Convert(120.0f);  // estrapola oltre 100, poi satura a 4.0
```

## Note

- Tipo `float`. Su chip senza FPU (es. ESP32-C3) il float è in software: per una manciata di conversioni per ciclo è trascurabile; per uso molto intensivo valutare fixed-point.
- Conversione **diretta** (X→Y). L'inversa (Y→X) non è prevista (richiederebbe curva monotona).

## Dipendenze

Nessuna (solo Arduino).
