#include "CommandOtaHandler.h"

CommandOtaHandler& CommandOtaHandler::GetInstance () {
    static CommandOtaHandler Instance;
    return Instance;
}
CommandOtaHandler& Ota = CommandOtaHandler::GetInstance();

CommandOtaHandler::CommandOtaHandler () {
    LOG(INFO, _LogName, "Instance created");
}

// --- Configurazione ---

void CommandOtaHandler::SetHostname (const String& Hostname) {
    _Hostname = Hostname;
    LOG(INFO, _LogName, "Hostname: " + _Hostname);
}

void CommandOtaHandler::SetPassword (const String& Password) {
    _Password = Password;
    LOG(INFO, _LogName, "Password set");
}

void CommandOtaHandler::SetPort (unsigned int Port) {
    _Port = Port;
    LOG(INFO, _LogName, "Port: " + String(_Port));
}

// --- Controllo runtime ---

void CommandOtaHandler::Start () {
    ArduinoOTA.setHostname(_Hostname.c_str());
    if (!_Password.isEmpty()) {
        ArduinoOTA.setPassword(_Password.c_str());
    }
    ArduinoOTA.setPort(_Port);

    ArduinoOTA.onStart([this]() {
        _UploadInProgress = true;
        _LastProgressSent = 0;
        String Type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        LOG(INFO, _LogName, "Update started: " + Type);
    });

    ArduinoOTA.onEnd([this]() {
        _UploadInProgress = false;
        LOG(INFO, _LogName, "Update completed");
    });

    ArduinoOTA.onProgress([this](unsigned int Progress, unsigned int Total) {
        unsigned int CurrentProgress = (Progress * 100) / Total;
        if (((CurrentProgress % 5) == 0) && (CurrentProgress > _LastProgressSent)) {
            _LastProgressSent = CurrentProgress;
            LOG(INFO, _LogName, "Update progress: " + String(CurrentProgress) + "%");
        }
    });

    ArduinoOTA.onError([this](ota_error_t Error) {
        LOG(ERROR, _LogName, "Error [" + String(Error) + "]");
        if      (Error == OTA_AUTH_ERROR)    LOG(ERROR, _LogName, "Auth Failed");
        else if (Error == OTA_BEGIN_ERROR)   LOG(ERROR, _LogName, "Begin Failed");
        else if (Error == OTA_CONNECT_ERROR) LOG(ERROR, _LogName, "Connect Failed");
        else if (Error == OTA_RECEIVE_ERROR) LOG(ERROR, _LogName, "Receive Failed");
        else if (Error == OTA_END_ERROR)     LOG(ERROR, _LogName, "End Failed");
    });

    ArduinoOTA.begin();
    _Started = true;
    LOG(INFO, _LogName, "Started");
}

void CommandOtaHandler::Stop () {
    _Started = false;
    LOG(INFO, _LogName, "Stopped");
}

bool CommandOtaHandler::IsUploadInProgress () {
    return _UploadInProgress;
}

// --- Loop ---

void CommandOtaHandler::Loop () {
    if (!_Started) return;
    ArduinoOTA.handle();
}
