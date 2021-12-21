/* Waveshare Library epd1in54_V2
 * changed by Andreas Wolter 29.09.2020
 *
 * Demo copyright Waveshare 2019
 *
 * changes:	- ESP32 / ESP8266 compatibility
 *			- epd Constructor changed (input own pin numbers)
 */ 

#include <SPI.h>
#include "epd1in54_V2.h"
#include "imagedata.h"
#include "epdpaint.h"
#include <stdio.h>

/* Info: For connecting DIN / CLK pins to MOSI / SCK pins check datasheed and pinout
 * For example:
 * ESP32 MOSI = 23, SCK = 18
 * ESP8266 MOSI = D7 (GPIO 13), SCK = D5 (GPIO 14)
 * Arduino Uno MOSI = 11, SCK = 13
 */

// Uncommend the Line with board configuration of your choice
// (On D1 Mini ESP8266 Pin D4 (GPIO2) prevents upload to the board)
// EPD epd(RESET PIN, DC PIN, CS PIN, BUSY PIN);
//
//Epd epd(5, 0, SS, 4); 		// my Pins ESP8266 (D1, D3, D8, D2)
//Epd epd(33, 25, 26, 27); 		// my Pins ESP32 (Reset, DC, CS, Busy)
//Epd epd; 						// default Pins: Reset = 8, DC = 9, CS = 10, Busy 7

unsigned char image[1024];
Paint paint(image, 0, 0);

unsigned long time_start_ms;
unsigned long time_now_s;
#define COLORED     0
#define UNCOLORED   1

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("e-Paper init and clear");
  epd.LDirInit();
  epd.Clear();

  paint.SetWidth(200);
  paint.SetHeight(24);

  Serial.println("e-Paper paint");
  paint.Clear(COLORED);
  paint.DrawStringAt(30, 4, "Hello world!", &Font16, UNCOLORED);
  epd.SetFrameMemory(paint.GetImage(), 0, 10, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  paint.DrawStringAt(30, 4, "e-Paper Demo", &Font16, COLORED);
  epd.SetFrameMemory(paint.GetImage(), 0, 30, paint.GetWidth(), paint.GetHeight());

  paint.SetWidth(64);
  paint.SetHeight(64);

  paint.Clear(UNCOLORED);
  paint.DrawRectangle(0, 0, 40, 50, COLORED);
  paint.DrawLine(0, 0, 40, 50, COLORED);
  paint.DrawLine(40, 0, 0, 50, COLORED);
  epd.SetFrameMemory(paint.GetImage(), 16, 60, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  paint.DrawCircle(32, 32, 30, COLORED);
  epd.SetFrameMemory(paint.GetImage(), 120, 60, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  paint.DrawFilledRectangle(0, 0, 40, 50, COLORED);
  epd.SetFrameMemory(paint.GetImage(), 16, 130, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  paint.DrawFilledCircle(32, 32, 30, COLORED);
  epd.SetFrameMemory(paint.GetImage(), 120, 130, paint.GetWidth(), paint.GetHeight());
  epd.DisplayFrame();
  delay(2000);

  Serial.println("e-Paper show pic");
  epd.HDirInit();
  epd.Display(IMAGE_DATA);

  //Part display
  epd.HDirInit();
  epd.Clear();
  epd.DisplayPartBaseWhiteImage();

  //paint.SetRotate(90);
  paint.SetWidth(200);
  paint.SetHeight(20);
  paint.Clear(UNCOLORED);

  char i = 0;
  char str[10][10] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
  for (i = 0; i < 10; i++) {
    paint.Clear(UNCOLORED);
    paint.DrawStringAt(0, 0, str[i], &Font24, COLORED);
    epd.SetFrameMemory(paint.GetImage(), 20, 20, paint.GetWidth(), paint.GetHeight());
    epd.DisplayPartFrame();
    delay(200);
  }

  Serial.println("e-Paper clear and goto sleep");
  epd.HDirInit();
  epd.Clear();
  epd.Sleep();
}

void loop()
{

}
