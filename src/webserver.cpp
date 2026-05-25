#include <Arduino.h>
#include "webserver.h"
#include "config.h"
#include "sysinfo.h"
#include <WebServer.h>

static WebServer   server(80);
static SensorData* _data = nullptr;

static String buildPage() {
    String h;
    h.reserve(5500);

    h += "<!DOCTYPE html><html lang='ru'><head>";
    h += "<meta charset='UTF-8'>";
    h += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
    h += "<title>Meteostation</title><style>";

    h += "*{box-sizing:border-box;margin:0;padding:0}";
    h += "body{font-family:'Segoe UI',system-ui,sans-serif;background:#0f172a;color:#e2e8f0;"
         "min-height:100vh;display:flex;flex-direction:column;align-items:center;padding:28px 16px}";
    h += "h1{font-size:1rem;font-weight:700;letter-spacing:.14em;color:#475569;"
         "text-transform:uppercase;margin-bottom:26px}";

    // Section label
    h += ".sec{font-size:.68rem;font-weight:700;letter-spacing:.14em;color:#475569;"
         "text-transform:uppercase;margin:24px 0 12px}";
    h += ".sec:first-of-type{margin-top:0}";
    h += ".wrap{width:100%;max-width:720px}";

    // Weather cards
    h += ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:14px}";
    h += ".card{background:#1e293b;border-radius:16px;padding:28px 20px;"
         "text-align:center;border:1px solid #334155}";
    h += ".icon{font-size:2.2rem;margin-bottom:10px}";
    h += ".lbl{font-size:.7rem;color:#64748b;text-transform:uppercase;"
         "letter-spacing:.1em;margin-bottom:10px}";
    h += ".val{font-size:2.8rem;font-weight:800;line-height:1;white-space:nowrap}";
    h += ".unit{font-size:1.1rem;color:#94a3b8;margin-left:3px;font-weight:400}";
    h += ".sub{font-size:.85rem;color:#94a3b8;margin-top:8px}";

    // Colors
    h += ".ct{color:#fb923c}.cp{color:#38bdf8}.ca{color:#a78bfa}";
    h += ".cm{color:#34d399}.cq{color:#60a5fa}.cd{color:#f472b6}.cf{color:#facc15}";

    // Forecast card value smaller so it fits
    h += ".fore-val{font-size:1.4rem;font-weight:700;margin-top:8px}";

    // System table
    h += ".tbl{background:#1e293b;border-radius:16px;border:1px solid #334155;overflow:hidden}";
    h += ".row{display:flex;justify-content:space-between;align-items:center;"
         "padding:13px 18px;border-bottom:1px solid #0f172a;gap:16px}";
    h += ".row:last-child{border:none}";
    h += ".k{color:#64748b;font-size:.82rem;flex-shrink:0}";
    h += ".rv{display:flex;align-items:center;justify-content:flex-end;"
         "gap:12px;font-weight:600;font-size:.88rem}";
    // Bar — overflow:hidden is the key fix
    h += ".bw{background:#0f172a;border-radius:99px;height:6px;width:100px;"
         "flex-shrink:0;overflow:hidden}";
    h += ".bar{height:100%;border-radius:99px;transition:width .6s ease}";
    h += ".bar-b{background:#38bdf8}";
    h += ".bar-g{background:#4ade80}";
    h += ".bar-o{background:#fb923c}";

    // Trend colors
    h += ".tu{color:#4ade80}.td{color:#f87171}.te{color:#94a3b8}";

    // Footer
    h += ".dot{display:inline-block;width:7px;height:7px;border-radius:50%;"
         "background:#22c55e;margin-right:5px;vertical-align:middle;"
         "animation:bk 2s ease-in-out infinite}";
    h += "@keyframes bk{0%,100%{opacity:1}50%{opacity:.3}}";
    h += ".footer{margin-top:24px;font-size:.7rem;color:#334155;text-align:center}";
    h += ".err{color:#f87171}";
    h += "</style></head><body>";

    // Title
    h += "<h1>&#127814; &#1052;&#1077;&#1090;&#1077;&#1086;&#1089;&#1090;&#1072;&#1085;&#1094;&#1080;&#1103;</h1>";
    h += "<div class='wrap'>";

    // ── WEATHER ──
    h += "<div class='sec'>&#127777; &#1055;&#1086;&#1075;&#1086;&#1076;&#1072;</div>";
    h += "<div class='grid'>";

    // Temperature
    h += "<div class='card'>"
         "<div class='icon'>&#127777;</div>"
         "<div class='lbl'>&#1058;&#1077;&#1084;&#1087;&#1077;&#1088;&#1072;&#1090;&#1091;&#1088;&#1072;</div>"
         "<div class='val ct' id='temp'>-<span class='unit'>&#176;C</span></div>"
         "</div>";

    // Pressure
    h += "<div class='card'>"
         "<div class='icon'>&#127754;</div>"
         "<div class='lbl'>&#1044;&#1072;&#1074;&#1083;&#1077;&#1085;&#1080;&#1077;</div>"
         "<div class='val cp' id='pres'>-<span class='unit'>&#1075;&#1055;&#1072;</span></div>"
         "<div class='sub' id='mmhg'>-</div>"
         "</div>";

    // Altitude
    h += "<div class='card'>"
         "<div class='icon'>&#9968;</div>"
         "<div class='lbl'>&#1042;&#1099;&#1089;&#1086;&#1090;&#1072;</div>"
         "<div class='val ca' id='alt'>-<span class='unit'>&#1084;</span></div>"
         "</div>";

    // QNH
    h += "<div class='card'>"
         "<div class='icon'>&#128202;</div>"
         "<div class='lbl'>QNH</div>"
         "<div class='val cq' id='qnh'>-<span class='unit'>&#1075;&#1055;&#1072;</span></div>"
         "</div>";

    // Air density
    h += "<div class='card'>"
         "<div class='icon'>&#128168;</div>"
         "<div class='lbl'>&#1055;&#1083;&#1086;&#1090;&#1085;&#1086;&#1089;&#1090;&#1100; &#1074;&#1086;&#1079;&#1076;&#1091;&#1093;&#1072;</div>"
         "<div class='val cd' id='dens'>-</div>"
         "<div class='sub'>&#1082;&#1075;/&#1084;&#179;</div>"
         "</div>";

    // Forecast
    h += "<div class='card'>"
         "<div class='icon cf' id='ficon'>?</div>"
         "<div class='lbl'>&#1055;&#1088;&#1086;&#1075;&#1085;&#1086;&#1079;</div>"
         "<div class='fore-val' id='fore'>-</div>"
         "<div class='sub' id='trend'>-</div>"
         "</div>";

    h += "</div>"; // .grid

    // ── SYSTEM ──
    h += "<div class='sec'>&#9881; &#1057;&#1080;&#1089;&#1090;&#1077;&#1084;&#1072;</div>";
    h += "<div class='tbl'>";

    // Chip temp + freq
    h += "<div class='row'>"
         "<span class='k'>&#1063;&#1080;&#1087; / CPU</span>"
         "<span class='rv'><span id='ctemp'>-</span>&nbsp;&middot;&nbsp;<span id='cpu'>-</span></span>"
         "</div>";

    // CPU load
    h += "<div class='row'>"
         "<span class='k'>&#1047;&#1072;&#1075;&#1088;&#1091;&#1079;&#1082;&#1072; CPU</span>"
         "<span class='rv'>"
         "<span id='cpuload'>-</span>"
         "<span class='bw'><span class='bar bar-o' id='cbar' style='width:0%'></span></span>"
         "</span></div>";

    // Heap
    h += "<div class='row'>"
         "<span class='k'>&#1055;&#1072;&#1084;&#1103;&#1090;&#1100; (heap)</span>"
         "<span class='rv'>"
         "<span id='heap'>-</span>"
         "<span class='bw'><span class='bar bar-b' id='hbar' style='width:0%'></span></span>"
         "</span></div>";

    // Uptime
    h += "<div class='row'>"
         "<span class='k'>&#1040;&#1087;&#1090;&#1072;&#1081;&#1084;</span>"
         "<span class='rv' id='up'>-</span>"
         "</div>";

    // SSID
    h += "<div class='row'>"
         "<span class='k'>SSID</span>"
         "<span class='rv' id='ssid'>-</span>"
         "</div>";

    // IP
    h += "<div class='row'>"
         "<span class='k'>IP</span>"
         "<span class='rv' id='ip'>-</span>"
         "</div>";

    // MAC
    h += "<div class='row'>"
         "<span class='k'>MAC</span>"
         "<span class='rv' style='font-size:.76rem' id='mac'>-</span>"
         "</div>";

    // RSSI
    h += "<div class='row'>"
         "<span class='k'>WiFi RSSI</span>"
         "<span class='rv'>"
         "<span id='rssi'>-</span>"
         "<span class='bw'><span class='bar bar-g' id='rbar' style='width:0%'></span></span>"
         "</span></div>";

    // Signal quality
    h += "<div class='row'>"
         "<span class='k'>&#1050;&#1072;&#1095;&#1077;&#1089;&#1090;&#1074;&#1086; &#1089;&#1080;&#1075;&#1085;&#1072;&#1083;&#1072;</span>"
         "<span class='rv' id='rssiQ'>-</span>"
         "</div>";

    h += "</div>"; // .tbl

    h += "<div class='footer'><span class='dot'></span>"
         "ESP32-C3 &middot; BMP280 &middot; <span id='ts'>-</span></div>";
    h += "</div>"; // .wrap

    // ── JavaScript ──
    h += "<script>";
    h += "function fmt(n,d){return(isNaN(n)||n===null)?'<span class=\"err\">-</span>':parseFloat(n).toFixed(d);}";
    h += "function uptime(s){var h=Math.floor(s/3600),m=Math.floor(s%3600/60),sc=s%60;"
         "return h+'\\u0447 '+String(m).padStart(2,'0')+'\\u043c '+String(sc).padStart(2,'0')+'\\u0441';}";
    h += "function setBar(id,pct){var el=document.getElementById(id);"
         "if(el)el.style.width=Math.min(100,Math.max(0,pct))+'%';}";

    h += "function load(){fetch('/api/data').then(r=>r.json()).then(d=>{";

    // Weather
    h += "if(d.weather&&d.weather.valid){var w=d.weather;";
    h += "document.getElementById('temp').innerHTML=fmt(w.temperature,1)+\"<span class='unit'>&#176;C</span>\";";
    h += "document.getElementById('pres').innerHTML=fmt(w.pressure,1)+\"<span class='unit'> &#1075;&#1055;&#1072;</span>\";";
    h += "document.getElementById('mmhg').textContent=fmt(w.pressure_mmhg,1)+' \\u043c\\u043c.\\u0440\\u0442.\\u0441\\u0442.';";
    h += "document.getElementById('alt').innerHTML=fmt(w.altitude,0)+\"<span class='unit'> \\u043c</span>\";";
    h += "document.getElementById('qnh').innerHTML=fmt(w.qnh,1)+\"<span class='unit'> &#1075;&#1055;&#1072;</span>\";";
    h += "document.getElementById('dens').textContent=fmt(w.air_density,4);";
    h += "var tr=parseFloat(w.trend);";
    h += "var tc=tr>0.05?'tu':tr<-0.05?'td':'te';";
    h += "var ta=tr>0.05?'\\u25b2 +':tr<-0.05?'\\u25bc ':' \\u2192 ';";
    h += "document.getElementById('trend').innerHTML='<span class=\"'+tc+'\">'+ta+fmt(Math.abs(tr),3)+' &#1075;&#1055;&#1072;/&#1095;</span>';";
    h += "document.getElementById('ficon').textContent=w.forecast_emoji;";
    h += "document.getElementById('fore').textContent=w.forecast_text;}";

    // System
    h += "if(d.system){var s=d.system;";
    h += "document.getElementById('ctemp').textContent=fmt(s.chip_temp,1)+'\\u00b0C';";
    h += "document.getElementById('cpu').textContent=s.cpu_mhz+' \\u041c\\u0413\\u0446';";
    h += "document.getElementById('cpuload').textContent=s.cpu_load+'%';";
    h += "setBar('cbar',s.cpu_load);";
    h += "var used=s.heap_used_pct;";
    h += "document.getElementById('heap').textContent=Math.round(s.free_heap/1024)+' \\u043a\\u0411 ('+used+'%)';";
    h += "setBar('hbar',used);";
    h += "document.getElementById('up').textContent=uptime(s.uptime_s);";
    h += "document.getElementById('ssid').textContent=s.ssid;";
    h += "document.getElementById('ip').textContent=s.ip;";
    h += "document.getElementById('mac').textContent=s.mac;";
    h += "document.getElementById('rssi').textContent=s.rssi+' dBm ('+s.rssi_pct+'%)';";
    h += "setBar('rbar',s.rssi_pct);";
    h += "document.getElementById('rssiQ').textContent=s.rssi_quality;}";

    h += "document.getElementById('ts').textContent=new Date().toLocaleTimeString('ru');";
    h += "}).catch(()=>{});}";
    h += "load();setInterval(load,";
    h += String(SENSOR_INTERVAL_MS);
    h += ");";
    h += "</script></body></html>";
    return h;
}

