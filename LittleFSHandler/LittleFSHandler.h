#pragma once
#include <LittleFS.h>
#include <LoggerHandler.h>

class LittleFSHandler {
public:
    static LittleFSHandler& GetInstance ();
    LittleFSHandler (const LittleFSHandler&)            = delete;
    LittleFSHandler& operator= (const LittleFSHandler&) = delete;

    // Configurazione
    bool Init ();
    bool Format ();

    // Operazioni file
    bool   FileExists (const String& Path);
    bool   WriteFile  (const String& Path, const String& Content);
    bool   DeleteFile (const String& Path);
    File   OpenFile   (const String& Path, const char* Mode);

    // Diagnostica
    size_t TotalBytes ();
    size_t UsedBytes  ();
    void   PrintFiles (const String& Path = "/");

private:
    LittleFSHandler ();

    String _LogName = "LittleFSHandler";
};

extern LittleFSHandler& FileSystem;
