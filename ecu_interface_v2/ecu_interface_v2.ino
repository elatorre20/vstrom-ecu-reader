#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306_EMULATOR.h>
#include "images.h"

//timer for interrupts
volatile uint16_t timer_ms = 0; //used for scheduling
volatile uint8_t task_sched = 0; //used to ensure tasks only execute once if they take less than 1ms
#define TIMER_INTERRUPT_DEBUG         2
#define _TIMERINTERRUPT_LOGLEVEL_     0
#define USE_TIMER_1     true
#define TIMER_INTERVAL_MS 1L //interval for timer interrupt in ms
#define SCHED_ROLLOVER 1000 //number of periods for timer_ms to rollover
#include <TimerInterrupt.h>
void TimerHandler(){ //timer interrupt handler
  task_sched = 1;
  timer_ms++;
  if(timer_ms == SCHED_ROLLOVER){
    timer_ms = 0;
  }
}


//display 
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET     -1 
#define SCREEN_ADDRESS 0x3D 
#define SSD1306_NO_SPLASH

//diag port 
#define BAUDRATE 10400
#define KLINE_PIN 1 //serial1 TX
#define LLINE_PIN 2 //any GPIO
char *init_sequence = {}




Adafruit_SSD1306_EMULATOR display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  //serial setup
  Serial.begin(9600);
  Serial.println("Motorcycle diagnostic interface");

  //timer setup
  ITimer1.init();
  ITimer.attachInterruptInterval(TIMER_INTERVAL_MS, TimerHandler);

  //display setup
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.setTextSize(2);            
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();

  //splash
  display.setCursor(0,0); //find center later
  display.print(F("SUZUKI"));
  display.display();
  delay(1000);
  display.clearDisplay();
  display.display();

  //diag UART setup
  pinMode(KLINE_PIN, OUTPUT);//setup to send init on k and l lines
  pinMode(LLINE_PIN, OUTPUT);
  fastInit();
  Serial1.begin(BAUDRATE);
  Serial1.write();

  

}

void loop() {
  // put your main code here, to run repeatedly:

}


void fastInit(){ //fast KWP2000 init protocol on k and l lines
  digitalWrite(KLINE_PIN, LOW);
  digitalWrite(LLINE_PIN, LOW);
  delay(25);
  digitalWrite(KLINE_PIN, HIGH);
  digitalWrite(LLINE_PIN, HIGH);
  delay(25);
}