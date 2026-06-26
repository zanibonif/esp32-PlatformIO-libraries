#include "ParametersHandler.h"
#include <LittleFSHandler.h>
#include <climits>
#include <math.h>

ParametersHandler& ParametersHandler::GetInstance () {
    static ParametersHandler Instance;
    return Instance;
}
ParametersHandler& Parameters = ParametersHandler::GetInstance();

ParametersHandler::ParametersHandler () {
    LOG(INFO, _LogName, "Istanza creata");
}

// --- Configurazione ---

void ParametersHandler::SetClockTime  (unsigned long Ms) { _ClockTime    = Ms; }
void ParametersHandler::SetWriteDelay (unsigned long Ms) { _WriteDelayMs = Ms; }

void ParametersHandler::AddFile (FileConfig& File) {
    if (_Ready) {
        LOG(ERROR, _LogName, "AddFile() chiamato dopo Begin()");
        return;
    }
    if (File.ID != -1) {
        LOG(ERROR, _LogName, "FileConfig già passato ad AddFile(): " + File.Path);
        return;
    }
    for (const FileEntry& F : _Files) {
        if (F.Path == File.Path) {
            LOG(ERROR, _LogName, "Path già registrato: " + File.Path);
            return;
        }
    }

    FileEntry Entry;
    Entry.Id   = _NextFileId++;
    Entry.Path = File.Path;
    _Files.push_back(Entry);
    File.ID = Entry.Id;

    LOG(INFO, _LogName, "File registrato: " + File.Path + " (ID " + String(Entry.Id) + ")");
}

void ParametersHandler::_AddParameterInternal (int Id, const String& Name, ConfigType Type,
                                               const String& DefaultStr, FileConfig& File, bool Encrypted) {
    if (_Ready) {
        LOG(ERROR, _LogName, "AddParameter() chiamato dopo Begin()");
        return;
    }
    if (File.ID == -1) {
        LOG(ERROR, _LogName, "AddParameter() con FileConfig non inizializzato per: " + Name);
        return;
    }
    for (const ParamEntry& P : _Params) {
        if (P.Id == Id) {
            LOG(ERROR, _LogName, "ID parametro duplicato: " + String(Id));
            return;
        }
        if (P.Name == Name && P.FileId == File.ID) {
            LOG(WARNING, _LogName, "Nome parametro duplicato nel file: " + Name);
        }
    }

    ParamEntry Entry;
    Entry.Id        = Id;
    Entry.Name      = Name;
    Entry.Type      = Type;
    Entry.Value     = DefaultStr;
    Entry.Default   = DefaultStr;
    Entry.FileId    = File.ID;
    Entry.Encrypted = Encrypted;
    _Params.push_back(Entry);
}

template<> void ParametersHandler::AddParameter<int>          (ParamId<int>           P, const String& N, const int&           D, FileConfig& F, bool E) { _AddParameterInternal(P.ID, N, INT_PARAMETER,           String(D),          F, E); }
template<> void ParametersHandler::AddParameter<unsigned int> (ParamId<unsigned int>  P, const String& N, const unsigned int&  D, FileConfig& F, bool E) { _AddParameterInternal(P.ID, N, UNSIGNED_INT_PARAMETER,  String(D),          F, E); }
template<> void ParametersHandler::AddParameter<long>         (ParamId<long>          P, const String& N, const long&          D, FileConfig& F, bool E) { _AddParameterInternal(P.ID, N, LONG_PARAMETER,          String(D),          F, E); }
template<> void ParametersHandler::AddParameter<unsigned long>(ParamId<unsigned long> P, const String& N, const unsigned long& D, FileConfig& F, bool E) { _AddParameterInternal(P.ID, N, UNSIGNED_LONG_PARAMETER, String(D),          F, E); }
template<> void ParametersHandler::AddParameter<float>        (ParamId<float>         P, const String& N, const float&         D, FileConfig& F, bool E) { _AddParameterInternal(P.ID, N, FLOAT_PARAMETER,         String(D, 6),       F, E); }
template<> void ParametersHandler::AddParameter<bool>         (ParamId<bool>          P, const String& N, const bool&          D, FileConfig& F, bool E) { _AddParameterInternal(P.ID, N, BOOL_PARAMETER,          D ? "true":"false", F, E); }
template<> void ParametersHandler::AddParameter<String>       (ParamId<String>        P, const String& N, const String&        D, FileConfig& F, bool E) { _AddParameterInternal(P.ID, N, STRING_PARAMETER,        D,                  F, E); }

