#include <SPI.h>
#include <stdlib.h>
#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "sprites.h"
#include "LCD_1in28.h"
#include "QMI8658.h"

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
String v_current = "0.0.2"; //version
uint32_t sched = 0;
uint8_t gear = 0; //zero for neutral, 1-6 for gears
uint8_t temp = 50; //coolant temperature in C

void setup() {
  //serial setup
  Serial.begin(9600);
  Serial.println("Suzuki K-line interface v%s" + v_current); 
  
  //spi init
  DEV_Module_Init();
  
  //setup lcd
  LCD_1IN28_Init(HORIZONTAL);
  
  //turn on backlight
  DEV_SET_PWM(50);

  //clear display
  LCD_1IN28_Clear(WHITE);

  //allocate and set image buffer
  UDOUBLE Imagesize = LCD_1IN28_HEIGHT * LCD_1IN28_WIDTH * 2;
  UWORD *canvas;
  if ((canvas = (UWORD *)malloc(Imagesize)) == NULL)
  {
      Serial.println("Failed to allocate image memory...");
      exit(0);
  }
  Paint_NewImage((UBYTE *)canvas, LCD_1IN28.WIDTH, LCD_1IN28.HEIGHT, 0, WHITE);
  Paint_SetScale(65);

  //white out screen
  Paint_SetRotate(ROTATE_0);
  Paint_Clear(WHITE);
  LCD_1IN28_Display(canvas);

  while(1){ //main()
    sched++;
    if(sched == 10000000){
      sched = 0;//sched rolls over approximately once a second
    }
    if(sched == 0){
      Paint_Clear(BLACK);

      //gear
      switch (gear)
      {
      case 0:
        Paint_DrawImage(neutral, 88, 72, 64, 96);
        break;

      case 1:
        Paint_DrawImage(first, 88, 72, 64, 96);
        break;

      case 2:
        Paint_DrawImage(second, 88, 72, 64, 96);
        break;

      case 3:
        Paint_DrawImage(third, 88, 72, 64, 96);
        break;

      case 4:
        Paint_DrawImage(fourth, 88, 72, 64, 96);
        break;

      case 5:
        Paint_DrawImage(fifth, 88, 72, 64, 96);
        break;
      
      case 6:
        Paint_DrawImage(sixth, 88, 72, 64, 96);
        break;

      default:
        break;
      }

      //coolant temperature, drawn top to bottom to correct for overlap
      Paint_DrawImage(temp_1_1217, 6, 103, 12, 17); //always draw the first segment
      if(temp > 56){
        Paint_DrawImage(temp_2_1217, 8, 83, 12, 17);
      }
      if(temp > 62){
        Paint_DrawImage(temp_3_1617, 14, 65, 16,17);
      }
      if(temp > 68){
        Paint_DrawImage(temp_4_1616, 22, 48, 16, 16);
      }
      if(temp > 74){
        Paint_DrawImage(temp_5_1616, 36, 33, 16, 16);
      }
      if(temp > 80){
        Paint_DrawImage(temp_6_2014, 49, 21, 20, 14);
      }
      if(temp > 86){
        Paint_DrawImage(temp_7_2013, 67, 12, 20, 13);
      }
      if(temp > 92){
        Paint_DrawImage(temp_8_2011, 87, 7, 20, 11);
      }
      if(temp > 98){
        Paint_DrawImage(temp_9_248, 108, 6, 24, 8);
      }
      if(temp > 104){
        Paint_DrawImage(temp_10_2011, 133, 7, 20, 11);
      }
      if(temp > 110){
        Paint_DrawImage(temp_11_2013, 153, 12, 20, 13);
      }
      if(temp > 116){
        Paint_DrawImage(temp_12_2014, 170, 21, 20, 14);
      }
      if(temp > 122){
        Paint_DrawImage(temp_13_1616, 188, 33, 16, 16);
      }
      if(temp > 128){
        Paint_DrawImage(temp_14_1616, 202, 48, 16, 16);
      }
      if(temp > 134){
        Paint_DrawImage(temp_15_1617, 212, 65, 16, 17);
      }
      if(temp > 140){
        Paint_DrawImage(temp_16_1217, 220, 83, 12, 17);
      }
      if(temp > 146){
        Paint_DrawImage(temp_17_1217, 222, 103, 12, 17);
      }
      
      //display constructed canvas
      LCD_1IN28_Display(canvas);

      //update gear and temp for debug
      gear++;
      if(gear == 7){
        gear = 0;
      }
      temp = temp + 3;
      if(temp > 150){
        temp = temp - 100;
      }
    }
  }
}

void loop() {
  //unused
}
