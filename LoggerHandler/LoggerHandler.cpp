#include <WebSerial.h>
#include <LittleFS.h>
#include "LoggerHandler.h"

LoggerHandler& LoggerHandler::GetInstance ()
{
    static LoggerHandler Instance;
    return Instance;
}

LoggerHandler& Logger = LoggerHandler::GetInstance();

LoggerHandler::LoggerHandler ()
    : _WebServer(nullptr),
      _WebServerRunning(false),
      _LogEnabled(true),
      _LogQueue(nullptr),
      _DroppedMessages(0)
{
    _WebSerialSemaphore = xSemaphoreCreateMutex();
    _FileLogSemaphore   = xSemaphoreCreateMutex();
    _LogQueue           = xQueueCreate(LOGGER_QUEUE_SIZE, sizeof(LogEntry));
}

// --- Configurazione generale ---

void LoggerHandler::SetDateTimeProvider (DateTimeProvider* Provider)
{
    _TimeProvider = Provider;
}

void LoggerHandler::SetSerialSpeed (unsigned long BaudRate)
{
#if defined(ARDUINO_USB_CDC_ON_BOOT) && ARDUINO_USB_CDC_ON_BOOT
    // USB-CDC nativa: ring buffer TX più capiente prima di begin()
    Serial.setTxBufferSize(LOGGER_USB_TX_BUFFER_SIZE);
#endif
    Serial.begin(BaudRate);
#if defined(ARDUINO_USB_CDC_ON_BOOT) && ARDUINO_USB_CDC_ON_BOOT
    // la write attende che l'host legga invece di scartare byte
    Serial.setTxTimeoutMs(LOGGER_USB_TX_TIMEOUT_MS);
#endif
}

void LoggerHandler::SetMaxMessagesPerCycle (int MaxMessages)
{
    _MaxMessagesPerCycle = MaxMessages;
}

void LoggerHandler::SetClockTime (unsigned long Ms)
{
    _ClockTime = Ms;
}

// --- Configurazione WebSerial ---

void LoggerHandler::SetWebServer (AsyncWebServer* Server)
{
    _WebServer = Server;
}

void LoggerHandler::SetWebServerRunning ()
{
    if (_WebSerialEnabled && _WebServer && !_WebSerialBeginDone) {
        WebSerial.begin(_WebServer);
        _WebSerialBeginDone = true;
    }
    _WebServerRunning = true;
}

void LoggerHandler::SetWebServerNotRunning () {
    _WebServerRunning = false;
}

// --- Controllo runtime ---

void LoggerHandler::Enable () {
    _LogEnabled = true;
}

void LoggerHandler::Disable () {
    _LogEnabled = false;
}

void LoggerHandler::EnableSerial () {
    _SerialEnabled = true;
}

void LoggerHandler::DisableSerial () {
    _SerialEnabled = false;
}

void LoggerHandler::EnableWebSerial () {
    if (_WebServer && !_WebSerialBeginDone) {
        WebSerial.begin(_WebServer);
        _WebSerialBeginDone = true;
    }
    _WebSerialEnabled = true;
}

void LoggerHandler::DisableWebSerial ()
{
    _WebSerialEnabled = false;
}

// --- Logging ---

void LoggerHandler::Log (LogType Type, const String& FunctionName, const String& Message)
{
    if (!_LogEnabled) return;
    if (_LogQueue == nullptr) return;

    LogEntry Entry;
    Entry.Type = Type;

    strncpy(Entry.FunctionName, FunctionName.c_str(), LOGGER_FUNCTION_NAME_MAX_LEN - 1);
    Entry.FunctionName[LOGGER_FUNCTION_NAME_MAX_LEN - 1] = '\0';

    strncpy(Entry.Message, Message.c_str(), LOGGER_MESSAGE_MAX_LEN - 1);
    Entry.Message[LOGGER_MESSAGE_MAX_LEN - 1] = '\0';

    if (FunctionName.length() >= LOGGER_FUNCTION_NAME_MAX_LEN)
        strcpy(&Entry.FunctionName[LOGGER_FUNCTION_NAME_MAX_LEN - 4], "...");

    if (Message.length() >= LOGGER_MESSAGE_MAX_LEN)
        strcpy(&Entry.Message[LOGGER_MESSAGE_MAX_LEN - 4], "...");

    if (xQueueSend(_LogQueue, &Entry, 0) != pdTRUE)
        _DroppedMessages++;
}

