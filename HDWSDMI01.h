#pragma once
#include <Arduino.h>

// UART / RS485 config (puedes override con -D en platformio.ini o antes del include)
#ifndef HDWSD_UART_NUM
  #define HDWSD_UART_NUM 1
#endif
#ifndef HDWSD_RX_PIN
  #define HDWSD_RX_PIN 15
#endif
#ifndef HDWSD_TX_PIN
  #define HDWSD_TX_PIN 13
#endif
#ifndef HDWSD_BAUD
  #define HDWSD_BAUD 9600
#endif
#ifndef HDWSD_SLAVE_ID
  #define HDWSD_SLAVE_ID 255  // HD-WSD-MI-01 default
#endif

// Register map (HD-WSD-MI-01)
#ifndef HDWSD_REG_SPEED
  #define HDWSD_REG_SPEED 0x000C  // uint16, scale x0.01 m/s
#endif
#ifndef HDWSD_REG_DIR
  #define HDWSD_REG_DIR   0x000D  // uint16, scale x0.1 deg
#endif

// Timeouts (ms)
#ifndef HDWSD_TX_GAP_MS
  #define HDWSD_TX_GAP_MS 2
#endif
#ifndef HDWSD_RX_TIMEOUT_MS
  #define HDWSD_RX_TIMEOUT_MS 150
#endif

// Init RS485/Serial
void hdwsd_begin();

// Read sensor. Returns true on success.
// On success: speed_mps, dir_deg filled. raw* and errCode are optional outs.
// errCode (si falla): 1=timeout, 2=bad_crc, 3=bad_addr/func/len
bool hdwsd_read(float &speed_mps, float &dir_deg,
                uint16_t *rawSpeed = nullptr,
                uint16_t *rawDir   = nullptr,
                uint8_t  *errCode  = nullptr);
