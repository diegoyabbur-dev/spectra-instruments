#include "libraries.h"
#include <vector>   // para std::vector

// Servidor web en el puerto 80
WebServer server(80);

// CONFIGURACIÓN DE LA RED (AP)
const char* ssidAP     = "SpectraLog Insight";   // Nombre de la red AP
const char* passwordAP = "";                       // Contraseña de la red AP (opcional)

// EEPROM (solo para la etiqueta/ID)
#define EEPROM_SIZE       100       // Tamaño total de EEPROM
#define TAG_ADDRESS       20        // Dirección inicial para la etiqueta
#define MAX_TAG_LENGTH    32        // Longitud máxima de la etiqueta

// Variables globales visibles en otras partes del proyecto
extern int   sleepCount;
extern float WindSpeed_mps;
extern int   WindDir_deg;
extern int Rain_mm;
extern float waterCount;
extern float airHumidity;
extern float airTemp;
extern float pressure;
extern float battery;
extern String dateAP;
extern String timeAP;

// Variables del RTC (definidas en rtc.h/.cpp)
extern int year, month, day, hour, minute, second;
void rtcAdjust();
void createTimeStamp();

// Variables locales
String etiqueta = "123123";  // ID del dispositivo (por defecto)

// --- ENCENDIDO PEREZOSO DE SENSORES ---
static bool g_sensorsArmed = false;

// --- Lecturas del anemómetro en crudo (de tu sensors.cpp) ---
extern void  readAnemometer();
extern float windSpeed;     // crudo
extern int   windDirection; // crudo

// --- Formateo seguro con NaN ---
static inline String numOrNaN(float v, int dec=2) {
  if (isnan(v)) return "NaN";
  return String(v, dec);
}
static inline String intOrNaNFromSpeed(float speedRef, int v) {
  if (isnan(speedRef)) return "NaN";
  return String(v);
}

// Pequeño poll para “refrescar” anemómetro antes de responder
static inline void fastWindPoll(uint16_t ms = 120) {
  unsigned long t0 = millis();
  while (millis() - t0 < ms) {
    readAnemometer();
    delay(2);
  }
}


// ---------------------------------------------------------------------
// FUNCIONES DE EEPROM (guardar/leer etiqueta/ID)
// ---------------------------------------------------------------------
void guardarEtiqueta(const String& e) {
  int len = e.length();
  if (len >= MAX_TAG_LENGTH) len = MAX_TAG_LENGTH - 1;
  for (int i = 0; i < len; i++) EEPROM.write(TAG_ADDRESS + i, e[i]);
  EEPROM.write(TAG_ADDRESS + len, '\0');  // Terminador NULL
  EEPROM.commit();
}

String leerEtiqueta() {
  char buffer[MAX_TAG_LENGTH];
  for (int i = 0; i < MAX_TAG_LENGTH; i++) {
    buffer[i] = EEPROM.read(TAG_ADDRESS + i);
    if (buffer[i] == '\0') break;
  }
  return String(buffer);
}

