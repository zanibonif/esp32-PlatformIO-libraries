#include "DMPOScheduler.h"

DMPOScheduler& DMPOScheduler::GetInstance () {
    static DMPOScheduler Instance;
    return Instance;
}
DMPOScheduler& Scheduler = DMPOScheduler::GetInstance();

DMPOScheduler::DMPOScheduler () {
    LOG(INFO, "DMPOScheduler", "Istanza creata");
}

// --- Configurazione ---

int DMPOScheduler::AddTask (TaskConfig& Config) {
    if (_Started) {
        LOG(ERROR, "DMPOScheduler::AddTask", "Impossibile aggiungere task dopo Begin()");
        Config.ID = -1;
        return -1;
    }

    if (Config.Name.empty()) {
        LOG(ERROR, "DMPOScheduler::AddTask", "Il nome del task non può essere vuoto");
        Config.ID = -1;
        return -1;
    }

    if (_FindTask(Config.Name) != nullptr) {
        LOG(ERROR, "DMPOScheduler::AddTask", ("Task con nome '" + Config.Name + "' già esistente").c_str());
        Config.ID = -1;
        return -1;
    }

    if (Config.PeriodUs == 0) {
        LOG(ERROR, "DMPOScheduler::AddTask", "PeriodUs non può essere 0");
        Config.ID = -1;
        return -1;
    }

    if (Config.PeriodUs < 1000 && !Config.AppCritical) {
        LOG(ERROR, "DMPOScheduler::AddTask", ("Task '" + Config.Name + "' ha PeriodUs < 1ms ma non è AppCritical: task non aggiunto").c_str());
        Config.ID = -1;
        return -1;
    }

    if (Config.DeadlineUs == 0) {
        Config.DeadlineUs = Config.PeriodUs;
        LOG(DEBUG, "DMPOScheduler::AddTask", ("DeadlineUs non specificata, impostata uguale a PeriodUs per il task '" + Config.Name + "'").c_str());
    }

    if (Config.DeadlineUs > Config.PeriodUs) {
        LOG(WARNING, "DMPOScheduler::AddTask", ("DeadlineUs > PeriodUs per il task '" + Config.Name + "': condizione inusuale").c_str());
    }

    if (Config.AppCritical) {
        Config.CoreID = 1;
        LOG(INFO, "DMPOScheduler::AddTask", ("Task '" + Config.Name + "' è AppCritical, CoreID forzato a 1").c_str());
    } else {
        Config.CoreID = 0;
        LOG(INFO, "DMPOScheduler::AddTask", ("Task '" + Config.Name + "' non è AppCritical, CoreID forzato a 0").c_str());
    }

    if (Config.StackSize == 0) {
        Config.StackSize = 4096;
        LOG(WARNING, "DMPOScheduler::AddTask", ("StackSize non specificato per il task '" + Config.Name + "', impostato a 4096 byte").c_str());
    }

    TaskDescriptor Descriptor;
    Descriptor.ID                  = static_cast<int>(_Tasks.size());
    Descriptor.Name                = Config.Name;
    Descriptor.PeriodUs            = Config.PeriodUs;
    Descriptor.DeadlineUs          = Config.DeadlineUs;
    Descriptor.AppCritical         = Config.AppCritical;
    Descriptor.CoreID              = Config.CoreID;
    Descriptor.StackSize           = Config.StackSize;
    Descriptor.Priority            = 0;
    Descriptor.Enabled             = true;
    Descriptor.Handle              = nullptr;
    Descriptor.OnMissedDeadline    = Config.OnMissedDeadline;
    Descriptor.LastCycleTimeUs     = 0;
    Descriptor.MissedDeadlineCount = 0;

    _Tasks.push_back(Descriptor);
    Config.ID = Descriptor.ID;

    LOG(INFO, "DMPOScheduler::AddTask", ("Task '" + Config.Name + "' aggiunto con ID " + std::to_string(Config.ID) +
                                          " | PeriodUs: " + std::to_string(Config.PeriodUs) +
                                          " us | CoreID: " + std::to_string(Config.CoreID)).c_str());
    return Config.ID;
}

