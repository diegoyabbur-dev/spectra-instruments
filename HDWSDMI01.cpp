#include "HDWSDMI01.h"

static HardwareSerial *gUart = nullptr;

static uint16_t crc16_modbus(const uint8_t *buf, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; ++i) {
    crc ^= buf[i];
    for (uint8_t b = 0; b < 8; ++b) {
      if (crc & 0x0001) crc = (crc >> 1) ^ 0xA001;
      else              crc = (crc >> 1);
    }
  }
  return crc;
}

void hdwsd_begin() {
  static HardwareSerial uart(HDWSD_UART_NUM);
  gUart = &uart;
  gUart->begin(HDWSD_BAUD, SERIAL_8N1, HDWSD_RX_PIN, HDWSD_TX_PIN);
}

// Envía petición Modbus RTU (func 0x03) y lee 2 registros
bool hdwsd_read(float &speed_mps, float &dir_deg,
                uint16_t *rawSpeed, uint16_t *rawDir, uint8_t *errCode)
{
  if (!gUart) { if (errCode) *errCode = 1; return false; }

  const uint8_t addr = (uint8_t)HDWSD_SLAVE_ID;
  const uint16_t startReg = HDWSD_REG_SPEED;
  const uint16_t qty = 2;

  // Petición: [addr][0x03][startHi][startLo][qtyHi][qtyLo][crcLo][crcHi]
  uint8_t req[8];
  req[0] = addr;
  req[1] = 0x03;
  req[2] = (uint8_t)(startReg >> 8);
  req[3] = (uint8_t)(startReg & 0xFF);
  req[4] = (uint8_t)(qty >> 8);
  req[5] = (uint8_t)(qty & 0xFF);
  uint16_t crc = crc16_modbus(req, 6);
  req[6] = (uint8_t)(crc & 0xFF);
  req[7] = (uint8_t)(crc >> 8);

  // Flush RX antes de transmitir
  while (gUart->available()) (void)gUart->read();

  // Enviar
  gUart->write(req, sizeof(req));
  gUart->flush();
  delay(HDWSD_TX_GAP_MS); // pequeño gap

  // Respuesta esperada: [addr][0x03][bytecount=4][D0][D1][D2][D3][crcLo][crcHi]
  const uint8_t expectedLen = 9; // 3 + 4 + 2
  uint8_t resp[16];
  size_t got = 0;
  const uint32_t t0 = millis();

  while ((millis() - t0) < HDWSD_RX_TIMEOUT_MS) {
    while (gUart->available() && got < sizeof(resp)) {
      resp[got++] = (uint8_t)gUart->read();
      if (got >= expectedLen) break;
    }
    if (got >= expectedLen) break;
    delay(1);
  }

  if (got < expectedLen) {
    if (errCode) *errCode = 1; // timeout
    return false;
  }

  // Validaciones básicas
  if (resp[0] != addr || resp[1] != 0x03 || resp[2] != 4) {
    if (errCode) *errCode = 3; // trama inesperada
    return false;
  }

  // CRC
  uint16_t rxCrc = (uint16_t)resp[expectedLen - 2] | ((uint16_t)resp[expectedLen - 1] << 8);
  uint16_t calc = crc16_modbus(resp, expectedLen - 2);
  if (rxCrc != calc) {
    if (errCode) *errCode = 2; // CRC malo
    return false;
  }

  // Datos
  uint16_t rs = ((uint16_t)resp[3] << 8) | resp[4];
  uint16_t rd = ((uint16_t)resp[5] << 8) | resp[6];

  if (rawSpeed) *rawSpeed = rs;
  if (rawDir)   *rawDir   = rd;

  speed_mps = rs * 0.01f;
  dir_deg   = rd * 0.1f;
  if (errCode) *errCode = 0;
  return true;
}
