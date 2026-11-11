# Changelog

## Non ancora rilasciato

### ⚠️ Cambiamenti incompatibili

- **`I2cBusHandler` 2.0.0 — la presenza dei dispositivi esce dalla libreria.** Rimossi
  `AddDevice()`, `IsAvailable()`, `SetAvailableCallback()`, `SetUnavailableCallback()`,
  `SetAvailableDelay()`, `SetUnavailableDelay()` e `SetClockTime()`. Con essi spariscono il
  registro dei dispositivi, il limite `I2C_BUS_HANDLER_MAX_DEVICES` e il probe periodico.
  La libreria ora **possiede il bus** e nient'altro: porta, pin, frequenza, inizializzazione
  differita, mutex, più `Probe()` su un indirizzo e `Scan()` su tutto il bus.

- **`DS3231_RtcHandler` 2.0.0 — si gestisce la propria presenza.** Tiene un
  `DigitalSignalHandler` suo, alimentato dall'esito delle letture vere quando il modulo è
  presente e da `I2cBus.Probe()` a intervalli quando lo crede assente. Nuovi
  `SetClockTime()`, `SetAvailableDelay()`, `SetUnavailableDelay()`,
  `SetAvailableCallback()`, `SetUnavailableCallback()`. `IsAvailable()` resta identico, quindi
  **il codice applicativo non cambia**: nel firmware Nixie si è spostata una sola riga, da
  `I2cBus.SetClockTime(...)` a `Rtc.SetClockTime(...)`.

**Perché.** Dall'ESP-IDF 5.4 il core Arduino compila `esp32-hal-i2c-ng.c`, sul driver
`i2c_master`: una scrittura a zero byte diventa `i2c_master_probe()` sull'handle del **bus**,
ogni altra transazione passa da un handle di **dispositivo** creato per indirizzo. Il probe
periodico alternava le due API sullo stesso indirizzo dieci volte al secondo. Ora i due casi si
escludono per costruzione — chi sonda non transa, chi transa non sonda — e in regime normale
`i2c_master_probe()` viene chiamata solo da `Scan()`, cioè a comando.

### Aggiunto

- **`LoggerHandler` 1.1.0 — separatore di avvio nel log su file.** `EnableFileLog()` scrive
  `LOGGER_FILE_SEPARATOR` (60 trattini, override-abile da `build_flags`) come primissima riga
  del file, per distinguere i riavvii in un log che si accumula. Solo su file: su Serial e
  WebSerial ogni sessione è già separata. `EnableFileLog()` è diventata idempotente, come
  vuole la convenzione per le `Enable()`.
- **`I2cBusHandler`**: `Probe(Address)`, `Scan(OnFound, FoundCount)`, `IsReady()`,
  `GetSdaPin()`, `GetSclPin()`, `GetFrequency()`.
- **`RadarHandler`** (libreria locale del progetto Nixie): dieci getter sui dati dell'ultimo
  frame — distanze e intensità del bersaglio in movimento e di quello fermo, stato in chiaro,
  versione firmware del sensore, conteggio frame.

## 2.0.0_rc1 — 11/11/2026

Migrazione ad **Arduino-ESP32 3.3.12 / ESP-IDF 5.5.5** (PlatformIO `espressif32@8.0.0`).
Validata a banco sul TritaSarde (ESP32-C3), firmware `2.0.0_rc1`.

### ⚠️ Cambiamenti incompatibili

- **Arduino-ESP32 3.x obbligatorio.** `AnalogInputHandler` e `DRV8874MotorHandler` non compilano
  più su Arduino 2.0.x / `espressif32@6.x`.
- **`AnalogInputHandler::SetScaling`** perde il primo parametro (`VoltageReference`): la tensione
  arriva già calibrata.

  ```cpp
  Input.SetScaling(3.3f, 0.0f, 3.3f, 0.0f, 100.0f);   // 1.x
  Input.SetScaling(0.0f, 3.3f, 0.0f, 100.0f);         // 2.0.0
  ```

  Il fondo scala di un potenziometro va **misurato**: sul C3 a 12 dB la lettura satura intorno a
  2,94 V, quindi con `MaxVoltage = 3.3` il 100 % non si raggiunge mai.
- **`DRV8874MotorHandler::SetPwm`** perde canale e risoluzione: `SetPwm(FreqHz)`.

  ```cpp
  Motor.SetPwm(0, 20000, 8);   // 1.x
  Motor.SetPwm(20000);         // 2.0.0
  ```

### AnalogInputHandler 2.0.0

- Legge con `analogRead` / `analogReadMilliVolts`: la tensione è **calibrata da eFuse** (curve
  fitting su C3/S3, line fitting su ESP32/S2). Il filtro lavora sui mV calibrati.
- Eliminati il driver ADC legacy (`driver/adc.h`, che non può convivere con quello del core) e la
  mappa GPIO → canale scritta a mano: la libreria funziona su qualunque SoC, compreso l'ESP32
  classico.
- Calibrazione assente (raw > 100 con 0 mV): ripiego sulla scala nominale, errore a log una volta,
  nuovo `IsCalibrated()`.
- `GetADCValue()` ritorna i conteggi grezzi dell'ultima lettura (prima erano filtrati).

### DRV8874MotorHandler 2.0.0

- PWM con `ledcAttach`: canale e timer assegnati dal core in base a (frequenza, risoluzione).
- Risoluzione calcolata come massima realizzabile, limitata a un bit sotto la larghezza del timer
  (a larghezza piena il duty "sempre acceso" torna a zero). Scritta a log `INFO`.
- Verifica della frequenza effettiva all'avvio (tolleranza 5 %). Se fallisce: `EN` in Hi-Z,
  `nSLEEP` basso, `IsDriveFault()` vero.
- Nuovi `GetPwmFrequency()`, `GetPwmResolution()`, `IsCurrentCalibrated()`.
- La corrente ora è calibrata: le curve di stallo tarate con la 1.x scattano a una corrente reale
  un po' più alta (sul C3 le letture 1.x erano ~8-9 % sopra il vero).

### System 2.0.0

- `Hibernate()` compila con ESP-IDF 5.x: i domini RTC si spengono solo sui SoC che li hanno. Su
  ESP-IDF 4.4 il comportamento non cambia.
- Tolta la chiamata `esp_sleep_pd_config(ESP_PD_DOMAIN_MAX, ...)`: `ESP_PD_DOMAIN_MAX` è il numero
  di domini, non un dominio.
- `GetLibrariesVersion()` → `"2.0.0"`.

### Trappole incontrate, valide per qualunque progetto su Arduino 3.x

- **`ledcReadFreq()` ritorna 0 quando il duty è 0.** Non usarlo per verificare la frequenza a
  carico fermo.
- **Il `Flash: xx %` di PlatformIO 8.0.0 non è la dimensione dell'immagine.** Il controllo vero è
  la dimensione di `firmware.bin` contro la partizione app: esptool scrive anche un'immagine che
  sfora, invadendo la partizione successiva.
- Il core 3.x pesa circa 400 kB in più del 2.0.17: una partizione app da 1,5 MB può non bastare.

## 1.0.0_build1

Snapshot su Arduino-ESP32 2.0.17 (`espressif32@6.9.0`).
