// ======================= SpectraLog Wind - main.cpp (clean logs) =======================
// DS3231 + Pluviómetro + AP + ULP Ultrasonic (NMEA MWV) por UART2
// Power rails: powerSupplyON()/powerSupplyOFF()
// --------------------------------------------------------------------------------------

#include "libraries.h"
#include <EEPROM.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include <RTClib.h>
#include <math.h>
#include "rtc.h"
#include "webserver_v2.h"
#include "sensors.h"

// ====== CONFIG LOGS ======
#ifndef LOG_LEVEL
#define LOG_LEVEL 1   // 0 = silencioso, 1 = normal, 2 = verbose
#endif
#ifndef VERBOSE_RAIN
#define VERBOSE_RAIN 0  // 0 = compacto, 1 = detalles antirebote
#endif

// ====== OPCIÓN DEBUG: forzar intervalo (min) sin tocar EEPROM ======
#ifndef DEBUG_INTERVAL_MIN
#define DEBUG_INTERVAL_MIN 5
#endif

// ====== EEPROM layout ======
#define EE_ADDR_PULSE_COUNT   0
#define EE_ADDR_INTERVAL_MIN  4
#define EE_ADDR_NEXT_EPOCH    8

// ====== Pines de wake ======
const gpio_num_t buttonPin = GPIO_NUM_39; // Botón AP (a GND)
const gpio_num_t pulsePin  = GPIO_NUM_33; // Reed pluviómetro (a GND)

// ====== ULP ======
static const uint32_t ULP_WARMUP_MS  = 2000; // 12 s en prod
static const uint32_t ULP_CAPTURE_MS = 2500; // 2.5 s

// ====== RTC RAM ======
RTC_DATA_ATTR uint8_t  lockout_timer_pending = 0;
RTC_DATA_ATTR uint32_t next_cycle_epoch_rtc  = 0;
RTC_DATA_ATTR uint32_t last_tip_epoch_rtc    = 0;

// ====== Globals externos ======
extern int   sleepCount;
extern float WindSpeed_mps;
extern int   WindDir_deg;
extern bool  accessPointActive;
extern volatile int pulseCount;

extern float windSpeed;      // sensors.cpp
extern int   windDirection;  // sensors.cpp

#ifndef SERIAL2_RXPIN
#define SERIAL2_RXPIN 13
#endif
#ifndef SERIAL2_TXPIN
#define SERIAL2_TXPIN 15
#endif

// ===== Pines de tu placa (LoRa) =====
#define LORA_RX_PIN 19    // ESP32 RX  <- TX del módulo
#define LORA_TX_PIN 32    // ESP32 TX  -> RX del módulo
#define PIN_M0      25
#define PIN_M1      27

const int powerLoRa = 26;   // Switch que ALIMENTA el LoRa (HIGH = ON)

static inline void mode_normal() { digitalWrite(PIN_M0, LOW); digitalWrite(PIN_M1, LOW); }

// ============================== Helpers de log ===============================
static inline void logSection(const char* title) {
  if (LOG_LEVEL == 0) return;
  Serial.println();
  Serial.print("=== "); Serial.print(title); Serial.println(" ===");
}

static inline void logKV(const char* k, const String& v) {
  if (LOG_LEVEL == 0) return;
  Serial.print("• "); Serial.print(k); Serial.print(": "); Serial.println(v);
}
static inline void logKV(const char* k, uint32_t v) { if (LOG_LEVEL == 0) return; Serial.printf("• %s: %u\n", k, v); }
static inline void logKV(const char* k, int32_t v)  { if (LOG_LEVEL == 0) return; Serial.printf("• %s: %d\n", k, v); }
static inline void logKV(const char* k, float v, int d=2) { if (LOG_LEVEL == 0) return; Serial.printf("• %s: %.*f\n", k, d, v); }

// ============================== Tiempo/EEPROM ===============================
static inline void eepromBeginIfNeeded() {
  static bool started = false;
  if (!started) { EEPROM.begin(512); started = true; }
}
static inline void eePutU32(int addr, uint32_t v){ eepromBeginIfNeeded(); EEPROM.put(addr, v); EEPROM.commit(); }
static inline uint32_t eeGetU32(int addr){ eepromBeginIfNeeded(); uint32_t v=0; EEPROM.get(addr, v); return v; }

