#include <SPI.h>
#include <stdlib.h>
#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "ImageData.h"
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
String v_current = "0.0.1"; //version
uint32_t sched = 0;

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
    if(sched == -1){
      Paint_Clear(BLUE);
      Paint_DrawString_EN(17,110, "Hello world!", &Font24, BLACK, BLUE);
      LCD_1IN28_Display(canvas);
    }
    if(sched == 5000000){
      Paint_Clear(RED);
      // Paint_DrawString_EN(17,110, "Hello world!", &Font24, BLACK, RED);
      Paint_DrawImage(BlackSquare88, 50, 50, 8, 8);
      Paint_DrawImage(WhiteSquare88, 100, 100, 8, 8);
      LCD_1IN28_Display(canvas);
    }
  }
}

void loop() {
  //unused
}
