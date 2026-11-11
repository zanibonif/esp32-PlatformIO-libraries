#pragma once

// Lettura/scrittura dell'RTC hardware DS3231 via I2C. Implementa DateTimeProvider.
// Si appoggia a I2cBusHandler per l'inizializzazione del bus e il mutex, ma la
// disponibilita' del modulo se la gestisce da solo:
//
//   modulo presente -> la presenza si deduce dall'esito delle letture vere, senza
//                      traffico aggiuntivo sul bus;
//   modulo assente  -> si sonda l'indirizzo a intervalli (DS3231_RTC_PROBE_INTERVAL)
//                      per accorgersi che e' tornato.
//
// I due rami si escludono, quindi sondaggio e transazioni non si sovrappongono mai:
// sul driver I2C dell'IDF 5.4+ sono due API diverse (i2c_master_probe sull'handle
// del bus contro i2c_master_transmit_receive su quello del dispositivo) e non vanno
// alternate sullo stesso indirizzo.
//
// L'esito grezzo passa per un DigitalSignalHandler, che filtra con ritardi di
// attivazione e disattivazione ed espone le callback sui fronti.
//
// La transazione I2C avviene SOLO nella Loop() (task lento), che tiene in cache
// l'epoch dell'ultima lettura valida: GetDateTime/GetFormattedTime leggono la cache
// e non sono mai bloccanti per il chiamante.

#include <Arduino.h>
#include <RTClib.h>
#include "LoggerHandler.h"
#include "DateTimeProvider.h"
#include "DigitalSignalHandler.h"
#include "I2cBusHandler.h"

#define DS3231_RTC_I2C_ADDRESS            0x68
#define DS3231_RTC_DEFAULT_AVAILABLE_DELAY 250   // ms
#define DS3231_RTC_PROBE_INTERVAL         1000   // ms fra due sondaggi a modulo assente

class DS3231_RtcHandler : public DateTimeProvider {
public:
    static DS3231_RtcHandler& GetInstance ();
    DS3231_RtcHandler (const DS3231_RtcHandler&)            = delete;
    DS3231_RtcHandler& operator= (const DS3231_RtcHandler&) = delete;

    // Configurazione
    void SetClockTime  (unsigned long ClockTime);   // periodo della Loop, serve ai ritardi
    void SetDateTime   (uint16_t Year, uint8_t Month, uint8_t Day, uint8_t Hour, uint8_t Minute, uint8_t Second);

    // Configurazione del segnale di disponibilita'
    void SetAvailableDelay      (unsigned long Delay);
    void SetUnavailableDelay    (unsigned long Delay);
    void SetAvailableCallback   (EdgeCallback Callback);
    void SetUnavailableCallback (EdgeCallback Callback);

    // Controllo runtime
    void Enable      ();
    void Disable     ();
    bool IsEnabled   () const;
    bool IsAvailable () const;

    // Metodi principali (non bloccanti: leggono la cache aggiornata dalla Loop)
    DateTime      GetDateTime      ();
    String        GetFormattedTime (const String& Format = "%d/%m/%Y %H:%M:%S") override;
    unsigned long GetEpochTime     () override;

    // Chiamato ciclicamente (task lento: transazione I2C e aggiornamento cache)
    void Loop ();

private:
    DS3231_RtcHandler ();

    bool _InitializeChip ();   // begin + gestione lostPower, con mutex
    bool _ReadDateTime   ();   // lettura dell'ora, con mutex

    RTC_DS3231           _Rtc;
    bool                 _Enabled      = false;
    bool                 _Initialized  = false;
    volatile uint32_t    _CachedEpoch  = 0;      // 0 = nessuna lettura valida
    unsigned long        _ClockTime    = 100;    // ms
    unsigned long        _ProbeTimer   = 0;      // ms al prossimo sondaggio
    bool                 _LastProbeOk  = false;  // tenuto fra un sondaggio e l'altro
    DigitalSignalHandler _PresenceSignal;
    String               _LogName      = "RTC";
};

extern DS3231_RtcHandler& Rtc;
