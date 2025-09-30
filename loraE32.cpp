// src/Lora_E32.cpp

#include "libraries.h"


String dataBuffer = "";

HardwareSerial LoRaPort(1);   // UART1 para LoRa

void readLoRa() {
  if (Serial2.available()) {
    Serial.write(Serial2.read());
  }
  if (Serial.available()) {
    Serial2.write(Serial.read());
  }
}

void sendLoRaData() {
    powerLoRaON();
    LoRaTransmitMode();
    LoRaPort.begin(9600, SERIAL_8N1, 19, 32);
    delay(100);
    dataPacket.reserve(100);  // Ajusta según el tamaño esperado
    dataPacket = "$" + deviceID + "_" + timeStamp + "," + airTemp + "," + airHumidity + "," + windSpeed + "," + windDirection
               + "," + internTemp + "," + pressure + "," + waterCount + "," + battery;

    LoRaPort.println(dataPacket);
    Serial.println(dataPacket);
    delay(100);
    
    powerLoRaOFF();
}

/*void processDataLoRa() {
  while (Serial2.available()) {
    char incomingChar = Serial2.read();
    dataBuffer += incomingChar;

    // Mostrar cada carácter recibido para depuración
    //Serial.print(incomingChar);

    if (incomingChar == '\n') {
      // Mostrar el dato recibido completo para depuración
      Serial.println("Dato completo recibido: " + dataBuffer);

      // Procesar y almacenar los datos
      int idEnd = dataBuffer.indexOf('_');
      if (idEnd != -1) {
        // Almacenar el ID del dispositivo
        LoRaID_Red = dataBuffer.substring(1, idEnd);

        // Eliminar el ID de la cadena de datos
        dataBuffer = dataBuffer.substring(idEnd + 1);

        // Separar los valores restantes
        std::vector<String> values;
        int pos = 0;
        String token;
        while ((pos = dataBuffer.indexOf(',')) != -1) {
          token = dataBuffer.substring(0, pos);
          values.push_back(token);
          dataBuffer = dataBuffer.substring(pos + 1);
        }
        // El último valor (batería) está seguido de '\n'
        values.push_back(dataBuffer.substring(0, dataBuffer.indexOf('/')));

        // Verificar el tamaño de los valores recibidos
        Serial.print("Tamaño de valores recibidos: ");
        Serial.println(values.size());
        Serial.println();

        // Almacenar los valores en las variables correspondientes
        if (values.size() == 8) {
          airTempLoRa = values[0].toFloat();
          airHumidityLoRa = values[1].toFloat();
          windSpeedLoRa = values[2].toFloat();
          windDirectionLoRa = values[3].toInt();
          internTempLoRa = values[4].toFloat();
          pressureLoRa = values[5].toFloat();
          waterCountLoRa = values[6].toFloat();
          batteryLoRa = values[7].toFloat();

          // Mostrar los valores en el monitor serial
          Serial.println("ID del dispositivo   : " + LoRaID_Red);
          Serial.println("Temperatura del aire : " + String(airTempLoRa, 2) + " °C");
          Serial.println("Humedad del aire     : " + String(airHumidityLoRa, 2) + " %");
          Serial.println("Velocidad del viento : " + String(windSpeedLoRa, 2) + " m/s");
          Serial.println("Dirección del viento : " + String(windDirectionLoRa) + "°");
          Serial.println("Temperatura interna  : " + String(internTempLoRa, 2) + " °C");
          Serial.println("Presión              : " + String(pressureLoRa, 2) + " hPa");
          Serial.println("Conteo de agua       : " + String(waterCountLoRa, 2) + " ml");
          Serial.println("Batería              : " + String(batteryLoRa, 2) + " V");
          Serial.println();
        } else {
          Serial.println("Error: número incorrecto de valores recibidos");
        }
      } else {
        Serial.println("Error: no se encontró el delimitador '_'");
      }

      // Limpiar el búfer después de procesar
      dataBuffer = "";
    }
  }
}
*/
