#pragma once

// Gestione del bus I2C: inizializzazione (pin, frequenza), mutex di bus condiviso
// e monitoraggio della disponibilità dei dispositivi registrati tramite probe
// periodico. Ogni dispositivo ha un DigitalSignalHandler interno: la disponibilità
// è filtrata (ritardo di attivazione di default 250 ms) ed espone callback sui
// fronti di connessione/disconnessione. La comunicazione vera e propria resta a
// carico dei driver dei singoli dispositivi, che devono consultare IsAvailable()
// prima di transare e possono serializzare l'accesso con TakeBus()/GiveBus().

#include <Arduino.h>
#include <Wire.h>
#include <LoggerHandler.h>
#include <DigitalSignalHandler.h>
#include "freertos/semphr.h"

#define I2C_BUS_HANDLER_MAX_DEVICES             8
#define I2C_BUS_HANDLER_DEFAULT_AVAILABLE_DELAY 250   // ms
#define I2C_BUS_HANDLER_MUTEX_MAX_TIME          50    // ms

class I2cBusHandler {
public:
    static I2cBusHandler& GetInstance ();
    I2cBusHandler (const I2cBusHandler&)            = delete;
    I2cBusHandler& operator= (const I2cBusHandler&) = delete;

    // Configurazione bus
    void SetWirePort  (TwoWire& WirePort);
    void SetSdaPin    (uint8_t  Pin);
    void SetSclPin    (uint8_t  Pin);
    void SetFrequency (uint32_t Frequency);
    void SetClockTime (unsigned long ClockTime);

    // Configurazione dispositivi
    void AddDevice              (uint8_t Address, const String& Name);
    void SetAvailableCallback   (uint8_t Address, EdgeCallback  Callback);
    void SetUnavailableCallback (uint8_t Address, EdgeCallback  Callback);
    void SetAvailableDelay      (uint8_t Address, unsigned long Delay);
    void SetUnavailableDelay    (uint8_t Address, unsigned long Delay);

    // Controllo runtime
    void Enable  ();
    void Disable ();

    // Stato / accesso bus
    bool IsAvailable (uint8_t Address) const;
    bool TakeBus     (unsigned long MaxWaitTime = I2C_BUS_HANDLER_MUTEX_MAX_TIME);
    void GiveBus     ();

    // Chiamato ciclicamente
    void Loop ();

private:
    I2cBusHandler ();

    struct I2cDevice {
        uint8_t              Address = 0;
        bool                 Used    = false;
        DigitalSignalHandler Signal;
    };

    bool _SetupReady () const;
    int  _FindDevice (uint8_t Address) const;
    bool _Probe      (uint8_t Address);

    TwoWire*          _WirePort                                = &Wire;
    uint8_t           _SdaPin                                  = 255;
    uint8_t           _SclPin                                  = 255;
    uint32_t          _Frequency                               = 100000;
    unsigned long     _ClockTime                               = 100;   // ms
    bool              _Enabled                                 = false;
    bool              _Initialized                             = false;
    SemaphoreHandle_t _Mutex                                   = nullptr;
    I2cDevice         _Devices[I2C_BUS_HANDLER_MAX_DEVICES];
};

extern I2cBusHandler& I2cBus;
