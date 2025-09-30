#include "pins.h"
#include "libraries.h"
#include "driver/rtc_io.h"

const int powerSupply = GPIO_NUM_14;  // Fuente de alimentación

const int SERIAL2_RXPIN = 13; // Pin RX para Serial2
const int SERIAL2_TXPIN = 15; // Pin TX para Serial2

const int buzzer = 12; // Pin para el buzzer

// Lora Pins
//const int LoRa_M0 = 25;
//const int LoRa_M1 = 26;
const gpio_num_t LoRa_M0 = GPIO_NUM_25;
const gpio_num_t LoRa_M1 = GPIO_NUM_27;
const int powerLoRa = 26;


// Modem Pin
const int powerModem = 19;
const int modemKey = 25;

// const int despertadorExtern = 33; SE OCUPARA PARA EL TIP DEL CHAGUAL
// const int RS485 = 32; SE OCUPA DE DE-RE EN MAX485

const int voltagePin = 36;

void initPins() {
    pinMode(powerSupply, OUTPUT);
    //pinMode(LoRa_M0, OUTPUT);
    //pinMode(LoRa_M1, OUTPUT);
    pinMode(powerModem, OUTPUT);
    pinMode(buzzer, OUTPUT);
    //pinMode(powerLoRa, OUTPUT);

    pinMode(voltagePin, INPUT);

    digitalWrite(powerSupply, LOW);
    digitalWrite(buzzer, LOW);
    //digitalWrite(LoRa_M0, LOW);
    //digitalWrite(LoRa_M1, LOW);
    digitalWrite(powerModem, LOW);
    //digitalWrite(powerLoRa, LOW);
}

void readVoltage() {
    const int numReadings = 10; 
    static int readings[numReadings];
    static int readIndex = 0; 
    static float total = 0; 
    static float average = 0; 

    // Resta la lectura más antigua del total
    total -= readings[readIndex];

    // Lee el nuevo valor analógico
    readings[readIndex] = analogRead(voltagePin);
    
    // Suma la nueva lectura al total
    total += readings[readIndex];
    
    // Avanza al siguiente índice, y reinicia si es necesario
    readIndex = (readIndex + 1) % numReadings;
    
    // Calcula el promedio
    average = total / numReadings;
    
    // Calcula el voltaje de la batería
    float voltage = (average / 4095.0) * 3.3 * 5; // Factor de 5 por el divisor

    battery = voltage * 2.19; // Almacena el voltaje calculado
}


// Power Supply control
void powerSupplyON() {
    gpio_hold_dis(GPIO_NUM_14);
    digitalWrite(powerSupply, HIGH);
}

void powerSupplyOFF() {
    digitalWrite(powerSupply, LOW);

    gpio_hold_en(GPIO_NUM_14); // Habilitar retención para el pin 14
    gpio_deep_sleep_hold_en(); // Habilitar retención general durante Deep Sleep
}

void powerLoRaON() {
    digitalWrite(powerLoRa, HIGH);
}

void powerLoRaOFF() {
    digitalWrite(powerLoRa, LOW);
}

void LoRaTransmitMode() {
    digitalWrite(LoRa_M0, LOW);
    digitalWrite(LoRa_M1, LOW);
}

void powerModemON() {
    digitalWrite(powerModem, HIGH);
}

void powerModemOFF() {
    digitalWrite(powerModem, LOW);
}

