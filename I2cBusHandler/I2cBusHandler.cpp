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

void I2cBusHandler::SetClockTime (unsigned long ClockTime)
{
    _ClockTime = ClockTime;
    for (int Index = 0; Index < I2C_BUS_HANDLER_MAX_DEVICES; Index++) {
        if (_Devices[Index].Used) {
            _Devices[Index].Signal.SetClockTime(_ClockTime);
        }
    }
}

// --- Configurazione dispositivi ---

void I2cBusHandler::AddDevice (uint8_t Address, const String& Name)
{
    if (_FindDevice(Address) != -1) {
        LOG(WARNING, "I2cBusHandler", "Dispositivo 0x" + String(Address, HEX) + " già registrato");
        return;
    }

    for (int Index = 0; Index < I2C_BUS_HANDLER_MAX_DEVICES; Index++) {
        if (!_Devices[Index].Used) {
            _Devices[Index].Used    = true;
            _Devices[Index].Address = Address;
            _Devices[Index].Signal.SetName(Name + " (0x" + String(Address, HEX) + ")");
            _Devices[Index].Signal.SetClockTime(_ClockTime);
            _Devices[Index].Signal.SetActivationDelay(I2C_BUS_HANDLER_DEFAULT_AVAILABLE_DELAY);
            _Devices[Index].Signal.SetDeactivationDelay(0);
            _Devices[Index].Signal.Enable();
            LOG(INFO, "I2cBusHandler", "Dispositivo " + Name + " registrato a 0x" + String(Address, HEX));
            return;
        }
    }

    LOG(ERROR, "I2cBusHandler", "Registrazione 0x" + String(Address, HEX) + " fallita: nessuno slot libero");
}

void I2cBusHandler::SetAvailableCallback (uint8_t Address, EdgeCallback Callback)
{
    int Index = _FindDevice(Address);
    if (Index == -1) {
        LOG(ERROR, "I2cBusHandler", "SetAvailableCallback: dispositivo 0x" + String(Address, HEX) + " non registrato");
        return;
    }
    _Devices[Index].Signal.SetActivationCallback(Callback);
}

void I2cBusHandler::SetUnavailableCallback (uint8_t Address, EdgeCallback Callback)
{
    int Index = _FindDevice(Address);
    if (Index == -1) {
        LOG(ERROR, "I2cBusHandler", "SetUnavailableCallback: dispositivo 0x" + String(Address, HEX) + " non registrato");
        return;
    }
    _Devices[Index].Signal.SetDeactivationCallback(Callback);
}

void I2cBusHandler::SetAvailableDelay (uint8_t Address, unsigned long Delay)
{
    int Index = _FindDevice(Address);
    if (Index == -1) {
        LOG(ERROR, "I2cBusHandler", "SetAvailableDelay: dispositivo 0x" + String(Address, HEX) + " non registrato");
        return;
    }
    _Devices[Index].Signal.SetActivationDelay(Delay);
}

void I2cBusHandler::SetUnavailableDelay (uint8_t Address, unsigned long Delay)
{
    int Index = _FindDevice(Address);
    if (Index == -1) {
        LOG(ERROR, "I2cBusHandler", "SetUnavailableDelay: dispositivo 0x" + String(Address, HEX) + " non registrato");
        return;
    }
    _Devices[Index].Signal.SetDeactivationDelay(Delay);
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

// --- Stato / accesso bus ---

bool I2cBusHandler::IsAvailable (uint8_t Address) const
{
    int Index = _FindDevice(Address);
    if (Index == -1) return false;
    return _Devices[Index].Signal.GetFilteredSignal();
}

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
    if (!_Enabled) return;

    // Inizializzazione del bus al primo ciclo da abilitato (Enable è flag-only)
    if (!_Initialized) {
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
        return;
    }

    for (int Index = 0; Index < I2C_BUS_HANDLER_MAX_DEVICES; Index++) {
        if (!_Devices[Index].Used) continue;

        if (TakeBus()) {
            bool Present = _Probe(_Devices[Index].Address);
            GiveBus();
            _Devices[Index].Signal.Update(Present);
        }
    }
}

// --- Internals ---

bool I2cBusHandler::_SetupReady () const
{
    return _SdaPin != 255 && _SclPin != 255;
}

int I2cBusHandler::_FindDevice (uint8_t Address) const
{
    for (int Index = 0; Index < I2C_BUS_HANDLER_MAX_DEVICES; Index++) {
        if (_Devices[Index].Used && _Devices[Index].Address == Address) {
            return Index;
        }
    }
    return -1;
}

bool I2cBusHandler::_Probe (uint8_t Address)
{
    _WirePort->beginTransmission(Address);
    return _WirePort->endTransmission() == 0;
}
