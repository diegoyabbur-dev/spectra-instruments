#ifndef PINS_H
#define PINS_H

#include <Arduino.h>

// Definiciones de pines
extern const gpio_num_t buttonPin;
extern const int pinVelocidadViento;
extern const int pinAnemometro;
extern const int powerSupply;

extern const int SERIAL2_RXPIN; // Pin RX para Serial2
extern const int SERIAL2_TXPIN; // Pin TX para Serial2

extern const int buzzer;

extern const int powerModem;
extern const int modemKey;

// Lora Pins
extern const gpio_num_t LoRa_M0;
extern const gpio_num_t LoRa_M1;

extern const int voltagePin;

// Setup Pins
void initPins();

void readVoltage();

// Powers Supply
void powerSupplyON();
void powerSupplyOFF();
void powerLoRaON();
void powerLoRaOFF();
void powerModemON();
void powerModemOFF();
void LoRaTransmitMode();

#endif
