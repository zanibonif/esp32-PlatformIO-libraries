#pragma once
#include <LittleFS.h>
#include <LoggerHandler.h>
#include <vector>

struct FileInfo {
    String Name;
    size_t Size;
    bool   IsDir     = false;
    time_t LastWrite = 0;
};

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
    size_t                TotalBytes   ();
    size_t                UsedBytes    ();
    bool                  IsDirectory  (const String& Path);
    void                  PrintFiles   (const String& Path = "/");
    std::vector<FileInfo> ListFiles    (const String& Path = "/");

    // Operazioni cartelle
    bool CreateDir (const String& Path);
    bool DeleteDir (const String& Path);

private:
    LittleFSHandler ();

    String _LogName = "LittleFSHandler";
};

extern LittleFSHandler& FileSystem;
