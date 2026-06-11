#include "LittleFSHandler.h"

LittleFSHandler& LittleFSHandler::GetInstance () {
    static LittleFSHandler Instance;
    return Instance;
}
LittleFSHandler& FileSystem = LittleFSHandler::GetInstance();

LittleFSHandler::LittleFSHandler () {}

// --- Configurazione ---

bool LittleFSHandler::Init () {
    if (!LittleFS.begin(true)) {
        LOG(ERROR, _LogName, "Inizializzazione fallita");
        return false;
    }
    LOG(INFO, _LogName, "Inizializzato (usati " + String(UsedBytes()) + " / " + String(TotalBytes()) + " bytes)");
    return true;
}

bool LittleFSHandler::Format () {
    LOG(INFO, _LogName, "Formattazione partizione");
    return LittleFS.format();
}

// --- Operazioni file ---

bool LittleFSHandler::FileExists (const String& Path) {
    return LittleFS.exists(Path);
}

bool LittleFSHandler::WriteFile (const String& Path, const String& Content) {
    LittleFS.remove(Path);
    File F = LittleFS.open(Path, "w");
    if (!F) {
        LOG(ERROR, _LogName, "Apertura file fallita: " + Path);
        return false;
    }
    F.print(Content);
    F.close();
    return true;
}

bool LittleFSHandler::DeleteFile (const String& Path) {
    return LittleFS.remove(Path);
}

File LittleFSHandler::OpenFile (const String& Path, const char* Mode) {
    return LittleFS.open(Path, Mode);
}

// --- Diagnostica ---

size_t LittleFSHandler::TotalBytes () {
    return LittleFS.totalBytes();
}

size_t LittleFSHandler::UsedBytes () {
    return LittleFS.usedBytes();
}

void LittleFSHandler::PrintFiles (const String& Path) {
    File Dir = LittleFS.open(Path);
    if (!Dir) {
        LOG(ERROR, _LogName, "Directory non trovata: " + Path);
        return;
    }
    File Entry = Dir.openNextFile();
    while (Entry) {
        if (Entry.isDirectory()) {
            LOG(INFO, _LogName, "[DIR]  " + String(Entry.name()));
            PrintFiles("/" + String(Entry.name()));
        } else {
            LOG(INFO, _LogName, "[FILE] " + String(Entry.name()) + " (" + String(Entry.size()) + " bytes)");
        }
        Entry = Dir.openNextFile();
    }
}
