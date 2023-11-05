#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

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
#define LLINE_PIN 4 //any GPIO
char write_buf[32];
String read_buf;
const static char PROGMEM init_sequence[] = {0xC1, 0x33, 0xF1, 0x81, 0x66}; //size is stored in the first byte of the message, figure out later

//misc
uint8_t line = 0;//for scrolling the display

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  //serial setup
  Serial.begin(9600);
  Serial.println(F("Motorcycle diagnostic interface"));

  //timer setup
  timerSetup(10);

  //diag UART setup
  pinMode(KLINE_PIN, OUTPUT);//setup to send init on k and l lines
  pinMode(LLINE_PIN, OUTPUT);
  digitalWrite(KLINE_PIN, HIGH);
  digitalWrite(LLINE_PIN, HIGH);


  //display setup
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);           
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.clearDisplay();
  display.display();
  

  Serial.println(F("Ready, enter command: "));//prompt user for command
}

void loop() {
  
  while(!Serial.available());//wait for input
  read_buf = Serial.readString();//read input to buffer
  fastInit();
  //display output
  display.setCursor(0,(8*(line++ % 8)));//scroll display
  display.fillRect(0,(8*((line-1) % 8)),128,8,SSD1306_BLACK);//clear line text
  display.print(String(line, DEC) + ". ");
  display.setCursor(20,(8*(line-1 % 8)));
  display.print(read_buf);
  display.display();

}


void fastInit(){ //fast KWP2000 init protocol on k and l lines
  digitalWrite(KLINE_PIN, LOW);
  digitalWrite(LLINE_PIN, LOW);
  delay(25);
  digitalWrite(KLINE_PIN, HIGH);
  digitalWrite(LLINE_PIN, HIGH);
  delay(25);
  Serial1.begin(BAUDRATE);
  writeFromProgmem(init_sequence, 5);
}

void writeFromProgmem(char* msg, uint8_t msglen){
  uint8_t count = 0;
  while(count < msglen){
    write_buf[count] = pgm_read_byte_near(msg + count);
    count++;
  }
  Serial1.write(write_buf, count);
}