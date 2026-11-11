#pragma once

// Possiede il bus I2C: porta, pin, frequenza, inizializzazione differita e mutex
// condiviso. Offre il sondaggio di un indirizzo e la scansione dell'intero bus.
//
// NON tiene un registro dei dispositivi e NON ne monitora la presenza: quello e'
// compito dei driver, che sanno se le proprie transazioni riescono e possono
// usare Probe() finche' credono il proprio dispositivo assente. Il motivo non e'
// solo di competenze: dall'IDF 5.4 il core Arduino compila esp32-hal-i2c-ng.c,
// dove una scrittura a zero byte diventa i2c_master_probe() sull'handle del BUS
// mentre ogni altra transazione passa da un handle di DISPOSITIVO creato per
// indirizzo. Alternare le due cose sullo stesso indirizzo e' esattamente cio' che
// questa divisione dei compiti rende impossibile: chi sonda non transa, e
// viceversa.

#include <Arduino.h>
#include <Wire.h>
#include <LoggerHandler.h>
#include "freertos/semphr.h"

#define I2C_BUS_HANDLER_MUTEX_MAX_TIME     10    // ms
#define I2C_BUS_HANDLER_FIRST_SCAN_ADDRESS 0x08  // sotto 0x08 e sopra 0x77 gli indirizzi
#define I2C_BUS_HANDLER_LAST_SCAN_ADDRESS  0x77  // sono riservati dalla specifica I2C

class I2cBusHandler {
public:
    static I2cBusHandler& GetInstance ();
    I2cBusHandler (const I2cBusHandler&)            = delete;
    I2cBusHandler& operator= (const I2cBusHandler&) = delete;

    typedef void (*ScanFoundCallback) (uint8_t Address);

    // Configurazione bus
    void SetWirePort  (TwoWire& WirePort);
    void SetSdaPin    (uint8_t  Pin);
    void SetSclPin    (uint8_t  Pin);
    void SetFrequency (uint32_t Frequency);

    // Controllo runtime
    void Enable  ();
    void Disable ();
    bool IsReady () const;   // bus inizializzato e pronto a transare

    // Accesso al bus: i driver racchiudono qui le sequenze che devono restare
    // indivisibili. La singola transazione Wire e' gia' atomica per conto suo.
    bool TakeBus (unsigned long MaxWaitTime = I2C_BUS_HANDLER_MUTEX_MAX_TIME);
    void GiveBus ();

    // Sonda un singolo indirizzo: true se risponde. Prende il mutex da sola.
    bool Probe (uint8_t Address);

    // Sonda tutti gli indirizzi assegnabili e richiama OnFound per ognuno che
    // risponde. Se FoundCount non e' nullo ci scrive quanti ne ha trovati: serve
    // perche' OnFound e' un puntatore a funzione e non puo' catturare un contatore
    // del chiamante. Tiene il mutex per tutta la spazzata; ritorna false se il bus
    // non e' pronto o e' occupato.
    bool Scan (ScanFoundCallback OnFound, uint8_t* FoundCount = nullptr);

    // Diagnostica (sola lettura)
    uint8_t  GetSdaPin    () const;
    uint8_t  GetSclPin    () const;
    uint32_t GetFrequency () const;

    // Chiamato ciclicamente: esegue l'inizializzazione differita del bus
    void Loop ();

private:
    I2cBusHandler ();

    bool _SetupReady () const;
    bool _Probe      (uint8_t Address);   // senza mutex: il chiamante lo ha gia' preso

    TwoWire*          _WirePort    = &Wire;
    uint8_t           _SdaPin      = 255;
    uint8_t           _SclPin      = 255;
    uint32_t          _Frequency   = 100000;
    bool              _Enabled     = false;
    bool              _Initialized = false;
    SemaphoreHandle_t _Mutex       = nullptr;
};

extern I2cBusHandler& I2cBus;
