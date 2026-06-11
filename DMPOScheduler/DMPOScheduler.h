#pragma once
#include <functional>
#include <vector>
#include <string>
#include <algorithm>
#include "LoggerHandler.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "freertos/semphr.h"

class DMPOScheduler {
public:

    // Singleton
    static DMPOScheduler& GetInstance ();
    DMPOScheduler (const DMPOScheduler&)            = delete;
    DMPOScheduler& operator= (const DMPOScheduler&) = delete;

    // Tipi pubblici
    using TaskFunction  = std::function<void()>;
    using ErrorCallback = std::function<void(const std::string& TaskName, uint32_t OverrunUs)>;

    struct TaskConfig {
        std::string   Name;
        uint32_t      PeriodUs;
        bool          AppCritical;        // se true, pinnato su Core 1
        uint32_t      StackSize;          // default consigliato: 4096
        uint32_t      DeadlineUs;         // se 0, si assume uguale a PeriodUs
        ErrorCallback OnMissedDeadline;

        int           ID;
        BaseType_t    CoreID;
    };

    // Configurazione
    int  AddTask     (TaskConfig& Config);
    bool AddFunction (int TaskID, TaskFunction Fn);

    // Avvio
    bool Begin ();

    // Controllo runtime
    bool EnableTask      (int TaskID);
    bool EnableTask      (const std::string& Name);
    bool DisableTask     (int TaskID);
    bool DisableTask     (const std::string& Name);
    void EnableAllTasks  ();
    void DisableAllTasks ();

    // Diagnostica
    uint32_t GetLastCycleTimeUs     (int TaskID);
    uint32_t GetLastCycleTimeUs     (const std::string& Name);
    uint32_t GetMissedDeadlineCount (int TaskID);
    uint32_t GetMissedDeadlineCount (const std::string& Name);
    void     PrintStatus ();

private:

    DMPOScheduler ();

    struct TaskDescriptor {
        int                       ID;
        std::string               Name;
        uint32_t                  PeriodUs;
        uint32_t                  DeadlineUs;
        bool                      AppCritical;
        BaseType_t                CoreID;
        uint32_t                  StackSize;
        UBaseType_t               Priority;
        bool                      Enabled;
        TaskHandle_t              Handle;
        ErrorCallback             OnMissedDeadline;
        esp_timer_handle_t        TimerHandle;
        SemaphoreHandle_t         TimerSemaphore;
        std::vector<TaskFunction> Functions;
        uint32_t                  LastCycleTimeUs;
        uint32_t                  MissedDeadlineCount;
    };

    void            _AssignDMPOPriorities ();
    static void     _TaskEntryPoint       (void* Param);
    void            _RunTask              (TaskDescriptor& Task);
    TaskDescriptor* _FindTask             (int TaskID);
    TaskDescriptor* _FindTask             (const std::string& Name);

    std::vector<TaskDescriptor> _Tasks;
    bool                        _Started = false;
};

extern DMPOScheduler& Scheduler;
