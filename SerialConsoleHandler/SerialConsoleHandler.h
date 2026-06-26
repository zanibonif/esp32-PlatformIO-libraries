#pragma once

#include <Arduino.h>
#include <vector>
#include <functional>
// LoggerHandler.h e ParametersHandler.h sono inclusi solo nel .cpp (header leggero)

// --- Define configurabili ---
#ifndef SERIAL_CONSOLE_MENU_KEY
#define SERIAL_CONSOLE_MENU_KEY   'M'    // apre il menu da NORMAL
#endif
#ifndef SERIAL_CONSOLE_EXIT_KEY
#define SERIAL_CONSOLE_EXIT_KEY   0x1B   // ESC: "back" universale
#endif

class SerialConsoleHandler {
public:
    static SerialConsoleHandler& GetInstance ();
    SerialConsoleHandler (const SerialConsoleHandler&)            = delete;
    SerialConsoleHandler& operator= (const SerialConsoleHandler&) = delete;

    // Configurazione (setup)
    void AddMenuItem (const String& Label, std::function<void()> Action);

    // Controllo runtime
    void Enable ();
    void Disable ();

    // Chiamato ciclicamente
    void Loop ();

private:
    SerialConsoleHandler ();

    enum ConsoleState { NORMAL, MENU, VIEW_LIVE };

    struct MenuItem {
        String                Label;
        std::function<void()> Action;
    };

    // Setup interno
    void _RegisterBuiltins ();

    // Navigazione / input
    void _ProcessInput ();
    void _EnterMenu ();
    void _ExitToNormal ();
    void _PrintMenu ();
    void _SelectItem (int Index);

    // Azioni built-in
    void _StartLiveLog ();   // entra in VIEW_LIVE (riabilita il log seriale)
    void _DumpLogFile ();    // Logger.ReadFullLog(...) -> torna al menu
    void _DumpConfig ();     // Parameters.ForEachParameter(...) -> torna al menu

    bool                  _Enabled         = false;
    bool                  _PreviousEnabled = false;
    ConsoleState          _State           = NORMAL;
    String                _InputBuffer     = "";
    std::vector<MenuItem> _Items;
    String                _LogName         = "SerialConsole";
};

extern SerialConsoleHandler& SerialConsole;
