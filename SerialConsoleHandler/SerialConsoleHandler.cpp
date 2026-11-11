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

// --- Richiesta di valori all'operatore ---

// Butta quello che e' gia' in coda: e' stato digitato prima che il prompt
// comparisse, quindi non puo' essere una risposta. Tipicamente e' il '\n' del CR/LF
// con cui l'operatore ha selezionato la voce di menu — _ProcessInput() lancia
// l'azione sul '\r' e lascia il '\n' nel buffer. Si svuota due volte a distanza di
// qualche millisecondo perche' i due byte della coppia possono arrivare separati.
void SerialConsoleHandler::_DrainInput () {
    while (Serial.available() > 0) Serial.read();
    delay(20);
    while (Serial.available() > 0) Serial.read();
}

bool SerialConsoleHandler::ReadNumber (const String& Prompt, int MinValue, int MaxValue, int& Value, unsigned long TimeoutMs) {
    _DrainInput();

    Serial.print(Prompt + " [" + String(MinValue) + ".." + String(MaxValue) + "]: ");

    String        Digits    = "";
    unsigned long StartTime = millis();   // attesa di un umano: qui millis() e' la misura giusta

    while ((millis() - StartTime) < TimeoutMs) {
        if (Serial.available() <= 0) {
            delay(1);   // azione manuale: cedere il core mentre si aspetta la tastiera
            continue;
        }

        char C = (char)Serial.read();

        if (C == SERIAL_CONSOLE_EXIT_KEY) {
            Serial.println();
            Serial.println("Annullato");
            return false;
        }

        if (C == '\r' || C == '\n') {
            // A cifre vuote un fine riga e' rumore, non una conferma: il CR/LF puo'
            // arrivare spezzato e il secondo byte presentarsi dopo lo svuotamento
            // iniziale. Per annullare c'e' ESC. Lo stesso vale per il solo segno meno.
            if (Digits.length() == 0 || Digits == "-") continue;

            Serial.println();
            int Parsed = Digits.toInt();
            if (Parsed < MinValue || Parsed > MaxValue) {
                Serial.println("Valore fuori intervallo: " + String(Parsed));
                return false;
            }
            Value = Parsed;
            return true;
        }

        if (C == '\b' || C == 127) {
            if (Digits.length() > 0) {
                Digits.remove(Digits.length() - 1);
                Serial.print("\b \b");
            }
            continue;
        }

        // Segno meno: solo come primo carattere, e solo se l'intervallo ammette
        // valori negativi. Serve per cose come il fuso orario.
        if (C == '-' && Digits.length() == 0 && MinValue < 0) {
            Digits += C;
            Serial.print(C);
            continue;
        }

        if (C >= '0' && C <= '9') {
            Digits += C;
            Serial.print(C);   // echo
        }
    }

    Serial.println();
    Serial.println("Tempo scaduto");
    return false;
}

bool SerialConsoleHandler::ReadText (const String& Prompt, uint8_t MaxLength, String& Value, bool MaskEcho, unsigned long TimeoutMs) {
    _DrainInput();

    Serial.print(Prompt + " [max " + String(MaxLength) + "]: ");

    String        Text      = "";
    unsigned long StartTime = millis();   // attesa di un umano: qui millis() e' la misura giusta

    while ((millis() - StartTime) < TimeoutMs) {
        if (Serial.available() <= 0) {
            delay(1);   // azione manuale: cedere il core mentre si aspetta la tastiera
            continue;
        }

        char C = (char)Serial.read();

        if (C == SERIAL_CONSOLE_EXIT_KEY) {
            Serial.println();
            Serial.println("Annullato");
            return false;
        }

        if (C == '\r' || C == '\n') {
            Serial.println();
            Value = Text;
            return true;
        }

        if (C == '\b' || C == 127) {
            if (Text.length() > 0) {
                Text.remove(Text.length() - 1);
                Serial.print("\b \b");
            }
            continue;
        }

        // ASCII stampabili, spazi compresi: SSID e password possono contenerli
        if (C >= 0x20 && C <= 0x7E) {
            if (Text.length() >= MaxLength) continue;
            Text += C;
            Serial.print(MaskEcho ? '*' : C);
        }
    }

    Serial.println();
    Serial.println("Tempo scaduto");
    return false;
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