// ---------------------------------------------------------------------
// HEADER Y FOOTER (HTML)
// ---------------------------------------------------------------------
String getHeader(const String& title) {
  return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>)rawliteral"
  + title +
  R"rawliteral(</title>
  <!-- Fuente Montserrat -->
  <link href="https://fonts.googleapis.com/css2?family=Montserrat&display=swap" rel="stylesheet">
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Montserrat', sans-serif; }
    body { background-color: #121212; color: #FFFFFF; margin: 0; }
    .navbar { background-color: #121212; display: flex; align-items: center; justify-content: space-between; padding: 0.8em 1em; border-bottom: 1px solid #2C2C2C; }
    .navbar h2 { font-size: 1.2em; margin: 0; }
    .menu-icon { width: 30px; height: 30px; cursor: pointer; display: block; position: relative; }
    .menu-icon div { background-color: #FFFFFF; height: 3px; margin: 5px 0; border-radius: 2px; transition: 0.4s; }
    .nav-links { position: absolute; top: 58px; left: 0; width: 100%; background-color: #1F1F1F; display: none; flex-direction: column; border-bottom: 1px solid #2C2C2C; }
    .nav-links a { color: #FFFFFF; text-decoration: none; padding: 0.75em 1em; border-bottom: 1px solid #2C2C2C; }
    .nav-links a:hover { background-color: #2A2A2A; }
    .nav-links.show { display: flex; }
    .container { max-width: 1200px; margin: 0 auto; padding: 1em; }
    .card { background-color: #1F1F1F; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.4); padding: 1em; margin: 1em 0; }
    .card h2 { margin-bottom: 0.75em; }
    .measurements { background-color: #1F1F1F; margin: 1em 0; padding: 1em; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.4); }
    .measurements h3 { margin-bottom: 0.5em; }
    .measurements ul { list-style: none; }
    .measurements li { padding: 0.5em 0; border-bottom: 1px solid #2C2C2C; }
    .measurements li:last-child { border-bottom: none; }
    label { margin-top: 0.5em; display: block; }
    input, select { margin-top: 0.3em; padding: 0.5em; background-color: #121212; border: 1px solid #333333; color: #FFFFFF; border-radius: 4px; width: 100%; }
    button { padding: 0.6em 1em; border: none; border-radius: 4px; cursor: pointer; background-color: rgb(56, 56, 56); color: #F5F5F5; margin-top: 0.5em; }
    button:hover { background-color: rgb(41, 41, 41); }
    a button { text-decoration: none; }
    footer { text-align: center; padding: 1em 0; color: #AAAAAA; font-size: 0.9em; border-top: 1px solid #2C2C2C; margin-top: 2em; }
    footer p { margin: 0.3em 0; }
    @media(min-width: 768px) {
      .nav-links { position: static; display: flex; flex-direction: row; width: auto; background-color: transparent; border-bottom: none; }
      .nav-links a { border-bottom: none; }
      .menu-icon { display: none; }
    }
  </style>
  <script>
    function toggleMenu() {
      var nav = document.getElementById('navLinks');
      nav.classList.toggle('show');
    }
  </script>
</head>
<body>
<div class="navbar">
  <h2>SpectraLog Insight</h2>
  <div class="menu-icon" onclick="toggleMenu()">
    <div></div>
    <div></div>
    <div></div>
  </div>
</div>
<div class="nav-links" id="navLinks">
  <a href="/">Inicio</a>
  <a href="/settings">Configuración</a>
  <a href="/about">Acerca de</a>
  <a href="/download">Descarga</a>
</div>
<div class="container">
)rawliteral";
}

String getFooter() {
  return R"rawliteral(
</div>
<footer>
  <p>© 2025 Abyssal Innovation. All rights reserved</p>
</footer>
</body>
</html>
)rawliteral";
}

// ---------------------------------------------------------------------
// PÁGINA DE INICIO
// ---------------------------------------------------------------------
void handleHome() {
  String deviceID        = etiqueta;
  String macAddress      = WiFi.softAPmacAddress();
  String firmwareVersion = "v1.1.4"; // fijo o trae tu define

  String page = getHeader("SpectraSense - Inicio");

  page += R"rawliteral(
    <div class="card">
      <h2 style="margin-bottom: 1em;">Estado del Dispositivo</h2>
      <ul style="list-style: none; padding-left: 0; margin: 0;">
        <li style="padding: 0.6em 0; border-bottom: 1px solid #2C2C2C;">
          <strong>Device ID:</strong> )rawliteral" + deviceID + R"rawliteral(
        </li>
        <li style="padding: 0.6em 0; border-bottom: 1px solid #2C2C2C;">
          <strong>MAC Address:</strong> )rawliteral" + macAddress + R"rawliteral(
        </li>
        <li style="padding: 0.6em 0; border-bottom: 1px solid #2C2C2C;">
          <strong>Firmware:</strong> )rawliteral" + firmwareVersion + R"rawliteral(
        </li>
        <li style="padding: 0.6em 0; border-bottom: 1px solid #2C2C2C;">
          <strong>Batería:</strong> <span id="batterySpan">-</span>
        </li>
        <li style="padding: 0.6em 0; border-bottom: 1px solid #2C2C2C;">
          <strong>Fecha:</strong> <span id="dateSpan">-</span>
        </li>
        <li style="padding: 0.6em 0;">
          <strong>Hora:</strong> <span id="timeSpan">-</span>
        </li>
      </ul>
    </div>
  )rawliteral";

  page += R"rawliteral(
  <div class="measurements">
    <h3>Mediciones</h3>
    <ul>
      <li><strong>Humedad:</strong> <span id="humSpan">-</span>%</li>
      <li><strong>Temperatura:</strong> <span id="tempSpan">-</span>°C</li>
      <li><strong>Presión:</strong> <span id="presSpan">-</span>hPa</li>
      <li><strong>Lluvia (Rainfall):</strong> <span id="rainSpan">-</span>mm</li>
      <li><strong>Agua Recolectada:</strong> <span id="waterSpan">-</span>L</li>
      <li><strong>Velocidad Viento:</strong> <span id="windSpSpan">-</span>m/s</li>
      <li><strong>Dirección Viento:</strong> <span id="windDirSpan">-</span>°</li>
    </ul>
  </div>
)rawliteral";

  page += R"rawliteral(
    <div style="margin-top:1em; text-align:center;">
      <button onclick="location.href='/restart'">Desconectar</button>
    </div>

    <script>
  function fetchSensorData() {
    fetch('/sensorData')
      .then(r => r.json())
      .then(data => {
        document.getElementById('humSpan').innerText     = data.humedad;
        document.getElementById('tempSpan').innerText    = data.temperatura;
        document.getElementById('presSpan').innerText    = data.presion;
        document.getElementById('rainSpan').innerText    = data.rainfall;
        document.getElementById('waterSpan').innerText   = data.waterCollected;
        document.getElementById('windSpSpan').innerText  = data.windSpeed;
        document.getElementById('windDirSpan').innerText = data.windDir;

        document.getElementById('batterySpan').innerText = data.battery;
        document.getElementById('dateSpan').innerText    = data.date;
        document.getElementById('timeSpan').innerText    = data.time;
      })
      .catch(console.log);
  }
  setInterval(fetchSensorData, 500);  // <-- antes 2000
  fetchSensorData();
</script>

)rawliteral";

  page += getFooter();
  server.send(200, "text/html", page);
}

// ---------------------------------------------------------------------
// RUTA /sensorData: devuelve JSON con mediciones (y batería/fecha/hora)
// ---------------------------------------------------------------------
void handleSensorData() {
  // Enciende sensores al primer request (si aplica en tu hardware)
  // Refresca anemómetro un instante
  //fastWindPoll(120);

  // Si quieres exponer también por las globals “públicas”:
  WindSpeed_mps = windSpeed;      // viene de sensors.cpp
  WindDir_deg   = windDirection;

  // Sella fecha/hora si tu lógica lo hace aquí
  createTimeStamp();

  // Empaqueta NaN como cadena "NaN" para evitar JSON inválido
  String humStr     = numOrNaN(airHumidity, 2);
  String tempStr    = numOrNaN(airTemp, 2);
  String presStr    = numOrNaN(pressure, 2);
  String windSpStr  = numOrNaN(WindSpeed_mps, 2);
  String windDirStr = intOrNaNFromSpeed(WindSpeed_mps, WindDir_deg);
  String rainStr    = numOrNaN((float)Rain_mm, 0); // si Rain_mm puede faltar, lo fuerzas a NaN si procede
  String waterStr   = numOrNaN(waterCount, 2);
  String battStr    = numOrNaN(battery, 2);

  // Construye JSON (números válidos sin comillas y NaN como "NaN")
  String json = "{";
  json += "\"humedad\":"        + (humStr     == "NaN" ? "\"NaN\"" : humStr)     + ",";
  json += "\"temperatura\":"    + (tempStr    == "NaN" ? "\"NaN\"" : tempStr)    + ",";
  json += "\"presion\":"        + (presStr    == "NaN" ? "\"NaN\"" : presStr)    + ",";
  json += "\"rainfall\":"       + (rainStr    == "NaN" ? "\"NaN\"" : rainStr)    + ",";
  json += "\"waterCollected\":" + (waterStr   == "NaN" ? "\"NaN\"" : waterStr)   + ",";
  json += "\"windSpeed\":"      + (windSpStr  == "NaN" ? "\"NaN\"" : windSpStr)  + ",";
  json += "\"windDir\":"        + (windDirStr == "NaN" ? "\"NaN\"" : windDirStr) + ",";
  json += "\"battery\":"        + (battStr    == "NaN" ? "\"NaN\"" : battStr)    + ",";
  json += "\"date\":\""         + dateAP + "\",";
  json += "\"time\":\""         + timeAP + "\"";
  json += "}";

  server.send(200, "application/json", json);
}


// ---------------------------------------------------------------------
// SETTINGS / ABOUT / DOWNLOAD
// ---------------------------------------------------------------------
void handleSettings() {
  EEPROM.get(4, sleepCount);
  int intervaloMinutos = sleepCount;

  String page = getHeader("SpectraSense - Configuración");

  page += R"rawliteral(
    <div class="card">
      <h2>Configurar Intervalo de Medición</h2>
      <form method="GET" action="/saveMeasurementInterval">
        <label for="intervalValue">Valor del Intervalo (Minutos):</label>
        <input type="number" id="intervalValue" name="intervalValue" min="1" value=")rawliteral";
  page += String(intervaloMinutos);
  page += R"rawliteral(" />
        <div style="display: flex; gap: 1em; margin-top: 1em;">
          <button type="submit">Guardar</button>
        </div>
      </form>
    </div>
  )rawliteral";

  page += R"rawliteral(
  <div class="card">
    <h2 style="margin-bottom: 1em;">Configurar Fecha y Hora</h2>
    <form method="GET" action="/setDateTimeManual" style="margin-bottom: 0.8em;">
      <label for="datetime">Fecha/Hora Manual:</label>
      <input type="datetime-local" id="datetime" name="datetime" style="width: 100%;" />
      <div style="display: flex; gap: 0.8em; margin-top: 0.6em;">
        <button type="submit" style="flex: 1;">Guardar</button>
        <button type="button" id="syncLocalTime" style="flex: 1;">Usar Hora del Dispositivo</button>
      </div>
    </form>
    <script>
      document.getElementById('syncLocalTime').addEventListener('click', function() {
        var now    = new Date();
        var year   = now.getFullYear();
        var month  = now.getMonth() + 1;
        var day    = now.getDate();
        var hour   = now.getHours();
        var minute = now.getMinutes();
        var second = now.getSeconds();
        var url = `/setDeviceTime?year=${year}&month=${month}&day=${day}&hour=${hour}&minute=${minute}&second=${second}`;
        fetch(url).then(r => r.text()).then(alert).catch(console.error);
      });
    </script>
  </div>
)rawliteral";

  page += R"rawliteral(
  <div class="card">
    <h2>Cambiar ID del Dispositivo</h2>
    <form method="GET" action="/changeDeviceID">
      <label for="deviceID">Nuevo ID:</label>
      <input type="text" id="deviceID" name="deviceID" placeholder="Ej: INSIGHT-1234" />
      <button type="submit">Cambiar ID</button>
    </form>
  </div>

  <div class="card">
    <h2>Configurar LoRa E29 (Ebyte)</h2>
    <form method="GET" action="/configLoRa">
      <label for="loraAddress">Dirección:</label>
      <input type="number" id="loraAddress" name="loraAddress" value="0" min="0" /><br />
      <label for="loraParam1">Parámetro 1 (ej. potencia):</label>
      <input type="number" id="loraParam1" name="loraParam1" value="10" /><br />
      <label for="loraParam2">Parámetro 2 (ej. canal):</label>
      <input type="number" id="loraParam2" name="loraParam2" value="1" /><br />
      <button type="submit">Guardar Config. LoRa</button>
    </form>
    <br />
    <h3>Test LoRa</h3>
    <form method="GET" action="/testLoRaSend">
      <button type="submit">Enviar Mensaje de Prueba</button>
    </form>
    <form method="GET" action="/testLoRaReceive" style="margin-top:0.5em;">
      <button type="submit">Recibir Mensaje de Prueba</button>
    </form>
  </div>
)rawliteral";

  page += getFooter();
  server.send(200, "text/html", page);
}

void handleAbout() {
  String page = getHeader("SpectraSense - Acerca de");
  page += R"rawliteral(
  <div class="card">
    <h2 style="margin-bottom: 1em;">Acerca de este dispositivo</h2>
    <p style="margin-bottom: 1em;">
      Este dispositivo ha sido desarrollado íntegramente por <strong>Abyssal Innovation</strong>
      como parte del sistema <strong>SpectraSense</strong>, un datalogger ambiental diseñado para
      operar de forma autónoma en terreno y facilitar el monitoreo científico y técnico en condiciones reales.
    </p>
    <p style="margin-bottom: 0.5em;">A través de esta plataforma local, es posible:</p>
    <ul style="margin-left: 1.2em; margin-bottom: 1em;">
      <li>Visualizar el estado del dispositivo, sensores y batería en tiempo real.</li>
      <li>Configurar parámetros como el intervalo de medición, fecha y hora, ID del equipo y comunicación LoRa.</li>
      <li>Descargar registros históricos en formatos CSV o JSON.</li>
      <li>Eliminar los datos almacenados cuando sea necesario.</li>
    </ul>
    <p style="margin-bottom: 1em;">
      El dispositivo actúa como un punto de acceso Wi-Fi (AP), permitiendo que cualquier usuario
      se conecte directamente desde un celular o notebook, sin requerir conexión a internet.
    </p>
    <p style="margin-bottom: 1.5em;">
      Tanto el <strong>hardware</strong> como el <strong>software</strong>
      han sido desarrollados por completo por <strong>Abyssal Innovation</strong>,
      con un enfoque en la precisión, eficiencia energética y facilidad de uso en entornos exigentes.
    </p>
    <hr style="border: none; border-top: 1px solid #444; margin-bottom: 1.5em;" />
    <p><strong>Desarrollado por:</strong> Abyssal Innovation</p>
    <p><strong>Versión:</strong> 1.1.4</p>
    <p><strong>Contacto:</strong> <a href="mailto:contacto@abyssalinnovation.com" style="color:#BB86FC;">contacto@abyssalinnovation.com</a></p>
    <p style="margin-top: 1.5em; font-style: italic; color: #AAAAAA;">
      Tecnología precisa, adaptable y sin complicaciones. Diseñada para entornos reales.
    </p>
  </div>
)rawliteral";
  page += getFooter();
  server.send(200, "text/html", page);
}

void handleDownload() {
  String page = getHeader("SpectraSense - Descarga de Datos");

  page += R"rawliteral(
  <div class="card">
    <h2>Descarga de Datos</h2>
    <p>Selecciona el rango de fechas y el formato deseado.</p>

    <!-- Formulario 1: rango de fechas -->
    <form method="GET" action="/downloadDataRange" id="rangeForm">
      <label for="startDate">Fecha Inicio:</label>
      <input type="date" id="startDate" name="startDate" />

      <label for="endDate">Fecha Fin:</label>
      <input type="date" id="endDate" name="endDate" />

      <label for="format">Formato:</label>
      <select id="format" name="format">
        <option value="csv">CSV</option>
        <option value="json">JSON</option>
      </select>

      <div style="display: flex; gap: 1em; margin-top: 1em;">
        <button type="submit">Descargar Rango</button>
      </div>
    </form>

    <!-- Formulario 2: descargar TODO (no anidado) -->
    <form method="GET" action="/downloadAll" id="allForm" style="margin-top: 1em;">
      <input type="hidden" name="format" id="allFormat" value="csv" />
      <button type="submit">Descargar Todo</button>
    </form>

    <script>
      // Copia el formato seleccionado al form "Descargar Todo"
      document.getElementById("allForm").addEventListener("submit", function () {
        const formatSel = document.getElementById("format").value;
        document.getElementById("allFormat").value = formatSel;
      });
    </script>

    <br />
    <h3>Eliminar Datos</h3>
    <form method="GET" action="/deleteData" onsubmit="return confirmDelete();">
      <button type="submit" style="background-color:rgb(182, 8, 40); color: #FFFFFF;">Eliminar Todos los Datos</button>
    </form>

    <script>
      function confirmDelete() {
        return confirm("¿Estás seguro que quieres eliminar TODOS los datos?\nEsta acción no se puede deshacer.");
      }
    </script>

  </div>
)rawliteral";

  page += getFooter();
  server.send(200, "text/html", page);
}

// ---------------------------------------------------------------------
// DESCARGA: TODO (CSV/JSON)
// ---------------------------------------------------------------------
void handleDownloadAll() {
  if (!server.hasArg("format")) {
    server.send(400, "text/plain", "Formato no especificado.");
    return;
  }

  String fmt = server.arg("format");
  File file = SPIFFS.open("/data.csv", "r");
  if (!file) {
    server.send(404, "text/plain", "Archivo data.csv no encontrado.");
    return;
  }

  if (fmt == "csv") {
    String downloadName = "SpectraSense_" + etiqueta + "_COMPLETO.csv";
    server.sendHeader("Content-Disposition", "attachment; filename=" + downloadName);
    server.sendHeader("Content-Type", "text/csv");
    server.sendHeader("Connection", "close");
    server.streamFile(file, "text/csv"); // envía TODO el archivo tal cual
    file.close();
  } else if (fmt == "json") {
    File jsonFile = SPIFFS.open("/temp_all.json", "w");
    if (!jsonFile) {
      file.close();
      server.send(500, "text/plain", "Error al crear archivo temporal JSON.");
      return;
    }

    jsonFile.print("{\"data\":[\n");
    file.readStringUntil('\n'); // Saltar encabezado
    bool firstLine = true;

    while (file.available()) {
      String line = file.readStringUntil('\n');
      line.trim();
      if (line.length() == 0) continue;

      if (!firstLine) jsonFile.print(",\n");
      firstLine = false;

      // Split CSV: Date, Time, WindSpeed_mps, WindDir_deg, Rain_mm
      std::vector<String> parts;
      int from = 0;
      while (from < (int)line.length()) {
        int comma = line.indexOf(',', from);
        if (comma == -1) comma = line.length();
        parts.push_back(line.substring(from, comma));
        from = comma + 1;
      }

      if (parts.size() >= 5) {
        jsonFile.print("{");
        jsonFile.print("\"fecha\":\"" + parts[0] + " " + parts[1] + "\",");
        jsonFile.print("\"WindSpeed_mps\":" + parts[2] + ",");
        jsonFile.print("\"WindDir_deg\":" + parts[3] + ",");
        jsonFile.print("\"Rain_mm\":" + parts[4]);
        jsonFile.print("}");
      }
    }

    jsonFile.print("\n]}");
    file.close();
    jsonFile.close();

    File jsonDownload = SPIFFS.open("/temp_all.json", "r");
    if (jsonDownload) {
      String downloadName = "SpectraSense_" + etiqueta + "_COMPLETO.json";
      server.sendHeader("Content-Disposition", "attachment; filename=" + downloadName);
      server.sendHeader("Content-Type", "application/json");
      server.sendHeader("Connection", "close");
      server.streamFile(jsonDownload, "application/json");
      jsonDownload.close();
      SPIFFS.remove("/temp_all.json");
    } else {
      server.send(500, "text/plain", "Error al abrir el archivo JSON generado.");
    }
  } else {
    server.send(400, "text/plain", "Formato no soportado.");
  }
}

// ---------------------------------------------------------------------
// DESCARGA DE DATOS POR RANGO
// ---------------------------------------------------------------------
void handleDownloadDataRange() {
  if (server.hasArg("startDate") && server.hasArg("endDate") && server.hasArg("format")) {
    String start = server.arg("startDate");  // YYYY-MM-DD
    String end   = server.arg("endDate");
    String fmt   = server.arg("format");

    Serial.printf("[INFO] Descargando datos desde %s hasta %s en formato %s\n",
                  start.c_str(), end.c_str(), fmt.c_str());

    File original = SPIFFS.open("/data.csv", "r");
    if (!original) {
      server.send(404, "text/plain", "Archivo de datos no encontrado.");
      return;
    }

    String extension = (fmt == "csv") ? ".csv" : ".json";
    String filename  = "/temp_download" + extension;

    File output = SPIFFS.open(filename, "w");
    if (!output) {
      original.close();
      server.send(500, "text/plain", "Error al crear archivo temporal.");
      return;
    }

    if (fmt == "csv") {
      String header = original.readStringUntil('\n');
      output.println(header);
      while (original.available()) {
        String line = original.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;

        int firstComma = line.indexOf(',');
        String fecha   = (firstComma > 0) ? line.substring(0, firstComma) : "";
        fecha.replace("/", "-");

        if (fecha >= start && fecha <= end) output.println(line);
      }
    } else { // json
      output.print("{\"data\":[\n");
      original.readStringUntil('\n');  // Saltar encabezado
      bool firstLine = true;

      while (original.available()) {
        String line = original.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;

        int firstComma = line.indexOf(',');
        String fecha   = (firstComma > 0) ? line.substring(0, firstComma) : "";
        fecha.replace("/", "-");

        if (fecha >= start && fecha <= end) {
          if (!firstLine) output.print(",\n");
          firstLine = false;

          // Split CSV
          std::vector<String> parts;
          int from = 0;
          while (from < (int)line.length()) {
            int comma = line.indexOf(',', from);
            if (comma == -1) comma = line.length();
            parts.push_back(line.substring(from, comma));
            from = comma + 1;
          }

          if (parts.size() >= 5) {
            output.print("{");
            output.print("\"fecha\":\"" + parts[0] + " " + parts[1] + "\",");
            output.print("\"WindSpeed_mps\":" + parts[2] + ",");
            output.print("\"WindDir_deg\":" + parts[3] + ",");
            output.print("\"Rain_mm\":" + parts[4]);
            output.print("}");
          }
        }
      }
      output.print("\n]}");
    }

    original.close();
    output.close();

    File downloadFile = SPIFFS.open(filename, "r");
    if (downloadFile) {
      String downloadName = "SpectraSense_" + etiqueta + extension;
      server.sendHeader("Content-Disposition", "attachment; filename=" + downloadName);
      server.sendHeader("Connection", "close");
      server.streamFile(downloadFile, (fmt == "csv") ? "text/csv" : "application/json");
      downloadFile.close();
      SPIFFS.remove(filename);
    } else {
      server.send(500, "text/plain", "Error al abrir archivo para descargar.");
    }
  } else {
    server.sendHeader("Location", "/download");
    server.send(303);
  }
}

// ---------------------------------------------------------------------
// GUARDAR INTERVALO / FECHA-HORA / CAMBIAR ID
// ---------------------------------------------------------------------
void handleSaveMeasurementInterval() {
  if (server.hasArg("intervalValue")) {
    String val  = server.arg("intervalValue");
    sleepCount  = val.toInt();
    EEPROM.put(4, sleepCount);
    EEPROM.commit();
    Serial.printf("[INFO] Intervalo guardado en EEPROM: %d min\n", sleepCount);
  }
  server.sendHeader("Location", "/settings");
  server.send(303);
}

void handleSetDateTimeManual() {
  if (server.hasArg("datetime")) {
    String dt = server.arg("datetime");
    Serial.println("[INFO] Fecha/Hora manual recibida: " + dt);

    int yearVal   = dt.substring(0, 4).toInt();
    int monthVal  = dt.substring(5, 7).toInt();
    int dayVal    = dt.substring(8, 10).toInt();
    int hourVal   = dt.substring(11, 13).toInt();
    int minuteVal = dt.substring(14, 16).toInt();
    int secondVal = 0;

    year   = yearVal;
    month  = monthVal;
    day    = dayVal;
    hour   = hourVal;
    minute = minuteVal;
    second = secondVal;

    rtcAdjust();

    Serial.printf("[INFO] Ajustando RTC a: %04d-%02d-%02d %02d:%02d:%02d\n",
                  year, month, day, hour, minute, second);
  }
  server.sendHeader("Location", "/settings");
  server.send(303);
}

void handleSetDateTimeAuto() {
  Serial.println("[INFO] Sincronizando fecha/hora automáticamente...");
  server.sendHeader("Location", "/settings");
  server.send(303);
}

// <<< FALTABA ESTA FUNCIÓN >>>
void handleSetDeviceTime() {
  if (server.hasArg("year") && server.hasArg("month") && server.hasArg("day") &&
      server.hasArg("hour") && server.hasArg("minute") && server.hasArg("second")) {

    year   = server.arg("year").toInt();
    month  = server.arg("month").toInt();
    day    = server.arg("day").toInt();
    hour   = server.arg("hour").toInt();
    minute = server.arg("minute").toInt();
    second = server.arg("second").toInt();

    rtcAdjust();

    String msg = "[INFO] Hora local del dispositivo ajustada a: ";
    msg += String(year) + "-" + String(month) + "-" + String(day) + " ";
    msg += String(hour) + ":" + String(minute) + ":" + String(second);
    server.send(200, "text/plain", msg);
    Serial.println(msg);
  } else {
    server.send(400, "text/plain", "Faltan parámetros (year, month, day, hour, minute, second)");
  }
}

void handleChangeDeviceID() {
  if (server.hasArg("deviceID")) {
    String newID = server.arg("deviceID");
    guardarEtiqueta(newID);
    etiqueta = newID;
    Serial.println("[INFO] Nuevo ID dispositivo: " + newID);
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

// ---------------------------------------------------------------------
// CONFIGURACIÓN LORA / TEST
// ---------------------------------------------------------------------
void handleConfigLoRa() {
  if (server.hasArg("loraAddress") && server.hasArg("loraParam1") && server.hasArg("loraParam2")) {
    String addr   = server.arg("loraAddress");
    String param1 = server.arg("loraParam1");
    String param2 = server.arg("loraParam2");
    Serial.printf("[INFO] Config LoRa -> Direccion: %s, Param1: %s, Param2: %s\n",
                  addr.c_str(), param1.c_str(), param2.c_str());
    // Tu lógica real para configurar el módulo LoRa
  }
  server.sendHeader("Location", "/settings");
  server.send(303);
}

void handleTestLoRaSend() {
  Serial.println("[INFO] Enviando mensaje LoRa de prueba...");
  sendLoRaData(); // Tu función real
  server.sendHeader("Location", "/settings");
  server.send(303);
}

void handleTestLoRaReceive() {
  Serial.println("[INFO] Recibiendo mensaje LoRa de prueba...");
  // Tu lógica real
  server.sendHeader("Location", "/settings");
  server.send(303);
}

// ---------------------------------------------------------------------
// ELIMINAR DATOS
// ---------------------------------------------------------------------
void handleDeleteData() {
  if (SPIFFS.exists("/data.csv")) {
    if (SPIFFS.remove("/data.csv")) {
      File file = SPIFFS.open("/data.csv", "w");
      if (file) {
        file.println("Date,Time,WindSpeed_mps,WindDir_deg,Rain_mm");
        file.close();
        server.sendHeader("Location", "/deleteSuccess");
        server.send(303);
      } else {
        server.send(500, "text/plain", "Error al crear el archivo nuevo.");
      }
    } else {
      server.send(500, "text/plain", "Error al borrar el archivo CSV.");
    }
  } else {
    server.send(404, "text/plain", "Archivo data.csv no encontrado.");
  }
}

void handleDeleteSuccess() {
  String page = getHeader("Datos eliminados");
  page += R"rawliteral(
    <script>
      alert("✅ Todos los datos han sido eliminados exitosamente.");
      window.location.href = "/download";
    </script>
  )rawliteral";
  page += getFooter();
  server.send(200, "text/html", page);
}

// ---------------------------------------------------------------------
// RUTA PARA REINICIAR (BOTÓN DESCONECTAR) / 404
// ---------------------------------------------------------------------
void handleRestart() {
  server.send(200, "text/plain", "Reiniciando ESP32...");
  delay(100);
  ESP.restart();
}

void handleNotFound() {
  server.send(404, "text/plain", "404: No se encontró la página");
}

// ---------------------------------------------------------------------
// INICIALIZAR SERVIDOR
// ---------------------------------------------------------------------
void initWebServer_V2() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssidAP, passwordAP);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP inicializado. IP del AP: ");
  Serial.println(IP);

  EEPROM.begin(EEPROM_SIZE);

  etiqueta = leerEtiqueta();
  if (etiqueta.length() == 0 || etiqueta == "0") {
    etiqueta = "ESP32_Dev";
    guardarEtiqueta(etiqueta);
  }

  // Rutas
  server.on("/", handleHome);
  server.on("/settings", handleSettings);
  server.on("/about", handleAbout);
  server.on("/download", handleDownload);

  server.on("/sensorData", handleSensorData);
  server.on("/restart", handleRestart);

  // Config
  server.on("/saveMeasurementInterval", handleSaveMeasurementInterval);
  server.on("/setDateTimeManual", handleSetDateTimeManual);
  server.on("/setDateTimeAuto", handleSetDateTimeAuto);
  server.on("/changeDeviceID", handleChangeDeviceID);
  server.on("/setDeviceTime", handleSetDeviceTime); // <- ahora existe

  // LoRa
  server.on("/configLoRa", handleConfigLoRa);
  server.on("/testLoRaSend", handleTestLoRaSend);
  server.on("/testLoRaReceive", handleTestLoRaReceive);

  // Descarga
  server.on("/downloadDataRange", handleDownloadDataRange);
  server.on("/downloadAll", handleDownloadAll);

  // Eliminar datos
  server.on("/deleteData", handleDeleteData);
  server.on("/deleteSuccess", handleDeleteSuccess);

  // 404
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("Servidor web iniciado. Esperando peticiones...");
}

void runServer_V2() {
  server.handleClient();
}
