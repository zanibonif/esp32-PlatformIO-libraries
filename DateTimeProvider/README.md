# DateTimeProvider

Interfaccia astratta (`pure virtual`) per i provider di data e ora. Non è un singleton: è la base comune di `NtpHandler` e `DS3231_RtcHandler`.

## Interfaccia

```cpp
class DateTimeProvider {
public:
    virtual String GetFormattedTime (const String& Format) = 0;
};
```

## Utilizzo

Passare un puntatore a `DateTimeProvider` dove serve un provider di tempo generico, tipicamente a `LoggerHandler`:

```cpp
Logger.SetDateTimeProvider(&Ntp);   // NtpHandler implementa DateTimeProvider
Logger.SetDateTimeProvider(&Rtc);   // DS3231_RtcHandler implementa DateTimeProvider
```

## Formato

Il formato segue la sintassi `strftime`:

```
%d/%m/%Y %H:%M:%S   →  25/12/2024 14:30:00
%H:%M:%S            →  14:30:00
%Y-%m-%d            →  2024-12-25
```

## Implementazioni disponibili

| Classe | Alias | Note |
|---|---|---|
| `NtpHandler` | `Ntp` | Sincronizzazione via rete, richiede WiFi |
| `DS3231_RtcHandler` | `Rtc` | Hardware RTC I2C, funziona offline |