static bool isIntervalSane(int m) { return (m >= 1 && m <= 1440); }
static bool isEpochSane(uint32_t e, uint32_t now) {
  if (e == 0 || e == 0xFFFFFFFF) return false;
  const uint32_t THREE_YEARS = 3u * 365u * 24u * 3600u;
  uint32_t diff = (e > now) ? (e - now) : (now - e);
  return diff <= THREE_YEARS;
}
static String fmtDateTime(const DateTime& dt) {
  char buf[20];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
           dt.year(), dt.month(), dt.day(), dt.hour(), dt.minute(), dt.second());
  return String(buf);
}
static String fmtFromEpoch(uint32_t epoch) { DateTime dt(epoch); return fmtDateTime(dt); }

static uint32_t rtcEpoch() {
  createTimeStamp();
  struct tm t = {};
  t.tm_year = year - 1900; t.tm_mon = month - 1; t.tm_mday = day;
  t.tm_hour = hour; t.tm_min = minute; t.tm_sec = second;
  return (uint32_t)mktime(&t);
}
static uint32_t advanceNextToFuture(uint32_t nextEpoch, uint32_t nowEpoch, uint32_t intervalMinutes) {
  uint32_t step = intervalMinutes * 60; if (step < 60) step = 60;
  while ((int64_t)nextEpoch - (int64_t)nowEpoch <= 0) nextEpoch += step;
  return nextEpoch;
}

// ============================== Impresiones compactas ========================
static void printScheduler(uint32_t now, uint32_t stored, int intervalMin, uint32_t next) {
  logSection("Scheduler");
  logKV("Ahora [epoch]", now);
  logKV("Ahora [fecha]" , fmtFromEpoch(now));
  logKV("Programado"    , fmtFromEpoch(stored));
  logKV("Intervalo (min)", intervalMin);
  logKV("Próximo ciclo" , fmtFromEpoch(next));
}

static void printWindCompact() {
  logSection("Viento");
  if (!isnan(WindSpeed_mps)) {
    logKV("Vel. (m/s)", WindSpeed_mps, 2);
    logKV("Dir. (deg)", WindDir_deg);
  } else {
    logKV("Estado", String("SIN DATOS"));
  }
}

static void printPluviometerCompact(bool endOfPeriod=false) {
  uint32_t tips = eeGetU32(EE_ADDR_PULSE_COUNT);
  Rain_mm = tips * 1.6f;
  logSection(endOfPeriod ? "Lluvia (fin de periodo)" : "Lluvia");
  logKV("Tips", tips);
  logKV("Lluvia (mm)", Rain_mm, 2);
  if (last_tip_epoch_rtc) {
    logKV("Último tip [fecha]" , fmtFromEpoch(last_tip_epoch_rtc));
    uint32_t now = rtcEpoch();
    logKV("Hace (s)", (uint32_t)((last_tip_epoch_rtc>0)? (now - last_tip_epoch_rtc) : 0));
  } else {
    logKV("Último tip", String("-"));
  }
}

static inline void resetRainPeriod(bool clearLastTip = true) {
  eePutU32(EE_ADDR_PULSE_COUNT, 0);
  pulseCount = 0;
  if (clearLastTip) last_tip_epoch_rtc = 0;
}

static void printSleepCompact(uint32_t now, uint32_t target, int64_t secs) {
  logSection("Sleep");
  logKV("Ahora" , fmtFromEpoch(now));
  logKV("Target", fmtFromEpoch(target));
  logKV("Segundos", (uint32_t)secs);
}

// ============================== Scheduler ===================================
static void ensureNextEpochReady() {
  eepromBeginIfNeeded();
  int eepInterval = 0; EEPROM.get(EE_ADDR_INTERVAL_MIN, eepInterval);
  if (!isIntervalSane(eepInterval)) { eepInterval = 30; EEPROM.put(EE_ADDR_INTERVAL_MIN, eepInterval); EEPROM.commit(); }
#if DEBUG_INTERVAL_MIN > 0
  eepInterval = DEBUG_INTERVAL_MIN;
#endif
  sleepCount = eepInterval;

  uint32_t now    = rtcEpoch();
  uint32_t stored = eeGetU32(EE_ADDR_NEXT_EPOCH);

#if DEBUG_INTERVAL_MIN > 0
  stored = now + (sleepCount * 60);
#else
  if (!isEpochSane(stored, now)) stored = now + (sleepCount * 60);
  stored = advanceNextToFuture(stored, now, (uint32_t)sleepCount);
#endif

  eePutU32(EE_ADDR_NEXT_EPOCH, stored);
  next_cycle_epoch_rtc = stored;
  printScheduler(now, stored, sleepCount, next_cycle_epoch_rtc);
}

