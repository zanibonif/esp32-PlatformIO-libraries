#pragma once
#include <ArduinoOTA.h>
#include <System.h>
#include <LoggerHandler.h>

class CommandOtaHandler {
public:
    static CommandOtaHandler& GetInstance ();
    CommandOtaHandler (const CommandOtaHandler&)            = delete;
    CommandOtaHandler& operator= (const CommandOtaHandler&) = delete;

    // Configurazione
    void SetHostname (const String& Hostname);
    void SetPassword (const String& Password);
    void SetPort (unsigned int Port);

    // Controllo runtime
    void Start ();
    void Stop ();
    bool IsUploadInProgress ();

    // Chiamato ciclicamente
    void Loop ();

private:
    CommandOtaHandler ();

    String       _LogName          = "CommandOtaHandler";
    String       _Hostname         = "UndefinedHostname";
    String       _Password         = "";
    unsigned int _Port             = 3232;
    bool         _Started          = false;
    bool         _UploadInProgress = false;
    unsigned int _LastProgressSent = 0;
};

extern CommandOtaHandler& Ota;
