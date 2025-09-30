#include "ulp_ultrasonic.h"
#include "libraries.h"   // para tus globals WindSpeed_mps / WindDir_deg, etc.

// === Globals "oficiales" que usa tu web/CSV ===
extern float WindSpeed_mps;
extern int   WindDir_deg;

// --- Parser state ---
static const char MESSAGE_START = '$';
static const char MESSAGE_END   = '*';
static char  msgBuf[128];
static int   msgIdx = 0;
static bool  rxIn   = false;

// --- helpers ---
static inline void uart2_flush_input() {
  while (Serial2.available()) (void)Serial2.read();
}

static float to_mps(float v, char unit) {
  switch (unit) {
    case 'M': return v;                        // m/s
    case 'N': return v * 0.514444f;            // kn → m/s
    case 'K': return v * (1000.0f / 3600.0f);  // km/h → m/s
    default:  return NAN;
  }
}

// Procesa una línea NMEA "MWV" SIN el '$' ni el '*'
static void processMWV(char* line) {
  // line ej: "WIMWV,045,R,10.4,N,A"
  // Aceptar cualquier "??MWV"
  char* talker_cmd = strtok(line, ",");
  if (!talker_cmd) return;

  size_t L = strlen(talker_cmd);
  if (L < 3 || strcmp(talker_cmd + (L - 3), "MWV") != 0) return;

  // dir
  char* tok = strtok(nullptr, ","); if (!tok) return;
  int dirDeg = atoi(tok);

  // ref (R/T) - no usado
  tok = strtok(nullptr, ","); if (!tok) return;
  // char ref = tok[0];

  // speed
  tok = strtok(nullptr, ","); if (!tok) return;
  float spd = atof(tok);

  // unit
  tok = strtok(nullptr, ","); if (!tok) return;
  char unit = tok[0];

  // status
  tok = strtok(nullptr, ","); if (!tok) return;
  char status = tok[0];
  if (status != 'A') return;  // válida solamente

  float mps = to_mps(spd, unit);
  if (isnan(mps)) return;

  // Publica a las variables globales del sistema
  WindDir_deg   = dirDeg;
  WindSpeed_mps = mps;

  // Debug útil (opcional)
  // Serial.printf("[ULP] MWV ok: dir=%d deg, spd=%.2f m/s (unit %c)\n", dirDeg, mps, unit);
}

static bool pumpUART_until(uint32_t deadline_ms) {
  bool gotValid = false;

  while (millis() < deadline_ms) {
    while (Serial2.available()) {
      char c = (char)Serial2.read();

      if (c == MESSAGE_START) {
        rxIn = true; msgIdx = 0;
      } else if (rxIn && c == MESSAGE_END) {
        // terminar línea actual y procesar
        msgBuf[msgIdx] = '\0';
        rxIn = false;

        // Clon temporal porque strtok modifica
        char line[128];
        strncpy(line, msgBuf, sizeof(line));
        line[sizeof(line)-1] = '\0';
        processMWV(line);
        gotValid = true;
      } else if (rxIn) {
        if (msgIdx < (int)sizeof(msgBuf) - 1) {
          msgBuf[msgIdx++] = c;
        } else {
          // overflow → reinicia
          rxIn = false;
          msgIdx = 0;
        }
      }
    }
    // ceder CPU un pelo
    delay(2);
  }
  return gotValid;
}

// === API ===

void ULP_Wind_init(uint32_t baud) {
  Serial2.begin(baud, SERIAL_8N1, SERIAL2_RXPIN, SERIAL2_TXPIN);
  uart2_flush_input();
}

void ULP_Wind_warmup(uint32_t ms) {
  uart2_flush_input();   // limpia basura previa
  uint32_t t0 = millis();
  while (millis() - t0 < ms) {
    // opcional: podrías leer y descartar para medir ritmo de tramas
    while (Serial2.available()) (void)Serial2.read();
    delay(5);
  }
  uart2_flush_input();   // limpia justo antes de capturar
}

bool ULP_Wind_capture(uint32_t window_ms, float* out_mps, int* out_deg) {
  uint32_t deadline = millis() + window_ms;
  bool ok = pumpUART_until(deadline);

  if (ok) {
    if (out_mps) *out_mps = WindSpeed_mps;
    if (out_deg) *out_deg = WindDir_deg;
  }
  return ok;
}
