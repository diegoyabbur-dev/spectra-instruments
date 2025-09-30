// sensors.cpp
#include "libraries.h"

Adafruit_SHT31 sht31 = Adafruit_SHT31();

int ADDRESS_BMP180 = 0x77;

#define I2C_ADDRESS_BMP180 ADDRESS_BMP180

BMP180I2C bmp180(I2C_ADDRESS_BMP180);

void initSHT31() {
    sht31.begin(0x44);
}

void handleSensors() {
  // Lógica para manejar sensores
  unsigned long currentMillis = millis();
  static unsigned long previousMillis = 0;
  const unsigned long captureInterval = 12 * 1000; // Intervalo de captura en milisegundos (12s) | Despues reemplazar por algun int

  readAnemometer();
  
  if (currentMillis - previousMillis >= captureInterval) {
    previousMillis = currentMillis;

    //createTimeStamp();
    //readVoltage();
    //readSHT31();

    //sleepMode = true;

  }
}

/*void readDirection() {
    valorLectura = analogRead(pinAnemometro);
    voltaje = (valorLectura / 4095.0) * 3.3;
    direccion = map(voltaje * 1000, 0, 3300, 10, 360);
    Serial.print("Dirección del viento: ");
    Serial.println(direccion);
}*/

void readSHT31() {
    airTemp = sht31.readTemperature();
    airHumidity = sht31.readHumidity();
}

const char MESSAGE_START = '$';
const char MESSAGE_END = '*';
char messageBuffer[100];
int messageIndex = 0;
bool isReceivingMessage = false;

void processReceivedMessage() {
  // Split the message using comma as delimiter
  char *token = strtok(messageBuffer, ",");
  if (token != nullptr && strcmp(token, "IIMWV") == 0) {
    // Parse direction, speed, and reference
    token = strtok(nullptr, ",");
    if (token != nullptr) {
      windDirection = atoi(token);
      token = strtok(nullptr, ",");
      if (token != nullptr && strcmp(token, "R") == 0) {
        token = strtok(nullptr, ",");
        if (token != nullptr) {
          windSpeed = atof(token);
          token = strtok(nullptr, ",");
          if (token != nullptr && strcmp(token, "M") == 0) {
            token = strtok(nullptr, ",");
            if (token != nullptr) {
              // At this point, token should be the warning indicator
              char warning = token[0];
              
              // Now you have direction, speed, reference, and warning
              
              
              /*Serial.println("---------------------");
              Serial.print("Wind Direction: ");
              Serial.println(windDirection);
              Serial.print("Wind Speed: ");
              Serial.println(windSpeed);*/
              

            }
          }
        }
      }
    }
  }
}

void readAnemometer() {
  while (Serial2.available()) {
    char receivedChar = Serial2.read();

    if (receivedChar == MESSAGE_START) {
      isReceivingMessage = true;
      messageIndex = 0;
    } else if (isReceivingMessage && receivedChar == MESSAGE_END) {
      isReceivingMessage = false;
      messageBuffer[messageIndex] = '\0'; // Null-terminate the message
      processReceivedMessage();
    } else if (isReceivingMessage && messageIndex < sizeof(messageBuffer) - 1) {
      // Store the character in the message buffer if receiving
      messageBuffer[messageIndex++] = receivedChar;
    }
  }
}

// Función para leer datos del sensor BMP180
void readBMP180() {
  // Medir temperatura
  if (!bmp180.measureTemperature()) {
    Serial.println("Could not start BMP180 temperature measurement.");
    return;
  }

  // Esperar hasta que se complete la medición de temperatura
  while (!bmp180.hasValue()) {
    delay(1); // Puedes considerar una estrategia de espera no bloqueante
  }

  // Obtener la temperatura medida
  internTemp = bmp180.getTemperature();

  // Medir presión
  if (!bmp180.measurePressure()) {
    Serial.println("Could not start BMP180 pressure measurement.");
    return;
  }

  // Esperar hasta que se complete la medición de presión
  while (!bmp180.hasValue()) {
    delay(1); // Puedes considerar una estrategia de espera no bloqueante
  }

  // Obtener la presión medida y convertirla a hPa
  float pressure_raw = bmp180.getPressure();
  if (pressure_raw == 0) {
    Serial.println("Error: BMP180 pressure measurement returned zero.");
    return;
  } 
  pressure = pressure_raw / 100.0;
}

// Función para configurar el sensor BMP180
void initBMP180() {
  // Inicializar el sensor BMP180
  if (!bmp180.begin()) {
    Serial.println("Could not initialize BMP180 sensor.");
    return;
  }
  
  // Restablecer el sensor a los valores predeterminados
  bmp180.resetToDefaults();

  // Establecer el modo de muestreo
  bmp180.setSamplingMode(BMP180MI::MODE_UHR);
}