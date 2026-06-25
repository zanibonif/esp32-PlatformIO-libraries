#pragma once

// Lettura/scrittura dell'RTC hardware DS3231 via I2C. Implementa DateTimeProvider.
// Si appoggia a I2cBusHandler per inizializzazione del bus, mutex e disponibilità
// del modulo: si auto-registra sul bus alla costruzione e non esegue transazioni
// quando il modulo non è disponibile (niente letture spazzatura con modulo scollegato).
// La transazione I2C di lettura avviene SOLO nella Loop() (task lento), che tiene in
// cache l'epoch dell'ultima lettura valida: GetDateTime/GetFormattedTime leggono la
// cache e non sono mai bloccanti per il chiamante. L'inizializzazione del chip
// (begin + gestione lostPower) avviene nella Loop() con modulo disponibile.

#include <Arduino.h>
#include <RTClib.h>
#include "LoggerHandler.h"
#include "DateTimeProvider.h"
#include "I2cBusHandler.h"

#define DS3231_RTC_I2C_ADDRESS 0x68

class DS3231_RtcHandler : public DateTimeProvider {
public:
    static DS3231_RtcHandler& GetInstance ();
    DS3231_RtcHandler (const DS3231_RtcHandler&)            = delete;
    DS3231_RtcHandler& operator= (const DS3231_RtcHandler&) = delete;

    // Configurazione
    void SetDateTime (uint16_t Year, uint8_t Month, uint8_t Day, uint8_t Hour, uint8_t Minute, uint8_t Second);

    // Controllo runtime
    void Enable      ();
    void Disable     ();
    bool IsEnabled   () const;
    bool IsAvailable () const;

    // Metodi principali (non bloccanti: leggono la cache aggiornata dalla Loop)
    DateTime      GetDateTime     ();
    String        GetFormattedTime (const String& Format = "%d/%m/%Y %H:%M:%S") override;
    unsigned long GetEpochTime     () override;

    // Chiamato ciclicamente (task lento: transazione I2C e aggiornamento cache)
    void Loop ();

private:
    DS3231_RtcHandler ();

    RTC_DS3231        _Rtc;
    bool              _Enabled     = false;
    bool              _Initialized = false;
    volatile uint32_t _CachedEpoch = 0;     // 0 = nessuna lettura valida
    String            _LogName     = "RTC";
};

extern DS3231_RtcHandler& Rtc;
