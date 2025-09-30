#ifndef VARIABLES_H
#define VARIABLES_H

#include <Arduino.h>

// Variables
extern int sleepTimeST;
extern volatile int conteoPulsos;
extern unsigned long tiempoAnterior;
extern float velocidadViento;
extern unsigned long intervaloGuardado;
extern int valorLectura;
extern float voltaje;
extern int direccion;
extern float airTemp;
extern float airHumidity;

extern String timeStamp;

extern String deviceID;

// Anemometer
extern float windSpeed;
extern int windDirection;

// BPM180
extern float internTemp;
extern float pressure;

// Neblinometer
extern float waterCount;
extern int pulsePluv;


// Battery device
extern float battery;

// Data Packet
extern String dataPacket;

// Sleep mode control
extern bool sleepMode;

// RTC
extern int year;
extern int month;
extern int day;
extern int hour;
extern int minute;
extern int second;
extern String timeAP;
extern String dateAP;

extern int Rain_mm;

extern int   sleepCount;         // minutos de intervalo
extern float WindSpeed_mps;      // m/s
extern int   WindDir_deg;        // grados
extern bool  accessPointActive;  // bandera para correr el AP en loop()
extern volatile int pulseCount;  // contador de tips (si se toca en ISR => volatile)

extern int pulseDisplay;

#endif