void LoggerHandler::LogFromISR (LogType Type, const char* FunctionName, const char* Message)
{
    if (!_LogEnabled) return;
    if (_LogQueue == nullptr) return;

    LogEntry Entry;
    Entry.Type = Type;

    strncpy(Entry.FunctionName, FunctionName, LOGGER_FUNCTION_NAME_MAX_LEN - 1);
    Entry.FunctionName[LOGGER_FUNCTION_NAME_MAX_LEN - 1] = '\0';

    strncpy(Entry.Message, Message, LOGGER_MESSAGE_MAX_LEN - 1);
    Entry.Message[LOGGER_MESSAGE_MAX_LEN - 1] = '\0';

    BaseType_t HigherPriorityTaskWoken = pdFALSE;
    if (xQueueSendFromISR(_LogQueue, &Entry, &HigherPriorityTaskWoken) != pdTRUE)
        _DroppedMessages++;

    if (HigherPriorityTaskWoken == pdTRUE) { portYIELD_FROM_ISR(); }
}

// --- Internals ---

void LoggerHandler::_PublishLog (const char* FormattedText)
{
    if (_SerialEnabled)
    {
        Serial.println(FormattedText);
    }

    if (_WebSerialEnabled && _WebSerialBeginDone && _WebServer && _WebServerRunning)
    {
        if (xSemaphoreTake(_WebSerialSemaphore, pdMS_TO_TICKS(_WebSerialSemaphoreMaxTime)) == pdTRUE)
        {
            WebSerial.println(FormattedText);
            xSemaphoreGive(_WebSerialSemaphore);
        }
    }

    if (_FileLogEnabled)
        _AppendToBuffer(FormattedText);
}

void LoggerHandler::Loop ()
{
    if (_LogQueue == nullptr) return;

    int Dropped = _DroppedMessages.load();
    if (Dropped > 0)
    {
        LogEntry Warning;
        Warning.Type = LogType::Warning;
        strncpy(Warning.FunctionName, "LoggerHandler", LOGGER_FUNCTION_NAME_MAX_LEN - 1);
        snprintf(Warning.Message, LOGGER_MESSAGE_MAX_LEN, "Messaggi persi per queue piena: %d", Dropped);

        if (xQueueSend(_LogQueue, &Warning, 0) == pdTRUE)
            _DroppedMessages.fetch_sub(Dropped);
    }

    LogEntry Entry;
    char FormattedBuffer[LOGGER_FORMATTED_MAX_LEN];
    int ProcessedMessages = 0;

    while (ProcessedMessages < _MaxMessagesPerCycle && xQueueReceive(_LogQueue, &Entry, 0) == pdTRUE)
    {
        _FormatLog(Entry, FormattedBuffer, sizeof(FormattedBuffer));
        _PublishLog(FormattedBuffer);

        if (_FileLogEnabled && (Entry.Type == LogType::Error || Entry.Type == LogType::FatalError))
            _ForceWrite = true;

        ProcessedMessages++;
    }

    _ServiceFileLog();

    if (_WebSerialEnabled && _WebSerialBeginDone && _WebServer && _WebServerRunning)
    {
        if (xSemaphoreTake(_WebSerialSemaphore, pdMS_TO_TICKS(_WebSerialSemaphoreMaxTime)) == pdTRUE)
        {
            WebSerial.loop();
            xSemaphoreGive(_WebSerialSemaphore);
        }
    }
}