// --- Avvio ---

bool ParametersHandler::Begin () {
    if (_Files.empty())  LOG(WARNING, _LogName, "Nessun file registrato");
    if (_Params.empty()) LOG(WARNING, _LogName, "Nessun parametro registrato");

    bool AllOk = true;
    for (FileEntry& F : _Files) {
        if (!_LoadFile(F)) AllOk = false;
    }

    _Ready = true;

    for (FileEntry& F : _Files) {
        if (!_SaveFile(F)) AllOk = false;
    }

    LOG(INFO, _LogName, "Pronto (" + String(_Params.size()) + " parametri, " + String(_Files.size()) + " file)");
    return AllOk;
}

// --- Get ---

template<> int           ParametersHandler::Get (ParamId<int>           P) const { String V; return _GetValue(P.ID, INT_PARAMETER,           "int",    V) ? (int)V.toInt()                          : 0;     }
template<> unsigned int  ParametersHandler::Get (ParamId<unsigned int>  P) const { String V; return _GetValue(P.ID, UNSIGNED_INT_PARAMETER,   "uint",   V) ? (unsigned int)strtoul(V.c_str(),nullptr,10) : 0U;  }
template<> long          ParametersHandler::Get (ParamId<long>          P) const { String V; return _GetValue(P.ID, LONG_PARAMETER,           "long",   V) ? V.toInt()                                : 0L;    }
template<> unsigned long ParametersHandler::Get (ParamId<unsigned long> P) const { String V; return _GetValue(P.ID, UNSIGNED_LONG_PARAMETER,  "ulong",  V) ? strtoul(V.c_str(),nullptr,10)            : 0UL;   }
template<> float         ParametersHandler::Get (ParamId<float>         P) const { String V; return _GetValue(P.ID, FLOAT_PARAMETER,          "float",  V) ? V.toFloat()                              : 0.0f;  }
template<> bool          ParametersHandler::Get (ParamId<bool>          P) const { String V; return _GetValue(P.ID, BOOL_PARAMETER,           "bool",   V) ? V == "true"                              : false; }
template<> String        ParametersHandler::Get (ParamId<String>        P) const { String V; return _GetValue(P.ID, STRING_PARAMETER,         "string", V) ? V                                        : "";    }

// --- Set ---

template<> void ParametersHandler::Set (ParamId<int>           P, const int&           V) { _SetValue(P.ID, INT_PARAMETER,          String(V),          "int");    }
template<> void ParametersHandler::Set (ParamId<unsigned int>  P, const unsigned int&  V) { _SetValue(P.ID, UNSIGNED_INT_PARAMETER, String(V),          "uint");   }
template<> void ParametersHandler::Set (ParamId<long>          P, const long&          V) { _SetValue(P.ID, LONG_PARAMETER,         String(V),          "long");   }
template<> void ParametersHandler::Set (ParamId<unsigned long> P, const unsigned long& V) { _SetValue(P.ID, UNSIGNED_LONG_PARAMETER,String(V),          "ulong");  }
template<> void ParametersHandler::Set (ParamId<float>         P, const float&         V) { _SetValue(P.ID, FLOAT_PARAMETER,        String(V, 6),       "float");  }
template<> void ParametersHandler::Set (ParamId<bool>          P, const bool&          V) { _SetValue(P.ID, BOOL_PARAMETER,         V ? "true":"false", "bool");   }
template<> void ParametersHandler::Set (ParamId<String>        P, const String&        V) { _SetValue(P.ID, STRING_PARAMETER,       V,                  "string"); }

// --- Get raw ---

String ParametersHandler::GetRaw (int Id) const {
    if (!_Ready) { LOG(WARNING, _LogName, "GetRaw() prima di Begin()"); return ""; }
    const ParamEntry* P = _FindParam(Id);
    if (!P) { LOG(ERROR, _LogName, "GetRaw(): ID sconosciuto " + String(Id)); return ""; }
    return P->Encrypted
        ? (String(PARAMETERS_ENCRYPT_PREFIX) + _Encrypt(P->Value))
        : P->Value;
}

// --- ForceWrite / Loop ---

void ParametersHandler::ForceWrite () {
    for (FileEntry& F : _Files) {
        if (F.WritePending) _SaveFile(F);
    }
}

