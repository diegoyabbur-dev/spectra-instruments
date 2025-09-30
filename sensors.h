// sensors.h
#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>

void initSHT31();
void handleSensors();
void readDirection();
void readSHT31();

// Ultrasonic Anemometer
void processReceivedMessage();
void readAnemometer();

// Pressure Sensor
void initBMP180();
void readBMP180();

#endif