static void rollToNextCycle() {
  eepromBeginIfNeeded();
  int eepInterval = 0; EEPROM.get(EE_ADDR_INTERVAL_MIN, eepInterval);
  if (!isIntervalSane(eepInterval)) eepInterval = 30;
#if DEBUG_INTERVAL_MIN > 0
  eepInterval = DEBUG_INTERVAL_MIN;
#endif
  sleepCount = eepInterval;

  uint32_t now  = rtcEpoch();
#if DEBUG_INTERVAL_MIN > 0
  uint32_t next = now + (uint32_t)sleepCount * 60;
#else
  uint32_t next = advanceNextToFuture(next_cycle_epoch_rtc, now, (uint32_t)sleepCount);
#endif
  eePutU32(EE_ADDR_NEXT_EPOCH, next);
  next_cycle_epoch_rtc = next;

  printScheduler(now, eeGetU32(EE_ADDR_NEXT_EPOCH), sleepCount, next_cycle_epoch_rtc);
}

// ============================== Sleep =======================================
static void sleepUntilEpoch(uint32_t target_epoch) {
  uint32_t now = rtcEpoch();
  int64_t secs = (int64_t)target_epoch - (int64_t)now;
  if (secs < 5) secs = 5;

  printSleepCompact(now, target_epoch, secs);

  esp_sleep_enable_timer_wakeup((uint64_t)secs * 1000000ULL);
  esp_sleep_enable_ext0_wakeup(pulsePin, 0);
  esp_sleep_enable_ext1_wakeup((1ULL << buttonPin), ESP_EXT1_WAKEUP_ALL_LOW);

  // Mantener pull-up RTC en pulsePin durante deep sleep
  rtc_gpio_deinit(pulsePin);
  rtc_gpio_init(pulsePin);
  rtc_gpio_set_direction(pulsePin, RTC_GPIO_MODE_INPUT_ONLY);
  rtc_gpio_pullup_en(pulsePin);
  rtc_gpio_pulldown_dis(pulsePin);

  Serial.printf("[SLEEP] hasta %u (en %lld s)\n", target_epoch, secs);
  Serial.flush();
  esp_deep_sleep_start();
}

// ============================== Buzzer ======================================
static void buzzerSound1() { pinMode(buzzer, OUTPUT); digitalWrite(buzzer, HIGH); delay(50); digitalWrite(buzzer, LOW); delay(50); digitalWrite(buzzer, HIGH); delay(50); digitalWrite(buzzer, LOW); }
static void buzzerSound2() { pinMode(buzzer, OUTPUT); digitalWrite(buzzer, HIGH); delay(150); digitalWrite(buzzer, LOW); delay(150); digitalWrite(buzzer, HIGH); delay(150); digitalWrite(buzzer, LOW); }

// ============================== Antirebote PLUV =============================
static bool waitForReedRelease(uint32_t timeout_ms = 500) {
  uint32_t t0 = millis(); pinMode(pulsePin, INPUT_PULLUP);
  while (millis() - t0 < timeout_ms) { if (digitalRead(pulsePin) == HIGH) return true; delay(5); }
  return false;
}
static const uint32_t PLUV_DEBOUNCE_SEC = 0;
static const uint32_t LOW_STABLE_MS     = 30;

// ============================== Wake handlers ===============================
static void handleWakeupFromPulsePin() {
  if (LOG_LEVEL >= 1) { logSection("Wake"); logKV("Causa", String("Pluviometro")); }

  pinMode(pulsePin, INPUT_PULLUP); delay(200);

  uint32_t t0 = millis(); bool low_stable = true;
  while (millis() - t0 < LOW_STABLE_MS) { if (digitalRead(pulsePin) != LOW) { low_stable = false; break; } delay(2); }

  uint32_t now_epoch = rtcEpoch();
  bool count_this_tip = false;

  if (low_stable) {
    count_this_tip = true;
#if VERBOSE_RAIN
    logKV("Tip", String("LOW estable"));
#endif
  } else {
    if (!last_tip_epoch_rtc || (now_epoch - last_tip_epoch_rtc) >= PLUV_DEBOUNCE_SEC) {
      count_this_tip = true;
#if VERBOSE_RAIN
      logKV("Tip", String("válido por EXT0 (pin volvió a HIGH)"));
#endif
    } else {
#if VERBOSE_RAIN
      logKV("Tip", String("rebote (ignorado) [EXT0]"));
#endif
    }
  }

  if (!count_this_tip) {
    uint32_t tips_period = eeGetU32(EE_ADDR_PULSE_COUNT);
    if (tips_period == 0 && (last_tip_epoch_rtc == 0 || (now_epoch - last_tip_epoch_rtc) <= 1)) {
      count_this_tip = true;
#if VERBOSE_RAIN
      logKV("Tip", String("FORZADO primer tip del periodo"));
#endif
    }
  }

  if (count_this_tip) {
    uint32_t tips = eeGetU32(EE_ADDR_PULSE_COUNT) + 1;
    eePutU32(EE_ADDR_PULSE_COUNT, tips);
    pulseCount = (int)tips;
    Rain_mm = pulseCount * 1.6f;
    last_tip_epoch_rtc = now_epoch;

#if VERBOSE_RAIN
    logKV("Tip #", tips);
#endif

    // Lockout 0.1 s sin EXT0
    lockout_timer_pending = 1;
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT0);
    esp_sleep_enable_timer_wakeup(100000ULL);
    esp_sleep_enable_ext1_wakeup((1ULL << buttonPin), ESP_EXT1_WAKEUP_ALL_LOW);

    Serial.println("[SLEEP] lockout 0.1 s (sin EXT0)");
    Serial.flush();
    esp_deep_sleep_start();
    return;
  }

  // Ignorado: info mínima
