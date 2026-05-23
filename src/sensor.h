#pragma once
#include <Arduino.h>

// ============================================================
//  sensor.h  —  обёртка над Adafruit BMP280
// ============================================================

struct SensorData {
    float temperature;  // °C
    float pressure;     // гПа
    float altitude;     // м (расчётная)
    bool  valid;        // false = ошибка чтения
};

bool   sensorInit();
SensorData sensorRead();
