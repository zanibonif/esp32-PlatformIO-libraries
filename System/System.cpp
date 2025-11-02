#include "System.h"
#include <LoggerHandler.h>

String GetLibrariesVersion() {
    return String(LIBRARIES_VERSION_1) + "." + String(LIBRARIES_VERSION_2) + "." + String(LIBRARIES_VERSION_3);
}

void Hibernate(unsigned long long int HibernationTime) {
    LOG(INFO, "Hibernate", "Going in hybernation for " + String(HibernationTime) + " seconds");

    esp_sleep_enable_timer_wakeup(HibernationTime * SECONDS_TO_MICROSECONDS);
    esp_sleep_pd_config(ESP_PD_DOMAIN_MAX, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);

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
        default:                               WakeUpReasonString = "SYSTE_JUST_POWERED_ON"; break;
    }

    LOG(INFO, "GetWakeUpReason", "Wake up reason is " + WakeUpReasonString);
    return WakeUpReasonString;
}

void SetCpuFrequency (unsigned int CpuFrequency) {
    LOG(INFO, "SetCpuFrequency", "Frequency set to " + String(CpuFrequency));
    setCpuFrequencyMhz(CpuFrequency);
}

unsigned int GetCpuFrequency () {
    return getCpuFrequencyMhz();
}