void LoggerHandler::_FormatLog (const LogEntry& Entry, char* Buffer, size_t BufferSize)
{
    char TimeString[32] = "";
    if (_TimeProvider) {
        String T = _TimeProvider->GetFormattedTime("%d/%m/%Y %H:%M:%S");
        strncpy(TimeString, T.c_str(), sizeof(TimeString) - 1);
    }

    const char* TypeString;
    switch (Entry.Type) {
        case LogType::Debug:      TypeString = "DEBUG   "; break;
        case LogType::Info:       TypeString = "INFO    "; break;
        case LogType::Warning:    TypeString = "WARNING "; break;
        case LogType::Error:      TypeString = "ERROR   "; break;
        case LogType::FatalError: TypeString = "FATAL   "; break;
        default:                  TypeString = "UNKNOWN "; break;
    }

    if (strlen(TimeString) > 0)
        snprintf(Buffer, BufferSize, "%s | %s | %s: %s", TimeString, TypeString, Entry.FunctionName, Entry.Message);
    else
        snprintf(Buffer, BufferSize, "%s | %s: %s", TypeString, Entry.FunctionName, Entry.Message);
}

// --- Log su file (LittleFS) ---

void LoggerHandler::EnableFileLog () {
    // Filesystem gia' montato esternamente: niente begin(). _File0Size riletta dal
    // disco per il recovery dopo un reset.
    if (xSemaphoreTake(_FileLogSemaphore, portMAX_DELAY) == pdTRUE){
        _EnsureLogDir();

        _File0Size = 0;
        if (LittleFS.exists(LOGGER_FILE_PATH_0))
        {
            File F = LittleFS.open(LOGGER_FILE_PATH_0, FILE_READ);
            if (F) { _File0Size = F.size(); F.close(); }
        }
        xSemaphoreGive(_FileLogSemaphore);
    }

    _FileBufferLen       = 0;
    _FileOverflowPending = false;
    _ForceWrite          = false;
    _WriteTimer          = LOGGER_FILE_WRITE_INTERVAL_MS;
    _FileLogEnabled      = true;
}

void LoggerHandler::DisableFileLog ()
{
    if (!_FileLogEnabled) return;
    _WriteBuffer(/*Blocking=*/true);   // svuota il residuo
    _FileLogEnabled = false;
}

void LoggerHandler::WriteLogFile ()
{
    _WriteBuffer(/*Blocking=*/true);
}

void LoggerHandler::_AppendToBuffer (const char* Text)
{
    size_t Len    = strlen(Text);
    size_t Needed = Len + 1;   // +1 per il '\n'

    if (Needed > LOGGER_FILE_BUFFER_SIZE) return;

    // se non c'e' spazio provo a scrivere subito (non bloccante)
    if (_FileBufferLen + Needed > LOGGER_FILE_BUFFER_SIZE)
        _WriteBuffer(/*Blocking=*/false);

    // ancora pieno (file occupato da un reader): scarto e segnalo una volta sola
    if (_FileBufferLen + Needed > LOGGER_FILE_BUFFER_SIZE)
    {
        if (!_FileOverflowPending)
        {
            _FileOverflowPending = true;
            if (_SerialEnabled)
                Serial.println("[LoggerHandler] log su file: buffer pieno, righe perse");
        }
        return;
    }

    memcpy(_FileBuffer + _FileBufferLen, Text, Len);
    _FileBufferLen += Len;
    _FileBuffer[_FileBufferLen++] = '\n';
}

void LoggerHandler::_ServiceFileLog ()
{
    if (!_FileLogEnabled) return;

    if (_WriteTimer > _ClockTime)
        _WriteTimer = _WriteTimer - _ClockTime;
    else
        _WriteTimer = 0;
    bool WriteTimeout = (_WriteTimer == 0);

    if (_ForceWrite || (WriteTimeout && (_FileBufferLen > 0 || _FileOverflowPending)))
    {
        _WriteBuffer(/*Blocking=*/false);
        _ForceWrite = false;
    }
}

void LoggerHandler::_WriteBuffer (bool Blocking)
{
    if (!_FileLogEnabled) return;
    if (_FileBufferLen == 0 && !_FileOverflowPending) return;

    if (xSemaphoreTake(_FileLogSemaphore, Blocking ? portMAX_DELAY : 0) != pdTRUE)
        return;   // file occupato: riprovo al prossimo Loop()

    _WriteBufferLocked();
    xSemaphoreGive(_FileLogSemaphore);
}

