#include "webserver.h"
#include "config.h"
#include <WebServer.h>
#include <ArduinoJson.h>

// ============================================================
//  webserver.cpp  —  HTTP сервер на стандартном WebServer.h
// ============================================================

static WebServer  server(80);
static SensorData* _data = nullptr;

// ── HTML дашборд (встроенный) ────────────────────────────────
static const char HTML_PAGE[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Метеостанция</title>
<style>
  *{box-sizing:border-box;margin:0;padding:0}
  body{font-family:'Segoe UI',system-ui,sans-serif;background:#0f172a;color:#e2e8f0;min-height:100vh;display:flex;flex-direction:column;align-items:center;padding:24px 16px}
  h1{font-size:1.4rem;font-weight:600;letter-spacing:.05em;color:#94a3b8;margin-bottom:24px;text-transform:uppercase}
  .grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:16px;width:100%;max-width:700px}
  .card{background:#1e293b;border-radius:16px;padding:24px;text-align:center;border:1px solid #334155;transition:transform .2s}
  .card:hover{transform:translateY(-3px)}
  .icon{font-size:2.4rem;margin-bottom:8px}
  .label{font-size:.75rem;color:#64748b;text-transform:uppercase;letter-spacing:.08em;margin-bottom:6px}
  .value{font-size:2.2rem;font-weight:700;line-height:1}
  .unit{font-size:.9rem;color:#94a3b8;margin-left:2px}
  .temp .value{color:#f97316}
  .pres .value{color:#38bdf8}
  .alt  .value{color:#a78bfa}
  .footer{margin-top:32px;font-size:.75rem;color:#334155;text-align:center}
  .status{display:inline-block;width:8px;height:8px;border-radius:50%;background:#22c55e;margin-right:6px;animation:pulse 2s infinite}
  @keyframes pulse{0%,100%{opacity:1}50%{opacity:.4}}
  .error{color:#ef4444}
</style>
</head>
<body>
<h1>&#127777; Метеостанция</h1>
<div class="grid">
  <div class="card temp">
    <div class="icon">🌡️</div>
    <div class="label">Температура</div>
    <div class="value" id="temp">—<span class="unit">°C</span></div>
  </div>
  <div class="card pres">
    <div class="icon">🌬️</div>
    <div class="label">Давление</div>
    <div class="value" id="pres">—<span class="unit">гПа</span></div>
  </div>
  <div class="card alt">
    <div class="icon">⛰️</div>
    <div class="label">Высота</div>
    <div class="value" id="alt">—<span class="unit">м</span></div>
  </div>
</div>
<div class="footer">
  <span class="status"></span>ESP32-C6-Zero &nbsp;|&nbsp; BMP280 &nbsp;|&nbsp;
  Обновление каждые <span id="interval">10</span> с &nbsp;|&nbsp;
  <span id="uptime">—</span>
</div>
<script>
const interval = )rawhtml"
// interval_ms вставляется из C++
R"rawhtml( ;
document.getElementById('interval').textContent = interval / 1000;

async function update() {
  try {
    const r = await fetch('/api/data');
    const d = await r.json();
    if (!d.valid) {
      ['temp','pres','alt'].forEach(id=>{
        document.getElementById(id).innerHTML='<span class="error">err</span>';
      });
      return;
    }
    document.getElementById('temp').innerHTML = d.temperature.toFixed(1)+'<span class="unit">°C</span>';
    document.getElementById('pres').innerHTML = d.pressure.toFixed(1)+'<span class="unit">гПа</span>';
    document.getElementById('alt').innerHTML  = d.altitude.toFixed(0)+'<span class="unit">м</span>';

    const s = d.uptime_s;
    const h = Math.floor(s/3600), m = Math.floor((s%3600)/60), sec = s%60;
    document.getElementById('uptime').textContent =
      `Аптайм: ${h}ч ${String(m).padStart(2,'0')}м ${String(sec).padStart(2,'0')}с`;
  } catch(e) {
    console.error('fetch error', e);
  }
}

update();
setInterval(update, interval);
</script>
</body>
</html>
)rawhtml";

// ── Обработчики маршрутов ────────────────────────────────────

static void handleRoot() {
    // Вставляем SENSOR_INTERVAL_MS в JS перед отправкой
    String page = FPSTR(HTML_PAGE);
    page.replace("const interval = )", "const interval = " + String(SENSOR_INTERVAL_MS) + ";)");
    // Упрощённый способ: строим HTML с нужным интервалом
    server.send(200, "text/html; charset=utf-8", page);
}

static void handleApiData() {
    StaticJsonDocument<256> doc;

    if (_data && _data->valid) {
        doc["valid"]       = true;
        doc["temperature"] = serialized(String(_data->temperature, 2));
        doc["pressure"]    = serialized(String(_data->pressure, 2));
        doc["altitude"]    = serialized(String(_data->altitude, 1));
        doc["uptime_s"]    = millis() / 1000UL;
    } else {
        doc["valid"] = false;
    }

    String json;
    serializeJson(doc, json);
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", json);
}

static void handleNotFound() {
    server.send(404, "text/plain", "Not found");
}

// ── Публичный API ─────────────────────────────────────────────

void webServerInit(SensorData* dataPtr) {
    _data = dataPtr;

    server.on("/",          HTTP_GET, handleRoot);
    server.on("/api/data",  HTTP_GET, handleApiData);
    server.onNotFound(handleNotFound);

    server.begin();
    Serial.println("[HTTP] Сервер запущен на порту 80");
    Serial.printf("[HTTP] http://%s.local\n", DEVICE_HOSTNAME);
}

void webServerHandle() {
    server.handleClient();
}
