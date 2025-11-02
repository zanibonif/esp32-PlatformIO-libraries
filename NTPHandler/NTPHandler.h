#pragma once

#include <NTPClient.h>
#include <WiFiUdp.h>
#include <System.h>

typedef void (*TimeSyncCallback)();

class NtpHandler {
private:
    String LogName = "NtpHandler";

    enum NtpStateEnum {
        NOT_CONNECTED,
        CONNECTION_IN_PROGRESS,
        CONNECTED
    };

    WiFiUDP Udp;
    NTPClient NtpClient;
    TaskHandle_t HandlerTaskPointer = nullptr;
    int HandlerTaskPriority = 4;
    unsigned long TaskPeriodMs      = 100;   // milliseconds
    unsigned long ConnectionMaxTime = 10000; // milliseconds
    unsigned long UpdateInterval    = 60000; // milliseconds
    unsigned long FiveSeconds       = 5000;  // milliseconds
    bool Enabled = false;
    bool Connected = false;

    TimeSyncCallback OnSyncCallback = nullptr;
    TimeSyncCallback OnDesyncCallback = nullptr;

    static NtpHandler* StaticInstance;

    NtpHandler();
    static void HandlerTaskStatic(void *pvParameters);
    void HandlerTask();

public:
    static NtpHandler* GetInstance();
    static void Destroy(); // opzionale, se vuoi gestire deallocazione

    void Enable();
    void Disable();
    void SetGmtOffset(int GmtOffsetHours);
    void SetOnSyncCallback(TimeSyncCallback Callback);
    void SetOnDesyncCallback(TimeSyncCallback Callback);
    bool IsConnected();
    String GetFormattedTime(const String& Format = "%H:%M:%S");
};