void LoggerHandler::_EnsureLogDir ()
{
    // crea la cartella del log se il path la prevede (LittleFS non la crea da sola)
    String Path = LOGGER_FILE_PATH_0;
    int LastSlash = Path.lastIndexOf('/');
    if (LastSlash > 0)
    {
        String Dir = Path.substring(0, LastSlash);
        if (!LittleFS.exists(Dir)) LittleFS.mkdir(Dir);
    }
}

void LoggerHandler::_WriteBufferLocked ()
{
    if (!_FileLogEnabled) return;
    if (_FileBufferLen == 0 && !_FileOverflowPending) return;

    File F = LittleFS.open(LOGGER_FILE_PATH_0, FILE_APPEND);
    if (!F)
    {
        // forse manca la cartella: la creo e riprovo una volta
        _EnsureLogDir();
        F = LittleFS.open(LOGGER_FILE_PATH_0, FILE_APPEND);
    }
    if (!F)
    {
        _FileLogEnabled = false;
        if (_SerialEnabled)
            Serial.println("[LoggerHandler] log su file disabilitato: apertura file fallita");
        return;
    }

    if (_FileOverflowPending)
    {
        static const char Marker[] = "*** righe perse: buffer log pieno ***\n";
        _File0Size += F.write((const uint8_t*)Marker, sizeof(Marker) - 1);
        _FileOverflowPending = false;
    }

    if (_FileBufferLen > 0)
    {
        _File0Size += F.write((const uint8_t*)_FileBuffer, _FileBufferLen);
        _FileBufferLen = 0;
    }

    F.close();
    _WriteTimer = LOGGER_FILE_WRITE_INTERVAL_MS;

    if (_File0Size >= LOGGER_FILE_MAX_SIZE)
        _RotateFiles();
}

void LoggerHandler::_RotateFiles ()
{
    if (LittleFS.exists(LOGGER_FILE_PATH_1))
        LittleFS.remove(LOGGER_FILE_PATH_1);

    LittleFS.rename(LOGGER_FILE_PATH_0, LOGGER_FILE_PATH_1);
    _File0Size = 0;
}

void LoggerHandler::ReadFullLog (std::function<void(const char*)> OnLine)
{
    if (xSemaphoreTake(_FileLogSemaphore, portMAX_DELAY) != pdTRUE) return;

    _WriteBufferLocked();                   // includi le righe ancora in RAM
    _ReadFile(LOGGER_FILE_PATH_1, OnLine);  // storico
    _ReadFile(LOGGER_FILE_PATH_0, OnLine);  // corrente

    xSemaphoreGive(_FileLogSemaphore);
}

void LoggerHandler::ReadLogFile (int FileIndex, std::function<void(const char*)> OnLine)
{
    const char* Path = (FileIndex == 1) ? LOGGER_FILE_PATH_1 : LOGGER_FILE_PATH_0;

    if (xSemaphoreTake(_FileLogSemaphore, portMAX_DELAY) != pdTRUE) return;

    if (FileIndex == 0)
        _WriteBufferLocked();               // includi le righe ancora in RAM

    _ReadFile(Path, OnLine);

    xSemaphoreGive(_FileLogSemaphore);
}

void LoggerHandler::ClearLogFiles ()
{
    if (xSemaphoreTake(_FileLogSemaphore, portMAX_DELAY) != pdTRUE) return;

    if (LittleFS.exists(LOGGER_FILE_PATH_0)) LittleFS.remove(LOGGER_FILE_PATH_0);
    if (LittleFS.exists(LOGGER_FILE_PATH_1)) LittleFS.remove(LOGGER_FILE_PATH_1);

    _File0Size           = 0;
    _FileBufferLen       = 0;
    _FileOverflowPending = false;

    xSemaphoreGive(_FileLogSemaphore);
}

void LoggerHandler::_ReadFile (const char* Path, std::function<void(const char*)>& OnLine)
{
    if (!LittleFS.exists(Path)) return;

    File F = LittleFS.open(Path, FILE_READ);
    if (!F) return;

    char Line[LOGGER_FORMATTED_MAX_LEN];
    while (F.available())
    {
        size_t N = F.readBytesUntil('\n', Line, sizeof(Line) - 1);
        Line[N]  = '\0';
        OnLine(Line);
    }

    F.close();
}
