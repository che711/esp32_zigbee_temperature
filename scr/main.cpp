#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>

#include "config.h"
#include "sensor.h"
#include "webserver.h"
#include "mqtt.h"

// ============================================================
//  main.cpp  —  точка входа, WiFi, OTA, главный цикл
// ============================================================

// Глобальный буфер данных (пишет sensor, читают web + mqtt)
static SensorData latestData{};
static unsigned long lastSensorRead = 0;

// ── Статус LED (WS2812 на GPIO8) ──────────────────────────────
// Минимальная реализация без библиотеки (один пиксель, RMT)
#include <driver/rmt_tx.h>

static void ledColor(uint8_t r, uint8_t g, uint8_t b) {
    // Используем простой digitalWrite-паттерн через neopixelWrite (arduino-esp32 ≥ 3.x)
    neopixelWrite(LED_PIN, g, r, b);  // WS2812: порядок G-R-B
}

// ── WiFi ──────────────────────────────────────────────────────
static void wifiConnect() {
    Serial.printf("[WiFi] Подключение к %s", WIFI_SSID);
    WiFi.setHostname(DEVICE_HOSTNAME);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    ledColor(0, 0, 20);   // синий — идёт подключение

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        if (++attempts > 40) {
            Serial.println("\n[WiFi] Не удалось подключиться! Перезагрузка...");
            ESP.restart();
        }
    }
    Serial.printf("\n[WiFi] Подключено. IP: %s\n", WiFi.localIP().toString().c_str());
    ledColor(0, 10, 0);   // зелёный — подключено
}

// ── mDNS ─────────────────────────────────────────────────────
static void mdnsInit() {
    if (MDNS.begin(DEVICE_HOSTNAME)) {
        MDNS.addService("http", "tcp", 80);
        Serial.printf("[mDNS] Доступен: http://%s.local\n", DEVICE_HOSTNAME);
    }
}

// ── OTA ──────────────────────────────────────────────────────
static void otaInit() {
#if OTA_ENABLED
    ArduinoOTA.setHostname(DEVICE_HOSTNAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);

    ArduinoOTA.onStart([]() {
        ledColor(20, 10, 0);  // оранжевый — OTA
        Serial.println("[OTA] Начало обновления...");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("[OTA] %u%%\r", progress * 100 / total);
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\n[OTA] Завершено. Перезагрузка.");
        ledColor(0, 20, 0);
    });
    ArduinoOTA.onError([](ota_error_t err) {
        ledColor(20, 0, 0);   // красный — ошибка
        Serial.printf("[OTA] Ошибка[%u]: ", err);
        switch (err) {
            case OTA_AUTH_ERROR:    Serial.println("Auth Failed");    break;
            case OTA_BEGIN_ERROR:   Serial.println("Begin Failed");   break;
            case OTA_CONNECT_ERROR: Serial.println("Connect Failed"); break;
            case OTA_RECEIVE_ERROR: Serial.println("Receive Failed"); break;
            case OTA_END_ERROR:     Serial.println("End Failed");     break;
        }
    });

    ArduinoOTA.begin();
    Serial.println("[OTA] Готов к обновлениям по воздуху.");
#endif
}

// ── Печать данных в Serial ───────────────────────────────────
static void printData(const SensorData& d) {
    if (!d.valid) {
        Serial.println("[Sensor] Нет данных.");
        return;
    }
    Serial.printf("[Sensor] T: %.2f°C | P: %.2f гПа | Alt: %.1f м | Uptime: %lus\n",
                  d.temperature, d.pressure, d.altitude, millis() / 1000UL);
}

// ─────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n\n=== ESP32-C6 Weather Station ===");

    pinMode(LED_PIN, OUTPUT);
    ledColor(0, 0, 5);   // dim синий при старте

    // Сенсор
    if (!sensorInit()) {
        // Мигаем красным при ошибке сенсора, но продолжаем
        for (int i = 0; i < 5; i++) {
            ledColor(20, 0, 0); delay(200);
            ledColor(0, 0, 0);  delay(200);
        }
    }

    // Первое чтение сразу
    latestData = sensorRead();
    printData(latestData);

    wifiConnect();
    mdnsInit();
    otaInit();

    webServerInit(&latestData);

#if MQTT_ENABLED
    mqttInit();
#endif

    Serial.println("[Setup] Готово!\n");
}

void loop() {
    // OTA
#if OTA_ENABLED
    ArduinoOTA.handle();
#endif

    // Веб-сервер
    webServerHandle();

    // Опрос сенсора по таймеру
    unsigned long now = millis();
    if (now - lastSensorRead >= SENSOR_INTERVAL_MS) {
        lastSensorRead = now;
        latestData = sensorRead();
        printData(latestData);

        // Индикация: зелёный при норме, красный при ошибке
        if (latestData.valid) {
            ledColor(0, 3, 0);
            delay(50);
            ledColor(0, 0, 0);
        } else {
            ledColor(3, 0, 0);
        }
    }

    // MQTT
#if MQTT_ENABLED
    mqttLoop(latestData);
#endif
}
