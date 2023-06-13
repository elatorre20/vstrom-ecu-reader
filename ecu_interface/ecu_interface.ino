#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GrayOLED.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>
#include <gfxfont.h>
#include <Adafruit_GC9A01A.h>

//LCD pins
#define LCD_BACKLIGHT 25
#define LCD_DC 8
#define LCD_CS 9
#define LCD_CLK 10
#define LCD_MOSI 11
#define LCD_RST 12

//Gyro pins
#define IMU_SDA 6
#define IMU_SCL 7
#define IMU_INT1 23
#define IMU_INT2 24

//global variables
Adafruit_GC9A01A lcd(LCD_CS, LCD_DC);
String v_current = "0.0.1"; //version

void setup() {
  //serial console for debug
  Serial.begin(9600);
  Serial.println("Suzuki K-line interface v%s" + v_current); 

  //LCD setup
  lcd.begin();
  pinMode(LCD_BACKLIGHT, OUTPUT);
  digitalWrite(LCD_BACKLIGHT, HIGH);
}

void loop() {
  // put your main code here, to run repeatedly:
  lcd.fillScreen(0xF800);
  yield();
  Serial.println("RED");
  delay(1000);
  lcd.fillScreen(0x001F);
  yield;
  Serial.println("BLUE");
  delay(1000);
}