void ParametersHandler::Loop () {
    for (FileEntry& F : _Files) {
        if (!F.WritePending) continue;
        if (_ClockTime > 0 && F.WriteTimer > _ClockTime) {
            F.WriteTimer -= _ClockTime;
        } else {
            F.WriteTimer = 0;
        }
        if (F.WriteTimer == 0) {
            _SaveFile(F);
            return;
        }
    }
}

// --- Internals ---

void ParametersHandler::_SetValue (int Id, ConfigType ExpectedType,
                                    const String& Serialized, const String& TypeName) {
    if (!_Ready) { LOG(ERROR, _LogName, "Set() prima di Begin()"); return; }
    ParamEntry* P = _FindParam(Id);
    if (!P)                      { LOG(ERROR, _LogName, "Set(): ID sconosciuto " + String(Id)); return; }
    if (P->Type != ExpectedType) { LOG(ERROR, _LogName, "Set(): tipo errato per " + P->Name + " (atteso " + TypeName + ")"); return; }
    P->Value = Serialized;
    _MarkWritePending(P->FileId);
}

bool ParametersHandler::_GetValue (int Id, ConfigType ExpectedType,
                                    const String& TypeName, String& OutValue) const {
    if (!_Ready) { LOG(WARNING, _LogName, "Get() prima di Begin()"); return false; }
    const ParamEntry* P = _FindParam(Id);
    if (!P)                      { LOG(ERROR, _LogName, "Get(): ID sconosciuto " + String(Id)); return false; }
    if (P->Type != ExpectedType) { LOG(ERROR, _LogName, "Get(): tipo errato per " + P->Name + " (atteso " + TypeName + ")"); return false; }
    OutValue = P->Value;
    return true;
}

bool ParametersHandler::_LoadFile (FileEntry& Entry) {
    if (!FileSystem.FileExists(Entry.Path)) {
        LOG(INFO, _LogName, "File non trovato, creo con valori default: " + Entry.Path);
        return _SaveFile(Entry);
    }

    File F = FileSystem.OpenFile(Entry.Path, "r");
    if (!F) {
        LOG(ERROR, _LogName, "Apertura fallita: " + Entry.Path);
        return false;
    }

    std::vector<int> FoundIds;
    while (F.available()) {
        String Line = F.readStringUntil('\n');
        Line.trim();
        if (Line.isEmpty()) continue;

        int Id; String Name, Type, Value;
        if (!_ParseLine(Line, Id, Name, Type, Value)) continue;

        for (ParamEntry& P : _Params) {
            if (P.FileId != Entry.Id || P.Id != Id) continue;
            P.Value = P.Encrypted ? _Decrypt(Value) : Value;
            FoundIds.push_back(Id);
            break;
        }
    }
    F.close();

    for (const ParamEntry& P : _Params) {
        if (P.FileId != Entry.Id) continue;
        bool Found = false;
        for (int Id : FoundIds) { if (Id == P.Id) { Found = true; break; } }
        if (!Found) {
            LOG(WARNING, _LogName, "'" + P.Name + "' (ID " + String(P.Id) + ") non trovato nel file, uso default: " + P.Default);
        }
    }

    LOG(INFO, _LogName, "Caricato: " + Entry.Path + " (" + String(FoundIds.size()) + " parametri)");
    return true;
}

bool ParametersHandler::_SaveFile (FileEntry& Entry) {
    int LastSlash = Entry.Path.lastIndexOf('/');
    if (LastSlash > 0) {
        String Dir = Entry.Path.substring(0, LastSlash);
        if (!FileSystem.IsDirectory(Dir)) FileSystem.CreateDir(Dir);
    }

    String Csv = _BuildCsv(Entry.Id);
    bool Ok = FileSystem.WriteFile(Entry.Path, Csv);
    if (Ok) {
        Entry.WritePending = false;
    } else {
        LOG(ERROR, _LogName, "Salvataggio fallito: " + Entry.Path);
    }
    return Ok;
}

String ParametersHandler::_TypeName (ConfigType Type) const {
    switch (Type) {
        case INT_PARAMETER:           return "int";
        case UNSIGNED_INT_PARAMETER:  return "uint";
        case LONG_PARAMETER:          return "long";
        case UNSIGNED_LONG_PARAMETER: return "ulong";
        case FLOAT_PARAMETER:         return "float";
        case BOOL_PARAMETER:          return "bool";
        case STRING_PARAMETER:        return "string";
    }
    return "";
}

