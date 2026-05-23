#pragma once
#include "sensor.h"

// ============================================================
//  webserver.h  —  HTTP сервер + встроенный HTML дашборд
// ============================================================

void webServerInit(SensorData* dataPtr);
void webServerHandle();
