# ESP32-C6-Zero + BMP280 — Метеостанция

Метеостанция на базе Waveshare ESP32-C6-Zero с сенсором BMP280.  
Измеряет **температуру**, **атмосферное давление** и **расчётную высоту** над уровнем моря.

---

## Структура проекта

```
bmp280-station/
├── platformio.ini
└── src/
    ├── config.h        ← все настройки здесь
    ├── main.cpp        ← setup / loop, WiFi, OTA
    ├── sensor.h/.cpp   ← BMP280 через Adafruit
    ├── webserver.h/.cpp← HTTP дашборд + REST API
    └── mqtt.h/.cpp     ← публикация в MQTT
```

---

## Подключение BMP280

```
ESP32-C6-Zero          BMP280 модуль
─────────────          ─────────────
3.3V        ──────────► VCC
GND         ──────────► GND
GPIO6 (SDA) ──────────► SDA
GPIO7 (SCL) ──────────► SCL
                        SDO → GND  (адрес 0x76)
```

> **Внимание**: BMP280 работает только от 3.3V. На 5V сгорит!

---

## Быстрый старт

1. Клонировать / скачать проект
2. Открыть `src/config.h` и заполнить:
   - `WIFI_SSID` / `WIFI_PASSWORD`
   - `MQTT_SERVER` (или поставить `MQTT_ENABLED false`)
   - `SEA_LEVEL_HPA` — текущее давление на уровне моря (прогноз погоды)
3. Прошить: `pio run --target upload`
4. Открыть браузер: `http://weather-station.local`

---

## Веб-интерфейс

| URL          | Описание                            |
|--------------|-------------------------------------|
| `/`          | HTML дашборд (авто-обновление)      |
| `/api/data`  | JSON с текущими данными             |

Пример ответа `/api/data`:
```json
{
  "valid": true,
  "temperature": 22.54,
  "pressure": 1018.32,
  "altitude": -41.2,
  "uptime_s": 3605
}
```

---

## MQTT топики

| Топик                         | Данные                      |
|-------------------------------|-----------------------------|
| `weather/station/state`       | JSON (все параметры)        |
| `weather/station/temperature` | float °C                    |
| `weather/station/pressure`    | float гПа                   |
| `weather/station/altitude`    | float м                     |
| `weather/station/status`      | `online` / `offline` (LWT)  |

---

## Home Assistant (YAML)

```yaml
mqtt:
  sensor:
    - name: "Температура"
      state_topic: "weather/station/temperature"
      unit_of_measurement: "°C"
      device_class: temperature
      value_template: "{{ value | float | round(1) }}"

    - name: "Давление"
      state_topic: "weather/station/pressure"
      unit_of_measurement: "hPa"
      device_class: atmospheric_pressure
      value_template: "{{ value | float | round(1) }}"
```

---

## OTA обновление

После первой прошивки по USB последующие — по воздуху:

```ini
; в platformio.ini раскомментировать:
upload_protocol = espota
upload_port     = weather-station.local
```

Пароль OTA задаётся в `config.h` → `OTA_PASSWORD`.

---

## Зависимости (устанавливаются автоматически)

- `adafruit/Adafruit BMP280 Library`
- `adafruit/Adafruit Unified Sensor`
- `knolleary/PubSubClient`

---

## LED-индикация (WS2812 GPIO8)

| Цвет         | Состояние                     |
|--------------|-------------------------------|
| Синий тусклый| Старт                         |
| Синий яркий  | Подключение к WiFi            |
| Зелёный      | Норма / успешное чтение       |
| Оранжевый    | OTA обновление                |
| Красный      | Ошибка сенсора или WiFi       |