void ParametersHandler::ForEachParameter (std::function<void(const ParameterInfo&)> OnParam) const {
    if (!OnParam) return;
    for (const FileEntry& F : _Files) {
        for (const ParamEntry& P : _Params) {
            if (P.FileId != F.Id) continue;
            ParameterInfo Info;
            Info.Id        = P.Id;
            Info.Name      = P.Name;
            Info.Type      = _TypeName(P.Type);
            Info.RawValue  = P.Encrypted ? (String(PARAMETERS_ENCRYPT_PREFIX) + _Encrypt(P.Value)) : P.Value;
            Info.FilePath  = F.Path;
            Info.Encrypted = P.Encrypted;
            OnParam(Info);
        }
    }
}

String ParametersHandler::_BuildCsv (int FileId) const {
    String Csv;
    for (const ParamEntry& P : _Params) {
        if (P.FileId != FileId) continue;
        String TypeStr = _TypeName(P.Type);
        String StoredValue = P.Encrypted
            ? (String(PARAMETERS_ENCRYPT_PREFIX) + _Encrypt(P.Value))
            : P.Value;
        Csv += String(P.Id) + "," + P.Name + "," + TypeStr + "," + StoredValue + "\n";
    }
    return Csv;
}

bool ParametersHandler::_ParseLine (const String& Line, int& OutId, String& OutName,
                                     String& OutType, String& OutValue) const {
    int First = Line.indexOf(',');
    if (First < 0) return false;
    int Second = Line.indexOf(',', First + 1);
    if (Second < 0) return false;
    int Third = Line.indexOf(',', Second + 1);
    if (Third < 0) return false;
    OutId    = Line.substring(0, First).toInt();
    OutName  = Line.substring(First + 1, Second);
    OutType  = Line.substring(Second + 1, Third);
    OutValue = Line.substring(Third + 1);
    return OutId > 0 || Line.substring(0, First) == "0";
}

void ParametersHandler::_MarkWritePending (int FileId) {
    FileEntry* F = _FindFile(FileId);
    if (!F) return;
    F->WritePending = true;
    F->WriteTimer   = _WriteDelayMs;
}

String ParametersHandler::_Encrypt (const String& Value) const {
    const char* Key    = PARAMETERS_ENCRYPT_KEY;
    size_t      KeyLen = strlen(Key);
    String      Result;
    for (size_t I = 0; I < (size_t)Value.length(); I++) {
        uint8_t Byte = (uint8_t)Value[I] ^ (uint8_t)Key[I % KeyLen];
        char    Hex[3];
        snprintf(Hex, sizeof(Hex), "%02X", Byte);
        Result += Hex;
    }
    return Result;
}

String ParametersHandler::_Decrypt (const String& Raw) const {
    String Hex = Raw.startsWith(PARAMETERS_ENCRYPT_PREFIX)
        ? Raw.substring(strlen(PARAMETERS_ENCRYPT_PREFIX))
        : Raw;

    const char* Key    = PARAMETERS_ENCRYPT_KEY;
    size_t      KeyLen = strlen(Key);
    String      Result;
    for (size_t I = 0; I + 1 < (size_t)Hex.length(); I += 2) {
        uint8_t Byte = (uint8_t)strtol(Hex.substring(I, I + 2).c_str(), nullptr, 16);
        Result += (char)(Byte ^ (uint8_t)Key[(I / 2) % KeyLen]);
    }
    return Result;
}

ParametersHandler::ParamEntry* ParametersHandler::_FindParam (int Id) {
    for (ParamEntry& P : _Params) { if (P.Id == Id) return &P; }
    return nullptr;
}

const ParametersHandler::ParamEntry* ParametersHandler::_FindParam (int Id) const {
    for (const ParamEntry& P : _Params) { if (P.Id == Id) return &P; }
    return nullptr;
}

ParametersHandler::FileEntry* ParametersHandler::_FindFile (int Id) {
    for (FileEntry& F : _Files) { if (F.Id == Id) return &F; }
    return nullptr;
}

const ParametersHandler::FileEntry* ParametersHandler::_FindFile (int Id) const {
    for (const FileEntry& F : _Files) { if (F.Id == Id) return &F; }
    return nullptr;
}
