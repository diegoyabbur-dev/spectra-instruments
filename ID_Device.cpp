// src/ID_Device.cpp

#include "libraries.h"

void getID() {

  uint8_t baseMac[6]; // Para almacenar la dirección MAC
  // Obtiene la dirección MAC del ESP
  esp_read_mac(baseMac, ESP_MAC_WIFI_STA);

  // Convierte la dirección MAC a String para su manipulación, sin incluir los dos puntos
  char baseMacChr[13] = {0}; // 12 caracteres + null terminator
  sprintf(baseMacChr, "%02X%02X%02X%02X%02X%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);

  String mac = String(baseMacChr);

  // Extrae los últimos 4 caracteres de la dirección MAC
  String lastFourChars = mac.substring(mac.length() - 4);

  // Añade la palabra "MB" al inicio por MakeBee
  deviceID = "" + lastFourChars;

  Serial.println();
  Serial.println("[Device identification]");
  Serial.println("---------------------------------------------");
  Serial.print("Product Name     : ");
  Serial.println("AtmosSense Essential");

  Serial.print("Firmware Version : ");
  Serial.println("1.0.0");

  Serial.print("MAC Address      : ");
  Serial.println(mac);

  Serial.print("ID               : ");
  Serial.println(deviceID);
  Serial.println();
}