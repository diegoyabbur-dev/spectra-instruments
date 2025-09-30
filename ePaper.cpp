// src/ePaper.cpp

#include "libraries.h"

#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>

GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT> display(GxEPD2_290_BS(/*CS=5*/ 5, /*DC=*/ 19, /*RES=*/ 4, /*BUSY=*/ 34)); // DEPG0290BS 128x296, SSD1680
//CS(SS)= 5, SCL(SCK)= 18, SDA(MOSI)= 23, BUSY= 34, RES(RST)= 4,DC= 19

#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>

void setupEPaper() {
    display.init(115200, true, 50, false);
}

void displayIntroScreen() {

  const char HelloWorld[] = "MistMetric";
  const char HelloWeACtStudio[] = "Fondecyt Project";

  display.setRotation(1);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  int16_t tbx, tby; uint16_t tbw, tbh;
  display.getTextBounds(HelloWorld, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center the bounding box by transposition of the origin:
  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  uint16_t y = ((display.height() - tbh) / 2) - tby;
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(x, y-tbh);
    display.print(HelloWorld);
    display.setTextColor(display.epd2.hasColor ? GxEPD_RED : GxEPD_BLACK);
    display.getTextBounds(HelloWeACtStudio, 0, 0, &tbx, &tby, &tbw, &tbh);
    x = ((display.width() - tbw) / 2) - tbx;
    display.setCursor(x, y+tbh);
    display.print(HelloWeACtStudio);
  }
  while (display.nextPage());
  
}

void updateDisplayAllData() {
   do
  {
    display.setRotation(1); // Set rotation if needed
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);

    
    display.drawBitmap(280, 3, battery_medium, 16, 14, GxEPD_BLACK); 

    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(5, 16); // Adjust text position
    display.print(day);
    display.print("/");
    display.print(month);
    display.print("/");
    display.println(year);
    
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(125, 16); // Adjust text position

    display.print(hour);
    display.print(":");
    display.println(minute);

    display.drawBitmap(0, 23, line_horizontal, 296, 2, GxEPD_BLACK);

    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(5, 43); // Adjust text position
    display.print("ID: ");
    display.println(deviceID);

    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(5, 63); // Adjust text position
    display.print("Air Temp: ");
    display.print(airTemp);
    display.println("C");

    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(5, 83); // Adjust text position
    display.print("Air Humidity: ");
    display.print(airHumidity);
    display.println("RH");

    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(5, 103); // Adjust text position
    display.print("Wind Speed: ");
    display.print(windSpeed);
    display.println("m/s");

    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(5, 123); // Adjust text position
    display.print("Wind Direction: ");
    display.print(windDirection);
    display.println("°");

    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(5, 143); // Adjust text position
    display.print("Collected Water: ");
    display.print(waterCount);
    display.println("mL");

    display.nextPage();
  } while (display.nextPage());

}

void updateDisplayRainOnly() {
   do
  {
    display.setRotation(2); // Set rotation if needed
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);

    

    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(20, 16); // Adjust text position
    display.println("SpectraLog");

    display.drawBitmap(0, 23, line_horizontal, 296, 2, GxEPD_BLACK);

    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(5, 43); // Adjust text position
    display.print("ID  : ");
    display.println(deviceID);

    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(5, 63); // Ajusta la posición
    display.print("Date: ");
    display.print(day);
    display.print("/");
    display.print(month);
    display.print("/");
    display.println(year % 100);  // <-- Solo los últimos 2 dígitos


    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(5, 83); // Adjust text position
    display.print("Hour: ");
    display.print(hour);
    display.print(":");
    display.println(minute);

    display.drawBitmap(0, 93, line_horizontal, 296, 2, GxEPD_BLACK);

    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(20, 113); // Adjust text position
    display.println("Anemometer");

    display.drawBitmap(0, 120, line_horizontal, 296, 2, GxEPD_BLACK);

    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(5, 140); // Adjust text position
    display.print("Spd : ");
    display.print(windSpeed);
    display.println("");

    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(5, 160); // Adjust text position
    display.print("Dir : ");
    display.print(windDirection);
    display.println("");

    display.drawBitmap(0, 170, line_horizontal, 296, 2, GxEPD_BLACK);

    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(20, 190); // Adjust text position
    display.println(" LWC Meter");

    display.drawBitmap(0, 197, line_horizontal, 296, 2, GxEPD_BLACK);

    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(5, 217); // Adjust text position
    display.print("FogC: ");
    display.print(Rain_mm);
    display.println("mL");

    display.drawBitmap(0, 227, line_horizontal, 296, 2, GxEPD_BLACK);

    display.drawBitmap(58, 240, spectra_instruments_logo, 38, 46, 2,GxEPD_BLACK);
    display.setTextColor(GxEPD_BLACK);

    display.nextPage();
  } while (display.nextPage());

}

void updateDisplayRainOnly1() {
   do
  {
    display.setRotation(2); // Set rotation if needed
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
  
    display.drawBitmap(0, 0, spectra_instruments_logo, 296, 152, 1); // Las dimensiones de la pantalla son 296x152 Pixeles. En caso de tener otras dimensiones, modificar estos valores.

    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(5, 16); // Adjust text position
    display.println("SpectraLog");

    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(125, 16); // Adjust text position
    display.print(day);
    display.print("/");
    display.print(month);
    display.print("/");
    display.print(year);
    display.print("|");
    display.print(hour);
    display.print(":");
    display.println(minute);

    display.drawBitmap(0, 23, line_horizontal, 296, 2, GxEPD_BLACK);

    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(5, 43); // Adjust text position
    display.print("ID device: ");
    display.println(deviceID);

    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(5, 63); // Adjust text position
    display.print("Pulse/hour: ");
    display.print(Rain_mm);
    display.println("");

    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(5, 83); // Adjust text position
    display.print("L/hour: ");
    display.print((Rain_mm*3.3)/1000);
    display.println("mL");

    display.nextPage();
  } while (display.nextPage());

}

void displayAccessPoint() {
    display.setRotation(1); // Set rotation if needed
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);

   
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(80, 16); // Adjust text position

    display.println("Access Point");

    display.drawBitmap(0, 23, line_horizontal, 296, 2, GxEPD_BLACK);

    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(5, 42); // Adjust text position
    display.println("AP: SpectraLog Insight");

    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(5, 62); // Adjust text position
    display.println("IP: http://192.168.4.1/");

    display.drawBitmap(0, 70, line_horizontal, 296, 2, GxEPD_BLACK);

    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(5, 95); // Adjust text position
    display.println("Embedded System");

    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(5, 115); // Adjust text position
    display.println("Spectra Instruments 2025");

    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(5, 135); // Adjust text position
    display.println("www.spectrainstruments.cl");

    display.nextPage();
}

void AbyssalInnovation_Logo() {
  display.setRotation(2);
  display.fillScreen(GxEPD_BLACK);  
  //display.drawBitmap(0, 0, spectra_instruments_logo, 152, 296, 1); // Las dimensiones de la pantalla son 296x152 Pixeles. En caso de tener otras dimensiones, modificar estos valores.
  display.drawBitmap(60, 257, spectra_instruments_logo, 38, 46, 2);
  display.setTextColor(GxEPD_BLACK);

  display.nextPage();
}

void displaySleep() {
    display.hibernate(); 
}