bool DMPOScheduler::AddFunction (int TaskID, TaskFunction Fn) {
    if (_Started) {
        LOG(ERROR, "DMPOScheduler::AddFunction", "Impossibile aggiungere funzioni dopo Begin()");
        return false;
    }

    if (!Fn) {
        LOG(ERROR, "DMPOScheduler::AddFunction", "La funzione passata non è valida");
        return false;
    }

    TaskDescriptor* Descriptor = _FindTask(TaskID);
    if (Descriptor == nullptr) {
        LOG(ERROR, "DMPOScheduler::AddFunction", ("Task con ID " + std::to_string(TaskID) + " non trovato").c_str());
        return false;
    }

    Descriptor->Functions.push_back(Fn);

    LOG(INFO, "DMPOScheduler::AddFunction", ("Funzione aggiunta al task '" + Descriptor->Name +
              "' | Totale funzioni: " + std::to_string(Descriptor->Functions.size())).c_str());
    return true;
}

// --- Avvio ---

bool DMPOScheduler::Begin () {
    if (_Started) {
        LOG(ERROR, "DMPOScheduler::Begin", "Begin() già chiamato in precedenza");
        return false;
    }

    if (_Tasks.empty()) {
        LOG(ERROR, "DMPOScheduler::Begin", "Nessun task registrato, Begin() annullato");
        return false;
    }

    for (TaskDescriptor& Descriptor : _Tasks) {
        if (Descriptor.Functions.empty()) {
            LOG(WARNING, "DMPOScheduler::Begin", ("Task '" + Descriptor.Name + "' non ha funzioni registrate").c_str());
        }
    }

    _AssignDMPOPriorities();

    for (size_t i = 0; i < _Tasks.size(); i++) {
        TaskDescriptor& Descriptor = _Tasks[i];

        Descriptor.TimerHandle    = nullptr;
        Descriptor.TimerSemaphore = nullptr;

        if (Descriptor.AppCritical) {
            Descriptor.TimerSemaphore = xSemaphoreCreateBinary();
            if (Descriptor.TimerSemaphore == nullptr) {
                LOG(FATAL_ERROR, "DMPOScheduler::Begin", ("Impossibile creare il semaforo per il task '" + Descriptor.Name + "'").c_str());
                return false;
            }

            esp_timer_create_args_t TimerArgs = {};
            TimerArgs.callback        = [](void* Param) {
                TaskDescriptor* Desc = static_cast<TaskDescriptor*>(Param);
                BaseType_t HigherPriorityTaskWoken = pdFALSE;
                xSemaphoreGiveFromISR(Desc->TimerSemaphore, &HigherPriorityTaskWoken);
                portYIELD_FROM_ISR(HigherPriorityTaskWoken);
            };
            TimerArgs.arg             = &Descriptor;
            TimerArgs.dispatch_method = ESP_TIMER_TASK;
            TimerArgs.name            = Descriptor.Name.c_str();

            esp_err_t Err = esp_timer_create(&TimerArgs, &Descriptor.TimerHandle);
            if (Err != ESP_OK) {
                LOG(FATAL_ERROR, "DMPOScheduler::Begin", ("Impossibile creare il timer hardware per il task '" + Descriptor.Name + "'").c_str());
                return false;
            }

            Err = esp_timer_start_periodic(Descriptor.TimerHandle, Descriptor.PeriodUs);
            if (Err != ESP_OK) {
                LOG(FATAL_ERROR, "DMPOScheduler::Begin", ("Impossibile avviare il timer hardware per il task '" + Descriptor.Name + "'").c_str());
                return false;
            }

            LOG(INFO, "DMPOScheduler::Begin", ("Task AppCritical '" + Descriptor.Name +
                      "' | Timer hardware creato | PeriodUs: " + std::to_string(Descriptor.PeriodUs) + " us").c_str());
        }

        BaseType_t Result = xTaskCreatePinnedToCore(
            _TaskEntryPoint,
            Descriptor.Name.c_str(),
            Descriptor.StackSize,
            &_Tasks[i],
            Descriptor.Priority,
            &_Tasks[i].Handle,
            Descriptor.CoreID
        );

        if (Result != pdPASS) {
            LOG(FATAL_ERROR, "DMPOScheduler::Begin", ("Impossibile creare il task FreeRTOS per '" + Descriptor.Name + "'").c_str());
            return false;
        }

        LOG(INFO, "DMPOScheduler::Begin", ("Task '" + Descriptor.Name +
            "' creato | CoreID: " + std::to_string(Descriptor.CoreID) +
            " | Priorità: " + std::to_string(Descriptor.Priority) +
            " | StackSize: " + std::to_string(Descriptor.StackSize) + " byte").c_str());
    }

    _Started = true;
    LOG(INFO, "DMPOScheduler::Begin", "Scheduler avviato correttamente");
    return true;
}

