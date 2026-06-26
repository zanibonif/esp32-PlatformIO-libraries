#pragma once
#include <Arduino.h>
#include <vector>
#include <functional>
#include "LoggerHandler.h"

#define PARAMETERS_ENCRYPT_KEY    "ESP32Key"
#define PARAMETERS_ENCRYPT_PREFIX "ENC:"

template<typename T>
struct ParamId {
    const int ID;
    constexpr ParamId (int Id) : ID(Id) {}
};

// Blocca la deduzione di T dal parametro Default: T viene dedotto solo da ParamId<T>
template<typename T> struct ParamIdIdentity         { using Type = T; };
template<typename T> using  NonDeducedT = typename ParamIdIdentity<T>::Type;

class ParametersHandler {
public:
    static ParametersHandler& GetInstance ();
    ParametersHandler (const ParametersHandler&)            = delete;
    ParametersHandler& operator= (const ParametersHandler&) = delete;

    struct FileConfig { String Path; int ID = -1; };

    // Enumerazione di sola lettura (per console/diagnostica)
    struct ParameterInfo {
        int    Id;
        String Name;
        String Type;       // "int","uint","long","ulong","float","bool","string"
        String RawValue;   // come su file: ENC:... se cifrato
        String FilePath;
        bool   Encrypted;
    };

    // Configurazione (setup — prima di Begin())
    void SetClockTime  (unsigned long Ms);
    void SetWriteDelay (unsigned long Ms);
    void AddFile       (FileConfig& File);

    template<typename T>
    void AddParameter (ParamId<T> Param, const String& Name, const NonDeducedT<T>& Default,
                       FileConfig& File, bool Encrypted = false);

    // Avvio sincrono — chiamare in setup() prima di Scheduler.Begin()
    bool Begin ();

    // Get/Set — il tipo è dedotto dal token ParamId<T>, non va mai ripetuto nel chiamante
    template<typename T> T    Get (ParamId<T> Param) const;
    template<typename T> void Set (ParamId<T> Param, const T& Value);

    // Get raw — valore come salvato su file; overload tipato per codice applicativo
    template<typename T> String GetRaw (ParamId<T> Param) const { return GetRaw(Param.ID); }
    String                      GetRaw (int Id)           const;

    // Enumerazione di tutti i parametri, ordinata per file (sola lettura)
    void ForEachParameter (std::function<void(const ParameterInfo&)> OnParam) const;

    // Forza scrittura immediata di tutti i file WritePending (es. prima di reboot/OTA)
    void ForceWrite ();

    // Chiamato ciclicamente (task lento)
    void Loop ();

private:
    ParametersHandler ();

    enum ConfigType {
        INT_PARAMETER, UNSIGNED_INT_PARAMETER, LONG_PARAMETER, UNSIGNED_LONG_PARAMETER,
        FLOAT_PARAMETER, BOOL_PARAMETER, STRING_PARAMETER
    };

    struct ParamEntry { int Id; String Name; ConfigType Type; String Value; String Default; int FileId; bool Encrypted; };
    struct FileEntry  { int Id; String Path; bool WritePending = false; unsigned long WriteTimer = 0; };

    void   _AddParameterInternal (int Id, const String& Name, ConfigType Type,
                                  const String& DefaultStr, FileConfig& File, bool Encrypted);
    bool   _LoadFile             (FileEntry& Entry);
    bool   _SaveFile             (FileEntry& Entry);
    void   _MarkWritePending     (int FileId);
    String _BuildCsv             (int FileId) const;
    String _TypeName             (ConfigType Type) const;
    bool   _ParseLine            (const String& Line, int& OutId, String& OutName, String& OutType, String& OutValue) const;
    String _Encrypt              (const String& Value) const;
    String _Decrypt              (const String& Raw)   const;
    void   _SetValue             (int Id, ConfigType ExpectedType, const String& Serialized, const String& TypeName);
    bool   _GetValue             (int Id, ConfigType ExpectedType, const String& TypeName, String& OutValue) const;

    ParamEntry*       _FindParam (int Id);
    const ParamEntry* _FindParam (int Id) const;
    FileEntry*        _FindFile  (int Id);
    const FileEntry*  _FindFile  (int Id) const;

    std::vector<ParamEntry> _Params;
    std::vector<FileEntry>  _Files;
    bool                    _Ready         = false;
    int                     _NextFileId    = 0;
    unsigned long           _ClockTime     = 0;
    unsigned long           _WriteDelayMs  = 500;
    String                  _LogName       = "ParametersHandler";
};

extern ParametersHandler& Parameters;

// Explicit specialization declarations — definite in .cpp
template<> int           ParametersHandler::Get<int>          (ParamId<int>           Param) const;
template<> unsigned int  ParametersHandler::Get<unsigned int> (ParamId<unsigned int>  Param) const;
template<> long          ParametersHandler::Get<long>         (ParamId<long>          Param) const;
template<> unsigned long ParametersHandler::Get<unsigned long>(ParamId<unsigned long> Param) const;
template<> float         ParametersHandler::Get<float>        (ParamId<float>         Param) const;
template<> bool          ParametersHandler::Get<bool>         (ParamId<bool>          Param) const;
template<> String        ParametersHandler::Get<String>       (ParamId<String>        Param) const;

template<> void ParametersHandler::Set<int>          (ParamId<int>           Param, const int&           Value);
template<> void ParametersHandler::Set<unsigned int> (ParamId<unsigned int>  Param, const unsigned int&  Value);
template<> void ParametersHandler::Set<long>         (ParamId<long>          Param, const long&          Value);
template<> void ParametersHandler::Set<unsigned long>(ParamId<unsigned long> Param, const unsigned long& Value);
template<> void ParametersHandler::Set<float>        (ParamId<float>         Param, const float&         Value);
template<> void ParametersHandler::Set<bool>         (ParamId<bool>          Param, const bool&          Value);
template<> void ParametersHandler::Set<String>       (ParamId<String>        Param, const String&        Value);

template<> void ParametersHandler::AddParameter<int>          (ParamId<int>           Param, const String& Name, const int&           Default, FileConfig& File, bool Encrypted);
template<> void ParametersHandler::AddParameter<unsigned int> (ParamId<unsigned int>  Param, const String& Name, const unsigned int&  Default, FileConfig& File, bool Encrypted);
template<> void ParametersHandler::AddParameter<long>         (ParamId<long>          Param, const String& Name, const long&          Default, FileConfig& File, bool Encrypted);
template<> void ParametersHandler::AddParameter<unsigned long>(ParamId<unsigned long> Param, const String& Name, const unsigned long& Default, FileConfig& File, bool Encrypted);
template<> void ParametersHandler::AddParameter<float>        (ParamId<float>         Param, const String& Name, const float&         Default, FileConfig& File, bool Encrypted);
template<> void ParametersHandler::AddParameter<bool>         (ParamId<bool>          Param, const String& Name, const bool&          Default, FileConfig& File, bool Encrypted);
template<> void ParametersHandler::AddParameter<String>       (ParamId<String>        Param, const String& Name, const String&        Default, FileConfig& File, bool Encrypted);
