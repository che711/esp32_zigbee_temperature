#pragma once
#include "sensor.h"

// ============================================================
//  mqtt.h  —  публикация данных через PubSubClient
// ============================================================

void mqttInit();
void mqttLoop(const SensorData& data);  // вызывать в loop()