// --- Controllo runtime ---

bool DMPOScheduler::EnableTask (int TaskID) {
    TaskDescriptor* Descriptor = _FindTask(TaskID);
    if (Descriptor == nullptr) {
        LOG(ERROR, "DMPOScheduler::EnableTask", ("Task con ID " + std::to_string(TaskID) + " non trovato").c_str());
        return false;
    }

    if (Descriptor->Enabled) {
        LOG(WARNING, "DMPOScheduler::EnableTask", ("Task '" + Descriptor->Name + "' già abilitato").c_str());
        return true;
    }

    Descriptor->Enabled = true;
    LOG(INFO, "DMPOScheduler::EnableTask", ("Task '" + Descriptor->Name + "' abilitato").c_str());
    return true;
}

bool DMPOScheduler::EnableTask (const std::string& Name) {
    TaskDescriptor* Descriptor = _FindTask(Name);
    if (Descriptor == nullptr) {
        LOG(ERROR, "DMPOScheduler::EnableTask", ("Task '" + Name + "' non trovato").c_str());
        return false;
    }

    if (Descriptor->Enabled) {
        LOG(WARNING, "DMPOScheduler::EnableTask", ("Task '" + Descriptor->Name + "' già abilitato").c_str());
        return true;
    }

    Descriptor->Enabled = true;
    LOG(INFO, "DMPOScheduler::EnableTask", ("Task '" + Descriptor->Name + "' abilitato").c_str());
    return true;
}

bool DMPOScheduler::DisableTask (int TaskID) {
    TaskDescriptor* Descriptor = _FindTask(TaskID);
    if (Descriptor == nullptr) {
        LOG(ERROR, "DMPOScheduler::DisableTask", ("Task con ID " + std::to_string(TaskID) + " non trovato").c_str());
        return false;
    }

    if (!Descriptor->Enabled) {
        LOG(WARNING, "DMPOScheduler::DisableTask", ("Task '" + Descriptor->Name + "' già disabilitato").c_str());
        return true;
    }

    Descriptor->Enabled = false;
    LOG(INFO, "DMPOScheduler::DisableTask", ("Task '" + Descriptor->Name + "' disabilitato").c_str());
    return true;
}

bool DMPOScheduler::DisableTask (const std::string& Name) {
    TaskDescriptor* Descriptor = _FindTask(Name);
    if (Descriptor == nullptr) {
        LOG(ERROR, "DMPOScheduler::DisableTask", ("Task '" + Name + "' non trovato").c_str());
        return false;
    }

    if (!Descriptor->Enabled) {
        LOG(WARNING, "DMPOScheduler::DisableTask", ("Task '" + Descriptor->Name + "' già disabilitato").c_str());
        return true;
    }

    Descriptor->Enabled = false;
    LOG(INFO, "DMPOScheduler::DisableTask", ("Task '" + Descriptor->Name + "' disabilitato").c_str());
    return true;
}

void DMPOScheduler::EnableAllTasks () {
    for (TaskDescriptor& Descriptor : _Tasks) {
        Descriptor.Enabled = true;
    }
    LOG(INFO, "DMPOScheduler::EnableAllTasks", "Tutti i task abilitati");
}