#if VERBOSE_RAIN
  {
    uint32_t tips = eeGetU32(EE_ADDR_PULSE_COUNT);
    Rain_mm = (tips * 1.6f);
    pulseDisplay = tips;
    createTimeStamp();
    logSection("Tip ignorado");
    logKV("Ahora [fecha]" , fmtFromEpoch(now_epoch));
    logKV("Tips (periodo)", tips);
    uint32_t remaining = (next_cycle_epoch_rtc > now_epoch) ? (next_cycle_epoch_rtc - now_epoch) : 0;
    logKV("Próx. ciclo", fmtFromEpoch(next_cycle_epoch_rtc));
    logKV("Faltan (s)", remaining);
  }
#endif

  if (!waitForReedRelease(500)) {
    lockout_timer_pending = 1;
    if (LOG_LEVEL >= 2) { logSection("Pluviometer - Lockout"); logKV("Estado", String("Pin LOW, sleep 2 s sin EXT0")); }
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT0);
    esp_sleep_enable_timer_wakeup(2000000ULL);
    esp_sleep_enable_ext1_wakeup((1ULL << buttonPin), ESP_EXT1_WAKEUP_ALL_LOW);
    Serial.println("[SLEEP] lockout 2 s (sin EXT0)");
    Serial.flush();
    esp_deep_sleep_start();
    return;
  } else {
    lockout_timer_pending = 1;
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT0);
    esp_sleep_enable_timer_wakeup(100000ULL);
    esp_sleep_enable_ext1_wakeup((1ULL << buttonPin), ESP_EXT1_WAKEUP_ALL_LOW);
    Serial.println("[SLEEP] mini-lockout 0.1 s (rebote ignorado)");
    Serial.flush();
    esp_deep_sleep_start();
    return;
  }
}

static void handleWakeupFromButtonPin() {
  logSection("Wake"); logKV("Causa", String("Boton AP"));
  accessPointActive = true;
  buzzerSound2();
  setupEPaper();
  displayAccessPoint();
  displaySleep();
  initWebServer_V2();
}

// ============================== Viento ULP ===============================
static void setupWindSensor() {
  powerSupplyON();
  Serial2.begin(38400, SERIAL_8N1, SERIAL2_RXPIN, SERIAL2_TXPIN);
  delay(ULP_WARMUP_MS);
}

static void captureWindOnce() {
  unsigned long t0 = millis();
  while (millis() - t0 < ULP_CAPTURE_MS) { readAnemometer(); delay(2); }
  WindSpeed_mps = windSpeed;
  WindDir_deg   = (int)windDirection;
}

