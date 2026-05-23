#include "sensor.h"
#include "config.h"
#include <Wire.h>
#include <Adafruit_BMP280.h>

// ============================================================
//  sensor.cpp  —  инициализация и чтение BMP280
// ============================================================

static Adafruit_BMP280 bmp;
static bool _initialized = false;

bool sensorInit() {
    Wire.begin(BMP280_SDA_PIN, BMP280_SCL_PIN);

    if (!bmp.begin(BMP280_I2C_ADDR)) {
        Serial.println("[BMP280] Устройство не найдено! Проверь адрес и подключение.");
        Serial.printf("[BMP280] Ожидаемый адрес: 0x%02X\n", BMP280_I2C_ADDR);
        _initialized = false;
        return false;
    }

    // Настройки из даташита Bosch для метеостанции (низкий шум)
    bmp.setSampling(
        Adafruit_BMP280::MODE_NORMAL,     // непрерывное измерение
        Adafruit_BMP280::SAMPLING_X2,     // температура: oversampling x2
        Adafruit_BMP280::SAMPLING_X16,    // давление:    oversampling x16
        Adafruit_BMP280::FILTER_X16,      // фильтр IIR x16
        Adafruit_BMP280::STANDBY_MS_500   // интервал standby
    );

    _initialized = true;
    Serial.println("[BMP280] Инициализация успешна.");
    return true;
}

SensorData sensorRead() {
    SensorData data{};

    if (!_initialized) {
        data.valid = false;
        return data;
    }

    float t = bmp.readTemperature();
    float p = bmp.readPressure() / 100.0f;   // Па → гПа
    float a = bmp.readAltitude(SEA_LEVEL_HPA);

    // Простая валидация: BMP280 возвращает 0 при ошибке шины
    if (isnan(t) || isnan(p) || p < 300.0f || p > 1200.0f) {
        data.valid = false;
        Serial.println("[BMP280] Некорректные данные!");
        return data;
    }

    data.temperature = t;
    data.pressure    = p;
    data.altitude    = a;
    data.valid       = true;
    return data;
}
