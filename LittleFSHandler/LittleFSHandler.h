#ifndef LITTLE_FS_HANDLER
#define LITTLE_FS_HANDLER

#include <LittleFS.h>
#include <LoggerHandler.h>

class LittleFSHandler {
    public:

        LittleFSHandler() {}

        bool Init () {
            if (LittleFS.begin(true)) {
                LOG(INFO, LogName, "LittleFS initialized (used " + String(UsedBytes()) + " bytes on a total space of " + String(TotalBytes()) + "bytes)");
                return true;
            } else if (Format()) {
                LOG(INFO, LogName, "LittleFS initialized and cleared (used " + String(UsedBytes()) + " bytes on a total space of " + String(TotalBytes()) + "bytes)");
                return true;
            } else {
                LOG(ERROR, LogName, "LittleFS initialization failed!");
                return false;
            }
        }

        static LittleFSHandler& GetInstance() {
            static LittleFSHandler Instance;
            return Instance;
        }

        bool Format () {
            LOG(INFO, LogName, "Formatting partition");
            return LittleFS.format();
        }

        File OpenFile(const String& Path, const char* Mode) {
            return LittleFS.open(Path, Mode);
        }

        bool FileExists(const String& Path) {
            if (LittleFS.exists(Path)) {
                return true;
            } else {
                return false;
            }
        }

        bool WriteFile(const String& Path, const String& Content) {
            LittleFS.remove(Path);

            File File = LittleFS.open(Path, "w+");
            if (!File) {
                LOG(ERROR, LogName, "Failed to open file " + Path + " for writing");
                return false;
            }

            File.print(Content);
            File.close();

            return true;
        }

        bool DeleteFile(const String& Path) {
            if (LittleFS.remove(Path)) {
                return true;
            } else {
                return false;
            }
        }

        size_t TotalBytes() {
            return LittleFS.totalBytes();
        }

        size_t UsedBytes() {
            return LittleFS.usedBytes();
        }

        void PrintFilesAndDirectories(const String& Path = "/") {
            Serial.println("Listing files in: " + Path);
            File dir = LittleFS.open(Path);
            if (!dir) {
                Serial.println("Failed to open directory");
                return;
            }

            File entry = dir.openNextFile();
            while (entry) {
                if (entry.isDirectory()) {
                    Serial.println("[DIR] " + String(entry.name()));
                } else {
                    Serial.print("[FILE] " + String(entry.name()));
                    Serial.print(" (Size: ");
                    Serial.print(entry.size());
                    Serial.println(" bytes)");
                }
                entry = dir.openNextFile();
                delay(10);
            }
        }



    private:
        String LogName = "LittleFSHandler";
        LittleFSHandler(const LittleFSHandler&) = delete;
        void operator=(const LittleFSHandler&) = delete;

};

#endif // LITTLE_FS_HANDLER