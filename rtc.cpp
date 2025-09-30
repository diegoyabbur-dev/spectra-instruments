// rtc.cpp
#include "rtc.h"
#include "libraries.h"

RTC_DS3231 rtc;

void initRTC() {
    if (!rtc.begin()) {
        Serial.println("[RTC Error]");
        Serial.println("---------------------------------------------");
        Serial.println("RTC DS3231 could not be found.");
        Serial.println();
        setRTCTimeManually();

    } else {
        Serial.println("[RTC Info.]");
        Serial.println("---------------------------------------------");
        Serial.println("RTC DS3231 initialized.");
        Serial.println();
    }
}

void createTimeStamp() { 
    DateTime now = rtc.now();  // Obtiene la fecha y hora actual

    timeStamp = String(now.year()) + "/" +
                (now.month() < 10 ? "0" : "") + String(now.month()) + "/" +
                (now.day() < 10 ? "0" : "") + String(now.day()) + "," +
                (now.hour() < 10 ? "0" : "") + String(now.hour()) + ":" +
                (now.minute() < 10 ? "0" : "") + String(now.minute()) + ":" +
                (now.second() < 10 ? "0" : "") + String(now.second());

    hour = now.hour();
    minute = now.minute();
    year = now.year();
    month = now.month();
    day = now.day();

    // Hora tipo: 14:05:02
    timeAP = 
      (hour < 10 ? "0" : "") + String(hour) + ":" +
      (minute < 10 ? "0" : "") + String(minute);

    // Fecha tipo: 2025-03-25
    dateAP = 
      String(year) + "-" +
      (month < 10 ? "0" : "") + String(month) + "-" +
      (day < 10 ? "0" : "") + String(day);
    

}

void setRTCTimeManually() {
    // Configura el RTC con una fecha y hora exactas
    rtc.adjust(DateTime(2024, 9, 11, 11, 1, 56)); // Año, mes, día, hora, minuto, segundo
}

void readRTCTime() {
    // Obtiene la fecha y hora actual del RTC
    DateTime now = rtc.now();

    // Muestra la fecha y hora en formato legible
    /*Serial.print("Fecha: ");
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print("  Hora: ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();*/
}

void rtcAdjust() {
    rtc.adjust(DateTime(year, month, day, hour, minute, second));
}