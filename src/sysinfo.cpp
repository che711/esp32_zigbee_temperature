#include "sysinfo.h"
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// ============================================================
//  sysinfo.cpp  —  системные метрики + измерение загрузки CPU
//
//  CPU load: запускаем низкоприоритетную задачу которая
//  крутит счётчик. Чем меньше итераций — тем выше загрузка.
// ============================================================

static volatile uint8_t  _cpuLoad    = 0;
static volatile uint32_t _calibrated = 0;  // базовое кол-во итераций за 500мс

static void cpuMonitorTask(void* pv) {
    const uint32_t WINDOW_MS = 500;

    // Калибровка: первые 500мс считаем сколько итераций при idle
    uint32_t t = millis();
    uint32_t cal = 0;
    while (millis() - t < WINDOW_MS) { cal++; taskYIELD(); }
    _calibrated = cal ? cal : 1;

    // Основной цикл
    while (1) {
        t = millis();
        uint32_t cnt = 0;
        while (millis() - t < WINDOW_MS) { cnt++; taskYIELD(); }

        // Если получили меньше итераций чем в idle — CPU занят
        uint8_t load = 0;
        if (cnt < _calibrated) {
            load = (uint8_t)(100UL - (cnt * 100UL / _calibrated));
        }
        _cpuLoad = load;
    }
}

void sysInfoInit() {
    // Запускаем на idle-приоритете (0+1), отдельное ядро не нужно
    xTaskCreate(cpuMonitorTask, "cpu_mon", 1024, nullptr, 1, nullptr);
    Serial.println("[SysInfo] CPU monitor task started.");
}

SysInfo sysInfoRead() {
    SysInfo s;
    s.chipTempC    = temperatureRead();
    s.freeHeap     = ESP.getFreeHeap();
    s.totalHeap    = ESP.getHeapSize();
    s.heapUsedPct  = (uint8_t)(100 - (s.freeHeap * 100UL / s.totalHeap));
    s.cpuLoadPct   = _cpuLoad;
    s.cpuFreqMHz   = ESP.getCpuFreqMHz();
    s.rssi         = WiFi.RSSI();
    s.rssiPct      = (uint8_t)constrain(2 * (s.rssi + 100), 0, 100);
    s.ssid         = WiFi.SSID();
    s.ip           = WiFi.localIP().toString();
    s.mac          = WiFi.macAddress();
    s.uptimeS      = millis() / 1000UL;
    return s;
}

const char* rssiQuality(int32_t rssi) {
    if (rssi >= -55) return "Отлично";
    if (rssi >= -67) return "Хорошо";
    if (rssi >= -80) return "Слабый";
    return "Критический";
}
