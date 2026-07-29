#include "SerialConsoleHandler.h"
#include "LoggerHandler.h"
#include "ParametersHandler.h"
#include "System.h"   // GetUptimeUs
#include <ctype.h>   // toupper

SerialConsoleHandler& SerialConsoleHandler::GetInstance () {
    static SerialConsoleHandler Instance;
    return Instance;
}

SerialConsoleHandler& SerialConsole = SerialConsoleHandler::GetInstance();

SerialConsoleHandler::SerialConsoleHandler () {
    _RegisterBuiltins();
}

// --- Configurazione ---

void SerialConsoleHandler::AddMenuItem (const String& Label, std::function<void()> Action) {
    _Items.push_back({ Label, Action });
}

// --- Controllo runtime ---

void SerialConsoleHandler::Enable () {
    if (_Enabled) return;
    _Enabled = true;
}

void SerialConsoleHandler::Disable () {
    if (!_Enabled) return;
    _Enabled = false;
}

// --- Chiamato ciclicamente ---

void SerialConsoleHandler::Loop () {
    if (_Enabled != _PreviousEnabled) {
        // uscendo o entrando ripristino sempre il log live sulla seriale
        _State       = NORMAL;
        _InputBuffer = "";
        Logger.EnableSerial();
        if (_Enabled) LOG(INFO, _LogName, "Console abilitata (premi 'M' o ESC per il menu)");
        _PreviousEnabled = _Enabled;
    }
    if (!_Enabled) return;

    _ProcessInput();
}

// --- Internals ---

void SerialConsoleHandler::_RegisterBuiltins () {
    _Items.push_back({ "Log live",       [this]() { _StartLiveLog(); } });
    _Items.push_back({ "Log da file",    [this]() { _DumpLogFile(); } });
    _Items.push_back({ "Configurazione", [this]() { _DumpConfig(); } });
}

void SerialConsoleHandler::_ProcessInput () {
    while (Serial.available() > 0) {
        char C = (char)Serial.read();

        bool IsMenuKey = (toupper((unsigned char)C) == toupper((unsigned char)SERIAL_CONSOLE_MENU_KEY));
        bool IsExitKey = (C == SERIAL_CONSOLE_EXIT_KEY);

        // Dal log live (NORMAL o VIEW_LIVE) sia 'M' sia ESC aprono il menu
        if (_State == NORMAL || _State == VIEW_LIVE) {
            if (IsMenuKey || IsExitKey) _EnterMenu();
            _InputBuffer = "";
            continue;
        }

        // _State == MENU: ESC torna al log live, i numeri selezionano
        if (IsExitKey) {
            _ExitToNormal();
            _InputBuffer = "";
            continue;
        }
        if (C == '\r' || C == '\n') {
            if (_InputBuffer.length() > 0) {
                int Index = _InputBuffer.toInt() - 1;
                _InputBuffer = "";
                _SelectItem(Index);
            }
        } else if (C == '\b' || C == 127) {
            if (_InputBuffer.length() > 0) {
                _InputBuffer.remove(_InputBuffer.length() - 1);
                Serial.print("\b \b");
            }
        } else if (C >= '0' && C <= '9') {
            _InputBuffer += C;
            Serial.print(C);   // echo
        }
    }
}

void SerialConsoleHandler::_EnterMenu () {
    Logger.DisableSerial();   // ferma il log live sulla seriale (resta su file/WebSerial)
    _State       = MENU;
    _InputBuffer = "";
    _PrintMenu();
}

void SerialConsoleHandler::_ExitToNormal () {
    _State       = NORMAL;
    _InputBuffer = "";
    Logger.EnableSerial();
    Serial.println();
    Serial.println("--- log live (premi 'M' o ESC per il menu) ---");
}

void SerialConsoleHandler::_PrintMenu () {
    Serial.println();
    Serial.println("===== MENU =====");
    Serial.println("Uptime: " + String((unsigned long)(GetUptimeUs() / SECONDS_TO_MICROSECONDS)) + " s");
    for (size_t I = 0; I < _Items.size(); I++) {
        Serial.println("  " + String(I + 1) + ") " + _Items[I].Label);
    }
    Serial.println("  (ESC per uscire)");
    Serial.print("> ");
}

void SerialConsoleHandler::_SelectItem (int Index) {
    if (Index < 0 || Index >= (int)_Items.size()) {
        Serial.println();
        Serial.println("Voce non valida.");
        _PrintMenu();
        return;
    }

    Serial.println();
    _Items[Index].Action();

    // se l'azione non e' entrata in una vista persistente, ristampo il menu
    if (_State == MENU) _PrintMenu();
}

void SerialConsoleHandler::_StartLiveLog () {
    Serial.println("--- log live (premi 'M' o ESC per il menu) ---");
    _State = VIEW_LIVE;
    Logger.EnableSerial();
}

void SerialConsoleHandler::_DumpLogFile () {
    Serial.println("----- LOG SU FILE -----");
    Logger.ReadFullLog([](const char* Line) { Serial.println(Line); });
    Serial.println("----- fine log -----");
}

void SerialConsoleHandler::_DumpConfig () {
    Serial.println("----- CONFIGURAZIONE -----");
    String LastFile = "";
    Parameters.ForEachParameter([&LastFile](const ParametersHandler::ParameterInfo& P) {
        if (P.FilePath != LastFile) {
            Serial.println();
            Serial.println("[" + P.FilePath + "]");
            LastFile = P.FilePath;
        }
        Serial.println("  " + String(P.Id) + "  " + P.Name + " (" + P.Type + ") = " + P.RawValue);
    });
    Serial.println("----- fine -----");
}
