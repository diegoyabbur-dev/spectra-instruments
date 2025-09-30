// memory.cpp
#include "memory.h"
#include "libraries.h"
#include "variables.h"
#include "rtc.h"

// Factor de conversión: mm de lluvia por tip
// AJUSTA ESTE VALOR a tu calibración real del pluviómetro
static constexpr float MM_PER_TIP = 1.6f;

static void formatDateTime(char* dateBuf, size_t dateLen,
                           char* timeBuf, size_t timeLen)
{
  // Asumimos que createTimeStamp() refresca year, month, day, hour, minute, second (globals de rtc.h)
  createTimeStamp();

  // YYYY-MM-DD
  snprintf(dateBuf, dateLen, "%04d-%02d-%02d", year, month, day);
  // HH:MM:SS
  snprintf(timeBuf, timeLen, "%02d:%02d:%02d", hour, minute, second);
}

static void ensureCsvHeader()
{
  if (!SPIFFS.exists("/data.csv")) {
    File f = SPIFFS.open("/data.csv", FILE_WRITE);
    if (f) {
      f.println("Date,Time,WindSpeed_mps,WindDir_deg,Rain_mm");
      f.close();
    } else {
      Serial.println("[Flash Memory Error]");
      Serial.println("---------------------------------------------");
      Serial.println("No se pudo crear /data.csv para encabezado.");
      Serial.println();
    }
  }
}

void initMemory() {
  if (!SPIFFS.begin(true)) {
    Serial.println("[Flash Memory Error]");
    Serial.println("---------------------------------------------");
    Serial.println("Error mounting the file system.");
    Serial.println();
    delay(10);
  } else {
    Serial.println("[Flash Memory Info]");
    Serial.println("---------------------------------------------");
    Serial.println("SPIFFS correctly mounted.");
    Serial.println();
    delay(10);
  }
}

void saveData() {
  ensureCsvHeader();

  // 1) Fecha/Hora legible (dos columnas)
  char dateBuf[11]; // YYYY-MM-DD
  char timeBuf[9];  // HH:MM:SS
  formatDateTime(dateBuf, sizeof(dateBuf), timeBuf, sizeof(timeBuf));

  // 2) Lluvia del período (mm) desde tips actuales
  //    pulseCount debe contener los tips acumulados del período vigente
  //float rain_mm = static_cast<float>(pulseCount) * MM_PER_TIP;


  // 4) Guardar línea CSV alineada con el encabezado
  File file = SPIFFS.open("/data.csv", FILE_APPEND);
  if (!file) {
    Serial.println("[Flash Memory Error]");
    Serial.println("---------------------------------------------");
    Serial.println("No se pudo abrir /data.csv en modo append.");
    Serial.println();
    return;
  }

  // Formato: Date,Time,WindSpeed_mps,WindDir_deg,Rain_mm
  file.printf("%s,%s,%.2f,%d,%.2f\n",
              dateBuf,
              timeBuf,
              WindSpeed_mps,
              WindDir_deg,
              Rain_mm);
  file.close();
}

void infoFlash() {
  size_t totalBytes = SPIFFS.totalBytes();
  size_t usedBytes  = SPIFFS.usedBytes();
  size_t freeBytes  = totalBytes - usedBytes;

  float totalMB = totalBytes / 1024.0f / 1024.0f;
  float usedMB  = usedBytes  / 1024.0f / 1024.0f;
  float freeMB  = freeBytes  / 1024.0f / 1024.0f;

  float usedPercentage = (usedBytes * 100.0f) / (float)totalBytes;
  float freePercentage = 100.0f - usedPercentage;

  Serial.println("[Flash Memory Info]");
  Serial.println("---------------------------------------------");
  Serial.printf("Total memory: %.2f MB\n", totalMB);
  Serial.printf("Used memory: %.2f MB (%.2f%%)\n", usedMB, usedPercentage);
  Serial.printf("Free memory: %.2f MB (%.2f%%)\n", freeMB, freePercentage);
  Serial.println("---------------------------------------------");
  delay(10);
}
