# Statistics

Statistiche **cumulative** di un segnale, con accumulatori a **memoria costante** (O(1), niente buffer dei campioni). Non è un singleton: istanzia un oggetto per ogni segnale da analizzare.

Calcola **minimo, massimo, media, deviazione standard (sigma), RSD, RMS**, più conteggio e ultimo valore.

## Come funziona

Ogni `Update` accumula soltanto `N`, `Σx`, `Σx²`, `min`, `max` e l'ultimo campione — nessuna divisione né radice. Le statistiche derivate si calcolano **su richiesta** nei getter, così il percorso `Update` (quello ad alta frequenza) resta il più leggero possibile.

## API

```cpp
Statistics Stat;

Stat.Update(1.8f);   // overload: float, double, int, long
Stat.Update(2.1f);
Stat.Update(1.9f);

double mean = Stat.GetMean();
double rms  = Stat.GetRMS();
double sd   = Stat.GetSigma();   // campionaria (N-1)
double rsd  = Stat.GetRSD();     // sigma/|media|*100  [%]
double mn   = Stat.GetMin();
double mx   = Stat.GetMax();
double last = Stat.GetLast();
unsigned long n = Stat.GetCount();

Stat.Reset();        // svuota gli accumulatori
```

## Formule

- media = `Σx / N`
- sigma = `sqrt(|Σx² − media²·N| / (N−1))` — campionaria; l'`ABS` assorbe la cancellazione numerica
- RSD [%] = `sigma / |media| · 100`
- RMS = `sqrt(Σx² / N)`

## Note

- A pancia vuota i getter tornano **0** (sigma/RSD tornano 0 anche con un solo campione: serve N ≥ 2).
- **RSD** è indefinita se `|media|` è troppo piccola: sotto `GetMinMeanForRSD()` (default `1e-9`, impostabile con `SetMinMeanForRSD()`) ritorna 0.
- Accumulatori in **`double`**: preciso e stabile per range/conteggi normali (con `double` la somma-dei-quadrati non ha problemi di cancellazione). I getter ritornano `double`, assegnabile a `float` senza problemi.
- Solo **cumulativo** dall'ultimo `Reset` (nessuna finestra mobile: quella richiederebbe un buffer).

## Dipendenze

Nessuna (solo Arduino).
