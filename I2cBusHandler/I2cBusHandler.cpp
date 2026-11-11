#include "I2cBusHandler.h"

I2cBusHandler& I2cBusHandler::GetInstance ()
{
    static I2cBusHandler Instance;
    return Instance;
}

I2cBusHandler& I2cBus = I2cBusHandler::GetInstance();

I2cBusHandler::I2cBusHandler ()
{
    _Mutex = xSemaphoreCreateMutex();
    if (!_Mutex) {
        LOG(ERROR, "I2cBusHandler", "Creazione mutex fallita");
    }
    LOG(INFO, "I2cBusHandler", "Istanza creata");
}

// --- Configurazione bus ---

void I2cBusHandler::SetWirePort (TwoWire& WirePort)
{
    _WirePort = &WirePort;
}

void I2cBusHandler::SetSdaPin (uint8_t Pin)
{
    _SdaPin = Pin;
}

void I2cBusHandler::SetSclPin (uint8_t Pin)
{
    _SclPin = Pin;
}

void I2cBusHandler::SetFrequency (uint32_t Frequency)
{
    _Frequency = Frequency;
}

// --- Controllo runtime ---

void I2cBusHandler::Enable ()
{
    if (_Enabled) return;

    _Enabled = true;
    LOG(INFO, "I2cBusHandler", "Abilitato");
}

void I2cBusHandler::Disable ()
{
    if (!_Enabled) return;

    _Enabled = false;
    LOG(INFO, "I2cBusHandler", "Disabilitato");
}

bool I2cBusHandler::IsReady () const
{
    return _Enabled && _Initialized;
}

// --- Accesso al bus ---

bool I2cBusHandler::TakeBus (unsigned long MaxWaitTime)
{
    if (!_Mutex) return false;
    return xSemaphoreTake(_Mutex, pdMS_TO_TICKS(MaxWaitTime)) == pdTRUE;
}

void I2cBusHandler::GiveBus ()
{
    if (!_Mutex) return;
    xSemaphoreGive(_Mutex);
}

bool I2cBusHandler::Probe (uint8_t Address)
{
    if (!IsReady()) return false;
    if (!TakeBus())  return false;

    bool Present = _Probe(Address);

    GiveBus();
    return Present;
}

bool I2cBusHandler::Scan (ScanFoundCallback OnFound, uint8_t* FoundCount)
{
    if (FoundCount) *FoundCount = 0;

    if (!IsReady()) {
        LOG(ERROR, "I2cBusHandler", "Scan: bus non pronto");
        return false;
    }

    // Il mutex si tiene per tutta la spazzata: prenderlo e rilasciarlo a ogni
    // indirizzo lascerebbe entrare una transazione di un driver a meta' scansione,
    // e la fotografia uscirebbe incoerente.
    if (!TakeBus()) {
        LOG(WARNING, "I2cBusHandler", "Scan: bus occupato");
        return false;
    }

    uint8_t Count = 0;
    for (uint8_t Address = I2C_BUS_HANDLER_FIRST_SCAN_ADDRESS; Address <= I2C_BUS_HANDLER_LAST_SCAN_ADDRESS; Address++) {
        if (!_Probe(Address)) continue;
        Count++;
        if (OnFound) OnFound(Address);
    }

    GiveBus();

    if (FoundCount) *FoundCount = Count;
    return true;
}

// --- Diagnostica ---

uint8_t I2cBusHandler::GetSdaPin () const
{
    return _SdaPin;
}

uint8_t I2cBusHandler::GetSclPin () const
{
    return _SclPin;
}

uint32_t I2cBusHandler::GetFrequency () const
{
    return _Frequency;
}

// --- Chiamato ciclicamente ---

void I2cBusHandler::Loop ()
{
    static bool ErrorLogged = false;
    if (!_SetupReady()) {
        if (!ErrorLogged) {
            LOG(ERROR, "I2cBusHandler", "Loop interrotto: pin non configurati");
            ErrorLogged = true;
        }
        return;
    }
    if (!_Enabled)    return;
    if (_Initialized) return;

    // Inizializzazione del bus al primo ciclo da abilitato (Enable e' flag-only)
    if (_WirePort->begin(_SdaPin, _SclPin, _Frequency)) {
        _Initialized = true;
        LOG(INFO, "I2cBusHandler", "Bus inizializzato (SDA: " + String(_SdaPin) + ", SCL: " + String(_SclPin) +
            ", " + String(_Frequency / 1000) + " kHz)");
    } else {
        static bool InitErrorLogged = false;
        if (!InitErrorLogged) {
            LOG(ERROR, "I2cBusHandler", "Inizializzazione bus fallita");
            InitErrorLogged = true;
        }
    }
}

// --- Internals ---

bool I2cBusHandler::_SetupReady () const
{
    return _SdaPin != 255 && _SclPin != 255;
}

bool I2cBusHandler::_Probe (uint8_t Address)
{
    _WirePort->beginTransmission(Address);
    return _WirePort->endTransmission() == 0;
}
