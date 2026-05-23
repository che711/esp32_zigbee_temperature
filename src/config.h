#pragma once

// ============================================================
//  config.h  —  все пользовательские настройки в одном месте
// ============================================================

// ── WiFi ────────────────────────────────────────────────────
#define WIFI_SSID     "SkyNet"
#define WIFI_PASSWORD "password"

// Hostname (mDNS: http://weather-station.local)
#define DEVICE_HOSTNAME "weather-station"

// ── MQTT ────────────────────────────────────────────────────
#define MQTT_ENABLED   true           // false — отключить MQTT полностью
#define MQTT_SERVER    "192.168.1.10" // IP брокера (Home Assistant / Mosquitto)
#define MQTT_PORT      1883
#define MQTT_USER      ""             // пусто, если без авторизации
#define MQTT_PASSWORD  ""
#define MQTT_CLIENT_ID "esp32c6-weather"

// Топики
#define MQTT_TOPIC_STATE    "weather/station/state"    // JSON payload
#define MQTT_TOPIC_TEMP     "weather/station/temperature"
#define MQTT_TOPIC_PRESSURE "weather/station/pressure"
#define MQTT_TOPIC_ALTITUDE "weather/station/altitude"
#define MQTT_TOPIC_STATUS   "weather/station/status"   // online / offline

// ── BMP280 ──────────────────────────────────────────────────
// I²C на ESP32-C6-Zero: SDA=GPIO6, SCL=GPIO7
#define BMP280_SDA_PIN  6
#define BMP280_SCL_PIN  7
// Адрес: 0x76 (SDO → GND, типично для модулей)
//        0x77 (SDO → VCC или не подключён)
#define BMP280_I2C_ADDR 0x76

// Давление на уровне моря для расчёта высоты (гПа)
// Варшава ~1013 гПа, но лучше брать текущее по прогнозу
#define SEA_LEVEL_HPA   1013.25f

// ── Onboard WS2812 RGB LED ───────────────────────────────────
#define LED_PIN         8   // GPIO8 на ESP32-C6-Zero

// ── Интервалы ────────────────────────────────────────────────
#define SENSOR_INTERVAL_MS   10000UL  // опрос сенсора, мс
#define MQTT_INTERVAL_MS     30000UL  // публикация MQTT, мс (≥ SENSOR_INTERVAL)
#define MQTT_RECONNECT_MS     5000UL  // пауза между попытками реконнекта

// ── OTA ──────────────────────────────────────────────────────
#define OTA_ENABLED  true
#define OTA_PASSWORD "ota_password"   // пароль для ArduinoOTA
