#include "System.h"
#include <LoggerHandler.h>

String GetLibrariesVersion () {
    return String(LIBRARIES_VERSION_1) + "." + String(LIBRARIES_VERSION_2) + "." + String(LIBRARIES_VERSION_3);
}

void Hibernate (unsigned long long int HibernationTime) {
    LOG(INFO, "Hibernate", "Going in hibernation for " + String(HibernationTime) + " seconds");

    esp_sleep_enable_timer_wakeup(HibernationTime * SECONDS_TO_MICROSECONDS);
    // ESP-IDF 5.x declares each power-down domain only on SoCs that have it (the C3 has none of
    // these three), so each call is guarded by the same macro used in esp_sleep.h.
    // ESP-IDF 4.4 declares all three unconditionally and lacks some of those macros: the version
    // check keeps the original behaviour there.
    // ESP_PD_DOMAIN_MAX is the number of domains, not a domain: it must not be passed here.
#if SOC_PM_SUPPORT_RTC_PERIPH_PD || (ESP_IDF_VERSION_MAJOR < 5)
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
#endif
#if SOC_PM_SUPPORT_RTC_SLOW_MEM_PD || (ESP_IDF_VERSION_MAJOR < 5)
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
#endif
#if SOC_PM_SUPPORT_RTC_FAST_MEM_PD || (ESP_IDF_VERSION_MAJOR < 5)
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
#endif

    esp_deep_sleep_start();
}

String GetWakeUpReason () {
    esp_sleep_wakeup_cause_t WakeUpReason = esp_sleep_get_wakeup_cause();
    String WakeUpReasonString;

    switch(WakeUpReason) {
        case ESP_SLEEP_WAKEUP_UNDEFINED:       WakeUpReasonString = "ESP_SLEEP_WAKEUP_UNDEFINED"; break;
        case ESP_SLEEP_WAKEUP_ALL:             WakeUpReasonString = "ESP_SLEEP_WAKEUP_ALL"; break;
        case ESP_SLEEP_WAKEUP_EXT0:            WakeUpReasonString = "ESP_SLEEP_WAKEUP_EXT0"; break;
        case ESP_SLEEP_WAKEUP_EXT1:            WakeUpReasonString = "ESP_SLEEP_WAKEUP_EXT1"; break;
        case ESP_SLEEP_WAKEUP_TIMER:           WakeUpReasonString = "ESP_SLEEP_WAKEUP_TIMER"; break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD:        WakeUpReasonString = "ESP_SLEEP_WAKEUP_TOUCHPAD"; break;
        case ESP_SLEEP_WAKEUP_ULP:             WakeUpReasonString = "ESP_SLEEP_WAKEUP_ULP"; break;
        case ESP_SLEEP_WAKEUP_GPIO:            WakeUpReasonString = "ESP_SLEEP_WAKEUP_GPIO"; break;
        case ESP_SLEEP_WAKEUP_UART:            WakeUpReasonString = "ESP_SLEEP_WAKEUP_UART"; break;
        case ESP_SLEEP_WAKEUP_WIFI:            WakeUpReasonString = "ESP_SLEEP_WAKEUP_WIFI"; break;
        case ESP_SLEEP_WAKEUP_COCPU:           WakeUpReasonString = "ESP_SLEEP_WAKEUP_COCPU"; break;
        case ESP_SLEEP_WAKEUP_COCPU_TRAP_TRIG: WakeUpReasonString = "ESP_SLEEP_WAKEUP_COCPU_TRAP_TRIG"; break;
        case ESP_SLEEP_WAKEUP_BT:              WakeUpReasonString = "ESP_SLEEP_WAKEUP_BT"; break;
        default:                               WakeUpReasonString = "SYSTEM_JUST_POWERED_ON"; break;
    }

    // Nessun log qui: questa e' una lettura, e il motivo di risveglio e' un dato
    // del boot, non un evento che accade quando qualcuno lo chiede. Loggandolo
    // dentro al getter ogni richiesta di stato lasciava una riga nel log, che
    // spuntava in mezzo a sequenze che non avevano niente a che vedere. Chi vuole
    // il motivo nel log lo scriva lui all'avvio, come fa setup().
    return WakeUpReasonString;
}

void SetCpuFrequency (unsigned int CpuFrequency) {
    if (!setCpuFrequencyMhz(CpuFrequency)) {
        LOG(WARNING, "SetCpuFrequency", "Frequenza " + String(CpuFrequency) +
            " MHz non supportata dal SoC, invariata a " + String(getCpuFrequencyMhz()) + " MHz");
        return;
    }
    LOG(INFO, "SetCpuFrequency", "Frequency set to " + String(getCpuFrequencyMhz()) + " MHz");
}

unsigned int GetCpuFrequency () {
    return getCpuFrequencyMhz();
}

unsigned long long GetUptimeUs () {
    return (unsigned long long)esp_timer_get_time();
}

unsigned long long GetUptimeS () {
    return (unsigned long long)(GetUptimeUs() / 1000000LL);
}

void SetSystemTime (DateTimeProvider& Provider) {
    time_t Epoch = (time_t)Provider.GetEpochTime();
    struct timeval Tv = { .tv_sec = Epoch, .tv_usec = 0 };
    settimeofday(&Tv, nullptr);
    LOG(INFO, "SetSystemTime", "System time impostato: " + String((unsigned long)Epoch));
}

// --- Safe arithmetic ---

void SafeIncrement (int& Value, int Step)                    { if (Value > INT_MAX   - Step) Value = INT_MAX;   else Value += Step; }
void SafeIncrement (unsigned int& Value, unsigned int Step)  { if (Value > UINT_MAX  - Step) Value = UINT_MAX;  else Value += Step; }
void SafeIncrement (long& Value, long Step)                  { if (Value > LONG_MAX  - Step) Value = LONG_MAX;  else Value += Step; }
void SafeIncrement (unsigned long& Value, unsigned long Step){ if (Value > ULONG_MAX - Step) Value = ULONG_MAX; else Value += Step; }

void SafeDecrement (int& Value, int Step)                    { if (Value < INT_MIN   + Step) Value = INT_MIN;   else Value -= Step; }
void SafeDecrement (unsigned int& Value, unsigned int Step)  { if (Value < Step)             Value = 0;         else Value -= Step; }
void SafeDecrement (long& Value, long Step)                  { if (Value < LONG_MIN  + Step) Value = LONG_MIN;  else Value -= Step; }
void SafeDecrement (unsigned long& Value, unsigned long Step){ if (Value < Step)             Value = 0;         else Value -= Step; }
