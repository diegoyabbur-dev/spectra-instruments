#include "variables.h"

// Definición de variables globales

int sleepTimeST = 300;
volatile int conteoPulsos = 0;
unsigned long tiempoAnterior = 0;
float velocidadViento = 0.0;
unsigned long intervaloGuardado = 5;
int valorLectura = 0;
float voltaje = 0.0;
int direccion = 0;
float airTemp = 0.0;
float airHumidity = 0.0;

String timeStamp = "";

String deviceID = "";

// Anemometer
float windSpeed = 0.0;
int windDirection = 0;

// BPM180
float internTemp = 0.0;
float pressure = 0.0;

// Neblinometer
float waterCount = 0.0;
int pulsePluv = 0;


// Battery device
float battery = 0.0;

// Data Packet
String dataPacket = "";

// RTC
int year = 0;
int month = 0;
int day = 0;
int hour = 0;
int minute = 0;
int second = 0;

String timeAP = "";
String dateAP = "";

int Rain_mm = 0;


bool sleepMode = false;

int   sleepCount         = 0;
float WindSpeed_mps      = 0.0f;
int   WindDir_deg        = 0;
bool  accessPointActive  = false;
volatile int pulseCount  = 0;

int pulseDisplay = 0;