// ============================== Medición ================================
static void doCycleMeasurement() {
  logSection("Cycle"); logKV("Accion", String("Medicion programada"));

  // Lluvia acumulada del periodo (para UI/log previo)
  printPluviometerCompact(false);

  // Viento
  setupWindSensor();
  captureWindOnce();
  powerSupplyOFF();
  printWindCompact();

  // Guardado + UI
  createTimeStamp();
  saveData();

  printPluviometerCompact(true); // fin de periodo
  resetRainPeriod(true);

  setupEPaper();
  updateDisplayRainOnly();

  // ===================== LoRa TX (UART1 / Serial1) =====================
  pinMode(PIN_M0, OUTPUT);
  pinMode(PIN_M1, OUTPUT);
  mode_normal();                      // M0=0, M1=0 (transparente)

  pinMode(powerLoRa, OUTPUT);
  digitalWrite(powerLoRa, HIGH);      // ENCIENDE el LoRa (GPIO26)
  delay(120);                         // warm-up del módulo

  Serial1.end();                      // por si quedó abierto antes
  Serial1.begin(9600, SERIAL_8N1, LORA_RX_PIN, LORA_TX_PIN);

  // Refresca timestamp si quieres lo más reciente
  createTimeStamp();

  String dataPacket;
  dataPacket.reserve(120);
  dataPacket = "$" + deviceID + "_" + timeStamp + ","
             + String(airTemp) + "," + String(airHumidity) + ","
             + String(windSpeed) + "," + String(windDirection) + ","
             + String(internTemp) + "," + String(pressure) + ","
             + String(waterCount) + "," + String(battery);

  Serial1.println(dataPacket);        // IMPORTANTE: con \n
  Serial1.flush();
  Serial.print("LoRa msg: "); Serial.println(dataPacket);

  Serial1.end();                      // cierra UART para ahorrar
  digitalWrite(powerLoRa, LOW);       // APAGA el LoRa (GPIO26)
  // =================== fin LoRa TX (UART1 / Serial1) ===================

  buzzerSound1();

  rollToNextCycle();
  sleepUntilEpoch(next_cycle_epoch_rtc);
}

// ============================== Arduino lifecycle =======================
void setup() {
  Serial.begin(115200);
  delay(20);

  // Wake pins
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(pulsePin,  INPUT_PULLUP);

  // Identidad / periféricos
  getID();
  initPins();
  initRTC();
  initMemory();
  infoFlash();

  // Pines de modo y energía LoRa (arranca apagado)
  pinMode(PIN_M0, OUTPUT);
  pinMode(PIN_M1, OUTPUT);
  pinMode(powerLoRa, OUTPUT);
  mode_normal();                 // M0=0, M1=0
  digitalWrite(powerLoRa, LOW);  // LoRa OFF al inicio

  // NO iniciar aquí la UART del LoRa: el módulo está apagado.
  // Serial1.begin(9600, SERIAL_8N1, LORA_RX_PIN, LORA_TX_PIN);

  // Arranque – banner corto y estado
  logSection("Device");
  logKV("Producto", String("SpectraLog"));
  logKV("FW", String("1.0.0"));
  logKV("RTC", String("DS3231 OK"));
  logKV("FS", String("SPIFFS OK"));

  // Wakes habilitados
  esp_sleep_enable_ext0_wakeup(pulsePin, 0);
  esp_sleep_enable_ext1_wakeup((1ULL << buttonPin), ESP_EXT1_WAKEUP_ALL_LOW);

  // Causa de wake
  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

  if (cause == ESP_SLEEP_WAKEUP_UNDEFINED) {
    ensureNextEpochReady();
  } else if (cause == ESP_SLEEP_WAKEUP_TIMER) {
    if (lockout_timer_pending) {
      lockout_timer_pending = 0;
      pinMode(pulsePin, INPUT_PULLUP); delay(2);
      if (digitalRead(pulsePin) == LOW) {
        lockout_timer_pending = 1;
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT0);
        esp_sleep_enable_timer_wakeup(100000ULL);
        esp_sleep_enable_ext1_wakeup((1ULL << buttonPin), ESP_EXT1_WAKEUP_ALL_LOW);
        Serial.println("[SLEEP] re-lockout 0.1 s (reed aun LOW)");
        Serial.flush();
        esp_deep_sleep_start();
        return;
      }
      sleepUntilEpoch(next_cycle_epoch_rtc);
      return;
    }
  } else if (cause == ESP_SLEEP_WAKEUP_EXT0) {
    if (next_cycle_epoch_rtc == 0) ensureNextEpochReady();
    handleWakeupFromPulsePin();
    return;
  } else if (cause == ESP_SLEEP_WAKEUP_EXT1) {
    handleWakeupFromButtonPin();
    return;
  }

  if (cause == ESP_SLEEP_WAKEUP_UNDEFINED) {
    logSection("Boot");
    logKV("Tipo"  , String("Cold start"));
    logKV("Accion", String("Medir ahora y programar siguiente"));
  } else {
    logSection("Cycle");
    logKV("Accion", String("Medicion programada"));
  }

  doCycleMeasurement();
}

void loop() {
  if (accessPointActive) { runServer_V2(); delay(5); return; }
  readAnemometer();
}
