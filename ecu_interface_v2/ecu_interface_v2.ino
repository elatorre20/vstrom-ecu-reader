#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "images.h"

//timer for interrupts
#define KEEPALIVE_PERIOD 200 //in periods of about 10ms
#define UPDATE_PERIOD 100
volatile uint8_t task_sched = 0;
volatile uint8_t send_keepalive = 1; //time to send keepalive
volatile uint8_t update = 1; //time to update
void timerSetup(uint16_t period){//period roughly in MS with a 16MHz clock. 
  //Note that if you are not using a 32u4-based MCU, you will have to change the register names/setup appropriately
  TCCR1A = 0; //no OC
  TCCR1B = 0; //clear control register
  TCCR1B = (1<<CS12)|(0<<CS11)|(1<<CS10)|(0<<WGM13)|(1<<WGM12); //1024 prescaler, counting up to OCR1A
  OCR1A = 16*period; //16,000,000/1024/ICR1 = timer frequency
  TIMSK1 |= (1<<OCIE1A);
}
ISR(TIMER1_COMPA_vect){ //timer interrupt handler
  task_sched = 1;
  send_keepalive++;//increment all counters
  update++;
  if(send_keepalive == KEEPALIVE_PERIOD){
    send_keepalive = 0;
  }
  if(update == UPDATE_PERIOD){
    update = 0;
  }
}


//display 
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET     -1 
#define SCREEN_ADDRESS 0x3D 
#define SSD1306_NO_SPLASH
uint8_t valid_cmd = 0; //for if the user command is valid
uint8_t line = 0;//for scrolling the display


//diag port 
#define BAUDRATE 10400
#define KLINE_PIN 1 //serial1 TX
#define LLINE_PIN 4 //any GPIO
uint8_t initialized = 0;
unsigned char write_buf[32]; //buffer for data to be written to ecu
unsigned char data_buf[64]; //buffer for data recieved from ecu

//messages 
//format: 81 12 F1 (ecu address) <request length> <request> <checksum>
const static char PROGMEM init_msg[] =    {0x81, 0x12, 0xF1, 0x81, 0x05}; //initialize, 5 bytes
const static char PROGMEM keepalive[] =   {0x80, 0x12, 0xF1, 0x01, 0x3E, 0xC2}; //keepalive, 6 bytes
const static char PROGMEM quit[] =        {0x80, 0x12, 0xF1, 0x01, 0x82, 0x06}; //quit communcations, 6 bytes
const static char PROGMEM clear_dtc[] =   {0x80, 0x12, 0xF1, 0x03, 0x14, 0x00, 0x00, 0x9A}; //clear DTC, 8 bytes
const static char PROGMEM sensor_data[] = {0x80, 0x12, 0xF1, 0x02, 0x21, 0x08, 0xAE}; //query all sensors, 7 bytes

//stats 
//all decodings are taken from https://github.com/synfinatic/sv650sds/wiki
//only the ones marked CONFIRMED produced sensible values in my testing
uint16_t speed = 0; //speed in KPH, byte 16 * 2
uint16_t rpm = 0; //RPM, 10*byte 17 + byte 18/10
uint16_t tps = 0; //throttle position sensor value, byte 19 range 0x37-0xDD
uint8_t iap1 = 0; //intake air pressure 1, byte 20
uint16_t engine_temp = 0; //engine oil temperature in celsius, (byte 21-48)/1.6
uint16_t intake_temp = 0; //intake air temperature in celsius, (byte 22-48)/1.6
float v_bat = 0; //battery voltage, byte 24*20/255
uint8_t gear = 0; //gear, byte 26 CONFIRMED
uint8_t iap2 = 0; //intake air pressure 2, byte 27
uint8_t throt_valv = 0; //throttle valve, byte 46
uint8_t clutch = 0; //clutch switch, byte 52 bit 4 CONFIRMED
uint8_t neutral = 0; //neutral switch, byte 53 bit 1



Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  //usb serial setup
  Serial.begin(9600);
  Serial.println(F("Motorcycle diagnostic interface"));

  //timer setup
  timerSetup(10);

  //diag UART setup
  pinMode(KLINE_PIN, OUTPUT);//Kline must be high for >50ms before init
  pinMode(LLINE_PIN, OUTPUT);
  digitalWrite(KLINE_PIN, HIGH);
  digitalWrite(LLINE_PIN, HIGH);


  //display setup
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);           
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(8);
  display.clearDisplay();
  display.drawBitmap(32,0,logo_splash,64,64,SSD1306_WHITE);
  display.display();
  display.clearDisplay();

  //allow time for ECU to start up after ignition switched
  delay(3000);
}

void loop() {
  if(!initialized){//establish connection if dropped
    initialized = 1;
    fastInit();
  }
  // if(!send_keepalive && initialized){//periodically send keepalives
  //   writeFromProgmem(keepalive, 6);
  // }
  if(!update  && initialized){//perioically update and draw gear
    query();
    drawGear();
  }
}