void DMPOScheduler::DisableAllTasks () {
    for (TaskDescriptor& Descriptor : _Tasks) {
        Descriptor.Enabled = false;
    }
    LOG(INFO, "DMPOScheduler::DisableAllTasks", "Tutti i task disabilitati");
}

// --- Diagnostica ---

uint32_t DMPOScheduler::GetLastCycleTimeUs (int TaskID) {
    TaskDescriptor* Descriptor = _FindTask(TaskID);
    if (Descriptor == nullptr) {
        LOG(ERROR, "DMPOScheduler::GetLastCycleTimeUs", ("Task con ID " + std::to_string(TaskID) + " non trovato").c_str());
        return 0;
    }
    return Descriptor->LastCycleTimeUs;
}

uint32_t DMPOScheduler::GetLastCycleTimeUs (const std::string& Name) {
    TaskDescriptor* Descriptor = _FindTask(Name);
    if (Descriptor == nullptr) {
        LOG(ERROR, "DMPOScheduler::GetLastCycleTimeUs", ("Task '" + Name + "' non trovato").c_str());
        return 0;
    }
    return Descriptor->LastCycleTimeUs;
}

uint32_t DMPOScheduler::GetMissedDeadlineCount (int TaskID) {
    TaskDescriptor* Descriptor = _FindTask(TaskID);
    if (Descriptor == nullptr) {
        LOG(ERROR, "DMPOScheduler::GetMissedDeadlineCount", ("Task con ID " + std::to_string(TaskID) + " non trovato").c_str());
        return 0;
    }
    return Descriptor->MissedDeadlineCount;
}

uint32_t DMPOScheduler::GetMissedDeadlineCount (const std::string& Name) {
    TaskDescriptor* Descriptor = _FindTask(Name);
    if (Descriptor == nullptr) {
        LOG(ERROR, "DMPOScheduler::GetMissedDeadlineCount", ("Task '" + Name + "' non trovato").c_str());
        return 0;
    }
    return Descriptor->MissedDeadlineCount;
}

void DMPOScheduler::PrintStatus () {
    LOG(INFO, "DMPOScheduler::PrintStatus", "=== DMPO Scheduler Status ===");
    LOG(INFO, "DMPOScheduler::PrintStatus", ("Task registrati: " + std::to_string(_Tasks.size())).c_str());
    LOG(INFO, "DMPOScheduler::PrintStatus", (_Started ? "Stato: AVVIATO" : "Stato: NON AVVIATO"));

    for (TaskDescriptor& Descriptor : _Tasks) {
        std::string Info =
            "Task '" + Descriptor.Name + "'"
            " | ID: "             + std::to_string(Descriptor.ID) +
            " | CoreID: "         + std::to_string(Descriptor.CoreID) +
            " | AppCritical: "    + (Descriptor.AppCritical ? "SI" : "NO") +
            " | Priorità: "       + std::to_string(Descriptor.Priority) +
            " | PeriodUs: "       + std::to_string(Descriptor.PeriodUs) + " us" +
            " | DeadlineUs: "     + std::to_string(Descriptor.DeadlineUs) + " us" +
            " | Stato: "          + (Descriptor.Enabled ? "ABILITATO" : "DISABILITATO") +
            " | LastCycleTime: "  + std::to_string(Descriptor.LastCycleTimeUs) + " us" +
            " | MissedDeadline: " + std::to_string(Descriptor.MissedDeadlineCount) +
            " | Funzioni: "       + std::to_string(Descriptor.Functions.size());

        LOG(INFO, "DMPOScheduler::PrintStatus", Info.c_str());
    }

    LOG(INFO, "DMPOScheduler::PrintStatus", "=============================");
}

// --- Internals ---

DMPOScheduler::TaskDescriptor* DMPOScheduler::_FindTask (int TaskID) {
    for (TaskDescriptor& Descriptor : _Tasks) {
        if (Descriptor.ID == TaskID)
            return &Descriptor;
    }
    return nullptr;
}

DMPOScheduler::TaskDescriptor* DMPOScheduler::_FindTask (const std::string& Name) {
    for (TaskDescriptor& Descriptor : _Tasks) {
        if (Descriptor.Name == Name)
            return &Descriptor;
    }
    return nullptr;
}

