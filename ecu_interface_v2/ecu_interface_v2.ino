#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "images.h"

//timer for interrupts
#define SCHED_ROLLOVER 300 //timer rollover in milliseconds
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
  display.setTextSize(3);            
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();

  //splash
  display.setCursor(15,20); //find center later
  display.print(F("SUZUKI"));
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
  if(task_sched){
    if(timer_ms > 200){
      gear = 2;
    }
    else if(timer_ms > 100){
      gear = 1;
    }
    else{
      gear = 0;
    }
    switch(gear){//display current gear
      case 0:
        display.clearDisplay();
        display.drawBitmap(0,0,neutral,32,64,WHITE);
        break;
      case 1:
        display.clearDisplay();
        display.drawBitmap(0,0,first,32,64,WHITE);
        break;
      default:
        display.clearDisplay();
        display.drawBitmap(0,0,uncertain, 32,64,WHITE);
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