void drawGear(){
  display.clearDisplay();
  switch(gear){//display current gear
    case 0:
      display.setCursor(4,4);
      display.print(F("N"));
      break;
    case 1:
      display.setCursor(4,4);
      display.print(F("1"));
      break;
    case 2:
      display.setCursor(4,4);
      display.print(F("2"));
      break;
    case 3:
      display.setCursor(4,4);
      display.print(F("3"));
      break;
    case 4:
      display.setCursor(4,4);
      display.print(F("4"));
      break;
    case 5:
      display.setCursor(4,4);
      display.print(F("5"));
      break;
    case 6:
      display.setCursor(4,4);
      display.print(F("6"));
      break;
    default:
      display.setCursor(4,4);
      display.print(F("-"));
      break;
  }
  display.display();
}
//Message timings (2006 Vstrom DL650): 5-7ms arduino to send message, 32-33ms ECU to begin response, 54-55ms ECU to transmit sensor data for request 0x21 0x08, 5-7ms ECU to respond to keepalive
void readResponse(){//read bytes returned by ECU
  delay(10); //ensure that UART is done writing
  while(Serial1.available()){ //because the UART is half-duplex, have to flush sent characters from the recieve buffer
    Serial1.read();
  }
  delay(100);//wait for ECU to finish sending whole response
  memset(data_buf, 0, 64);//clear data buffer
  Serial1.readBytes(data_buf, Serial1.available());//read recieved bytes
  if(data_buf[0]){//if data read, report success
    Serial.println(F("ECU responded successfully"));
  }
  else{
    initialized = 0;//if no data recieved back, communication has dropped
  }
}

void parseData(){ //parse received data and update variables
  Serial.println(F("Recieved data: "));
  for(uint8_t i = 0; i < 64; i++){//print all recieved data to USB serial
    Serial.print((uint8_t)data_buf[i], HEX);
    Serial.print(F(" "));
  }
  Serial.println("\n");
  //update and print stats
  //speed
  speed = (uint16_t)data_buf[16] * 2;
  Serial.print(F("Speed KPH: "));
  Serial.println((uint16_t)speed, DEC);
  //rpm
  rpm = ((data_buf[17] * 10) + data_buf[18]) / 10;
  Serial.print(F("RPM: "));
  //Serial.println((uint8_t)rpm, DEC);
  Serial.print((uint8_t)data_buf[17], DEC);
  Serial.println((uint8_t)data_buf[18], DEC);
  //throttle position sensor
  tps = ((uint8_t)data_buf[19] - 0x37)/0xA6;
  Serial.print(F("TPS %: "));
  Serial.println((uint8_t)tps, DEC);
  //intake air pressure 1
  iap1 = data_buf[20];
  Serial.print(F("IAP 1: "));
  Serial.println((uint8_t)iap1, DEC);
  //engine temp
  engine_temp = ((uint8_t)data_buf[21] - 48)/1.6;
  Serial.print(F("Engine oil temp *C: "));
  Serial.println((uint8_t)engine_temp, DEC);
  //intake temp
  intake_temp = ((uint8_t)data_buf[22]-48)/1.6;
  Serial.print(F("Intake air temperature *C: "));
  Serial.println((uint8_t)intake_temp, DEC);
  //battery voltage
  v_bat = ((uint8_t)data_buf[24]*20)/255;
  Serial.print(F("Battery voltage: "));
  Serial.println((uint8_t)v_bat, DEC);
  //gear
  gear = data_buf[26];
  Serial.print(F("Gear: "));
  Serial.println((uint8_t)gear, DEC);
  //intake air pressure 2
  iap2 = data_buf[27];
  Serial.print(F("Intake air pressure 2: "));
  Serial.println((uint8_t)iap2, DEC);
  //throttle valve opening
  throt_valv = data_buf[46];
  Serial.print(F("Throttle valve: "));
  Serial.println(throt_valv, DEC);
  //clutch switch
  clutch = data_buf[52];
  Serial.print(F("Clutch switch: "));
  Serial.println((uint8_t)clutch, HEX);
  //neutral switch
  neutral = data_buf[53];
  Serial.print(F("Neutral switch: "));
  Serial.println((uint8_t)neutral, HEX);
}

void query(){
  writeFromProgmem(sensor_data, 7);
  readResponse();//get data returned by ECU
  if(initialized){
    parseData();
  }
}

void fastInit(){ //fast KWP2000 init protocol on k and l lines
  Serial1.end(); //release serial for GPIO control
  delay(100); //KWP2000 init protocol: >50ms high, 25ms low, 25ms high, send init sequence
  digitalWrite(KLINE_PIN, LOW);
  digitalWrite(LLINE_PIN, LOW);
  delay(25);
  digitalWrite(KLINE_PIN, HIGH);
  digitalWrite(LLINE_PIN, HIGH);
  delay(25);
  Serial1.begin(BAUDRATE);
  writeFromProgmem(init_msg, 5);
  readResponse(); //check whether ECU has responded
}

void disconnect(){ //disconnect to allow for reconnection attempts
  writeFromProgmem(quit, 6);
  initialized = 0;
}

void writeFromProgmem(char* msg, uint8_t msglen){//allows saving messages in flash
  uint8_t count = 0;
  while(count < msglen){
    write_buf[count] = pgm_read_byte_near(msg + count);
    count++;
  }
  Serial1.write(write_buf, count);
}
