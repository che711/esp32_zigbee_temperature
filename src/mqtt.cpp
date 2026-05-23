#include "mqtt.h"
#include "config.h"
#include <WiFi.h>
#include <PubSubClient.h>

// ============================================================
//  mqtt.cpp  —  MQTT через PubSubClient
// ============================================================

static WiFiClient   wifiClient;
static PubSubClient mqttClient(wifiClient);

static unsigned long _lastPublish   = 0;
static unsigned long _lastReconnect = 0;

// ── Реконнект (неблокирующий) ─────────────────────────────────
static bool mqttReconnect() {
    if (mqttClient.connected()) return true;

    unsigned long now = millis();
    if (now - _lastReconnect < MQTT_RECONNECT_MS) return false;
    _lastReconnect = now;

    Serial.printf("[MQTT] Подключение к %s:%d ...\n", MQTT_SERVER, MQTT_PORT);

    bool ok;
    if (strlen(MQTT_USER) > 0) {
        ok = mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD,
                                MQTT_TOPIC_STATUS, 0, true, "offline");
    } else {
        ok = mqttClient.connect(MQTT_CLIENT_ID, nullptr, nullptr,
                                MQTT_TOPIC_STATUS, 0, true, "offline");
    }

    if (ok) {
        mqttClient.publish(MQTT_TOPIC_STATUS, "online", true);
        Serial.println("[MQTT] Подключено.");
    } else {
        Serial.printf("[MQTT] Ошибка rc=%d, повтор через %lu с\n",
                      mqttClient.state(), MQTT_RECONNECT_MS / 1000);
    }
    return ok;
}

// ── Публикация JSON + отдельные топики ───────────────────────
static void publishData(const SensorData& d) {
    char buf[128];

    // Отдельные топики (удобно для Home Assistant sensors)
    snprintf(buf, sizeof(buf), "%.2f", d.temperature);
    mqttClient.publish(MQTT_TOPIC_TEMP, buf, false);

    snprintf(buf, sizeof(buf), "%.2f", d.pressure);
    mqttClient.publish(MQTT_TOPIC_PRESSURE, buf, false);

    snprintf(buf, sizeof(buf), "%.1f", d.altitude);
    mqttClient.publish(MQTT_TOPIC_ALTITUDE, buf, false);

    // JSON payload в один топик (удобно для Node-RED / InfluxDB)
    snprintf(buf, sizeof(buf),
             "{\"temperature\":%.2f,\"pressure\":%.2f,\"altitude\":%.1f}",
             d.temperature, d.pressure, d.altitude);
    mqttClient.publish(MQTT_TOPIC_STATE, buf, false);

    Serial.printf("[MQTT] Опубликовано: T=%.2f°C P=%.2fгПа A=%.1fм\n",
                  d.temperature, d.pressure, d.altitude);
}

// ── Публичный API ─────────────────────────────────────────────

void mqttInit() {
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setKeepAlive(60);
    mqttClient.setBufferSize(512);
    Serial.println("[MQTT] Клиент настроен.");
}

void mqttLoop(const SensorData& data) {
    if (!mqttReconnect()) return;
    mqttClient.loop();  // обслуживание keep-alive

    if (!data.valid) return;

    unsigned long now = millis();
    if (now - _lastPublish >= MQTT_INTERVAL_MS) {
        _lastPublish = now;
        publishData(data);
    }
}
