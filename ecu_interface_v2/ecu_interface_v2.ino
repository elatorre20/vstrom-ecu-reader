#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "images.h"

//timer for interrupts
#define SCHED_ROLLOVER 100 //timer rollover in milliseconds
volatile uint8_t task_sched = 0;
volatile uint16_t timer_ms = 0; //actually 1 timer period, not 1ms
void timerSetup(uint16_t period){//period roughly in MS
  TCCR1A = 0; //no OC
  TCCR1B = 0; //clear control register
  TCCR1B = (1<<CS12)|(0<<CS11)|(1<<CS10)|(0<<WGM13)|(1<<WGM12); //1024 prescaler, counting up to OCR1A
  OCR1A = 16*period; //16,000,000/1024/ICR1 = timer frequency
  TIMSK1 |= (1<<OCIE1A);
}
ISR(TIMER1_COMPA_vect){ //timer interrupt handler
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
char init_sequence[] = {0xC1, 0x33, 0xF1, 0x81, 0x66};

//misc
uint8_t gear = 0;




Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  //serial setup
  Serial.begin(9600);
  Serial.println("Motorcycle diagnostic interface");

  //timer setup
  timerSetup(10);


  //display setup
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);           
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(8);
  display.clearDisplay();
  display.drawBitmap(32,0,logo_splash,64,64,SSD1306_WHITE);
  display.display();
  delay(3000);
  

  //diag UART setup
  pinMode(KLINE_PIN, OUTPUT);//setup to send init on k and l lines
  pinMode(LLINE_PIN, OUTPUT);
  fastInit();
  Serial1.begin(BAUDRATE);
  Serial1.write(init_sequence, 5);

}

void loop() {
  // put your main code here, to run repeatedly:
  if(task_sched){//only execute once per timer period
    if(timer_ms == 50){
      gear++;
      if(gear == 8){
        gear = 0;
      }
    }
    
    switch(gear){//display current gear
      case 0:
        display.clearDisplay();
        display.setCursor(4,4);
        display.print(F("N"));
        break;
      case 1:
        display.clearDisplay();
        display.setCursor(4,4);
        display.print(F("1"));
        break;
      case 2:
        display.clearDisplay();
        display.setCursor(4,4);
        display.print(F("2"));
        break;
      case 3:
        display.clearDisplay();
        display.setCursor(4,4);
        display.print(F("3"));
        break;
      case 4:
        display.clearDisplay();
        display.setCursor(4,4);
        display.print(F("4"));
        break;
      case 5:
        display.clearDisplay();
        display.setCursor(4,4);
        display.print(F("5"));
        break;
      case 6:
        display.clearDisplay();
        display.setCursor(4,4);
        display.print(F("6"));
        break;
      default:
        display.clearDisplay();
        display.setCursor(4,4);
        display.print(F("-"));
        break;
    }
    display.display();
    task_sched = 0;
  }
}


void fastInit(){ //fast KWP2000 init protocol on k and l lines
  digitalWrite(KLINE_PIN, LOW);
  digitalWrite(LLINE_PIN, LOW);
  delay(25);
  digitalWrite(KLINE_PIN, HIGH);
  digitalWrite(LLINE_PIN, HIGH);
  delay(25);
}