static String buildJson() {
    SysInfo sys = sysInfoRead();
    String j = "{\"weather\":{";
    if (_data && _data->valid) {
        j += "\"valid\":true,";
        j += "\"temperature\":"    + String(_data->temperature,  2) + ",";
        j += "\"pressure\":"       + String(_data->pressure,     2) + ",";
        j += "\"pressure_mmhg\":"  + String(_data->pressureMmHg, 2) + ",";
        j += "\"altitude\":"       + String(_data->altitude,      1) + ",";
        j += "\"qnh\":"            + String(_data->pressureQnh,   2) + ",";
        j += "\"air_density\":"    + String(_data->airDensity,    4) + ",";
        j += "\"trend\":"          + String(_data->pressureTrend, 3) + ",";
        j += "\"forecast_text\":\"" + String(forecastText(_data->forecastIcon))  + "\",";
        j += "\"forecast_emoji\":\"" + String(forecastEmoji(_data->forecastIcon)) + "\"";
    } else {
        j += "\"valid\":false";
    }
    j += "},\"system\":{";
    j += "\"chip_temp\":"     + String(sys.chipTempC,  1) + ",";
    j += "\"cpu_mhz\":"       + String(sys.cpuFreqMHz)     + ",";
    j += "\"cpu_load\":"      + String(sys.cpuLoadPct)      + ",";
    j += "\"free_heap\":"     + String(sys.freeHeap)         + ",";
    j += "\"total_heap\":"    + String(sys.totalHeap)         + ",";
    j += "\"heap_used_pct\":" + String(sys.heapUsedPct)       + ",";
    j += "\"rssi\":"          + String(sys.rssi)               + ",";
    j += "\"rssi_pct\":"      + String(sys.rssiPct)            + ",";
    j += "\"rssi_quality\":\"" + String(rssiQuality(sys.rssi)) + "\",";
    j += "\"ssid\":\""        + sys.ssid + "\",";
    j += "\"ip\":\""          + sys.ip   + "\",";
    j += "\"mac\":\""         + sys.mac  + "\",";
    j += "\"uptime_s\":"      + String(sys.uptimeS);
    j += "}}";
    return j;
}

static void handleRoot()     { server.send(200, "text/html; charset=utf-8", buildPage()); }
static void handleApi()      {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Cache-Control", "no-cache");
    server.send(200, "application/json", buildJson());
}
static void handleFavicon()  { server.send(204); }
static void handleNotFound() { server.send(404, "text/plain", "Not found"); }

void webServerInit(SensorData* dataPtr) {
    _data = dataPtr;
    server.on("/",            HTTP_GET, handleRoot);
    server.on("/api/data",    HTTP_GET, handleApi);
    server.on("/favicon.ico", HTTP_GET, handleFavicon);
    server.onNotFound(handleNotFound);
    server.begin();
    Serial.println("[HTTP] Server started on port 80");
    Serial.printf("[HTTP] http://%s.local\n", DEVICE_HOSTNAME);
}

void webServerHandle() { server.handleClient(); }