void DMPOScheduler::_AssignDMPOPriorities () {
    for (int CoreID = 0; CoreID <= 1; CoreID++) {
        std::vector<TaskDescriptor*> CoreTasks;
        for (TaskDescriptor& Descriptor : _Tasks) {
            if (Descriptor.CoreID == CoreID)
                CoreTasks.push_back(&Descriptor);
        }

        if (CoreTasks.empty())
            continue;

        std::sort(CoreTasks.begin(), CoreTasks.end(), [](const TaskDescriptor* A, const TaskDescriptor* B) {
            return A->DeadlineUs < B->DeadlineUs;
        });

        UBaseType_t Priority = static_cast<UBaseType_t>(CoreTasks.size());
        for (TaskDescriptor* Descriptor : CoreTasks) {
            Descriptor->Priority = Priority;
            LOG(INFO, "DMPOScheduler::_AssignDMPOPriorities", ("Task '" + Descriptor->Name +
                      "' | CoreID: " + std::to_string(CoreID) +
                      " | DeadlineUs: " + std::to_string(Descriptor->DeadlineUs) +
                      " | Priorità assegnata: " + std::to_string(Priority)).c_str());
            Priority--;
        }
    }
}

void DMPOScheduler::_TaskEntryPoint (void* Param) {
    TaskDescriptor* Descriptor = static_cast<TaskDescriptor*>(Param);
    DMPOScheduler::GetInstance()._RunTask(*Descriptor);
}

void DMPOScheduler::_RunTask (TaskDescriptor& Task) {
    if (Task.AppCritical) {
        while (true) {
            xSemaphoreTake(Task.TimerSemaphore, portMAX_DELAY);

            if (Task.Enabled) {
                uint64_t StartUs = esp_timer_get_time();

                for (TaskFunction& Fn : Task.Functions)
                    Fn();

                uint64_t EndUs = esp_timer_get_time();
                Task.LastCycleTimeUs = static_cast<uint32_t>(EndUs - StartUs);

                if (Task.LastCycleTimeUs > Task.DeadlineUs) {
                    Task.MissedDeadlineCount++;
                    LOG(WARNING, "DMPOScheduler::_RunTask",
                        ("Deadline miss AppCritical '" + Task.Name +
                         "' | CycleTime: " + std::to_string(Task.LastCycleTimeUs) +
                         " us | DeadlineUs: " + std::to_string(Task.DeadlineUs) +
                         " us | Totale miss: " + std::to_string(Task.MissedDeadlineCount)).c_str());

                    if (Task.OnMissedDeadline)
                        Task.OnMissedDeadline(Task.Name, Task.LastCycleTimeUs - Task.DeadlineUs);
                }
            }
        }
    } else {
        TickType_t LastWakeTime  = xTaskGetTickCount();
        const TickType_t PeriodTicks = pdMS_TO_TICKS(Task.PeriodUs / 1000);

        while (true) {
            if (Task.Enabled) {
                uint64_t StartUs = esp_timer_get_time();

                for (TaskFunction& Fn : Task.Functions)
                    Fn();

                uint64_t EndUs = esp_timer_get_time();
                Task.LastCycleTimeUs = static_cast<uint32_t>(EndUs - StartUs);

                if (Task.LastCycleTimeUs > Task.DeadlineUs) {
                    Task.MissedDeadlineCount++;
                    LOG(WARNING, "DMPOScheduler::_RunTask",
                        ("Deadline miss '" + Task.Name +
                         "' | CycleTime: " + std::to_string(Task.LastCycleTimeUs) +
                         " us | DeadlineUs: " + std::to_string(Task.DeadlineUs) +
                         " us | Totale miss: " + std::to_string(Task.MissedDeadlineCount)).c_str());

                    if (Task.OnMissedDeadline)
                        Task.OnMissedDeadline(Task.Name, Task.LastCycleTimeUs - Task.DeadlineUs);
                }
            }

            vTaskDelayUntil(&LastWakeTime, PeriodTicks);
        }
    }
}
