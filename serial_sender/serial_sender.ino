#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "images.h"

//timer for interrupts
#define SCHED_ROLLOVER 100 //timer rollover in periods
volatile uint8_t task_sched = 0;
volatile uint8_t send_keepalive = 1;
volatile uint16_t timer_ms = 0; //actually 1 timer period, not 1ms
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
  timer_ms++;
  if(timer_ms == SCHED_ROLLOVER){
    send_keepalive = 1;
    timer_ms = 0;
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
char write_buf[32]; //buffer for data to be written to ecu
String cmd_buf; //buffer for commands read in from usb serial
char data_buf[64]; //buffer for data recieved from ecu

//messages 
//format: 81 12 F1 (ecu address) <request length> <request> <checksum>
const static char PROGMEM init_msg[] =    {0x81, 0x12, 0xF1, 0x81, 0x05}; //initialize, 5 bytes
const static char PROGMEM keepalive[] =   {0x80, 0x12, 0xF1, 0x01, 0x3E, 0xC2}; //keepalive, 6 bytes
const static char PROGMEM quit[] =        {0x80, 0x12, 0xF1, 0x01, 0x82, 0x06}; //quit communcations, 6 bytes
const static char PROGMEM clear_dtc[] =   {0x80, 0x12, 0xF1, 0x03, 0x14, 0x00, 0x00, 0x9A}; //clear DTC, 8 bytes
const static char PROGMEM sensor_data[] = {0x80, 0x12, 0xF1, 0x02, 0x21, 0x08, 0xAE}; //query all sensors, 7 bytes

//stats 
uint8_t speed = 0; //speed in KPH, byte 16 * 2
uint16_t rpm = 0; //RPM, 10*byte 17 + byte 18/10
uint8_t tps = 0; //throttle position sensor value, byte 19 range 0x37-0xDD
uint8_t iap1 = 0; //intake air pressure 1, byte 20
uint8_t engine_temp = 0; //engine oil temperature in celsius, (byte 21-48)/1.6
uint8_t intake_temp = 0; //intake air temperature in celsius, (byte 22-48)/1.6
uint8_t v_bat = 0; //battery voltage, byte 24*20/255
uint8_t gear = 0; //gear, byte 26
uint8_t iap2 = 0; //intake air pressure 2, byte 27
uint8_t throt_valv = 0; //throttle valve, byte 46
uint8_t clutch = 0; //clutch switch, byte 52 bit 4
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
  display.setTextSize(1);
  display.clearDisplay();
  display.drawBitmap(32,0,logo_splash,64,64,SSD1306_WHITE);
  display.display();
  display.clearDisplay();
  

  Serial.println(F("Ready, enter command: "));//prompt user for command
}

void loop() {
  if(send_keepalive && initialized){
    send_keepalive = 0;
    writeFromProgmem(keepalive, 6);
  }
  if(task_sched){ //check and send at most once per 10ms
    task_sched = 0;
    if(Serial.available()){ //check for input
      cmd_buf = Serial.readString();//read input to buffer
      cmd_buf.trim();
      valid_cmd = 0;
      //command parser
      if(cmd_buf == F("INIT")){//do fast initialization
        valid_cmd = 1;
        timer_ms = 0; //only need to do keepalives if a command has not been sent for at least 1s
        fastInit();
        cmd_buf = F("Initializing");
      }
      else if(cmd_buf == F("QUIT")){ //disconnect from ecu
        valid_cmd = 1;
        timer_ms = 0;
        writeFromProgmem(quit, 6);
        initialized = 0;
        cmd_buf = F("Disconnecting");
      }
      else if(cmd_buf == F("CLEAR")){ //clear DTC
        valid_cmd = 1;
        timer_ms = 0;
        writeFromProgmem(clear_dtc, 8);
        cmd_buf = F("Disconnecting");
      }
      else if(cmd_buf == F("QUERY")){ //get new sensor data
        valid_cmd = 1;
        timer_ms = 0;
        writeFromProgmem(sensor_data, 7);
        cmd_buf = F("Data requested");
        readResponse();//get data returned by ECU
        parseData(); //update stats and print data
      }
      //display output
      if(valid_cmd){
        display.setCursor(0,(8*(line++ % 8)));//scroll display
        display.fillRect(0,(8*((line-1) % 8)),128,8,SSD1306_BLACK);//clear line text
        display.print(String(line, HEX) + ". ");
        display.setCursor(20,(8*((line-1) % 8)));
        display.print(cmd_buf);
        Serial.println(cmd_buf);
        display.display();
      }
      else{
        Serial.println(F("Invalid Command"));
      }
    }
  }
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
    cmd_buf = F("Data queried successfully");
  }
}

void parseData(){ //parse received data and update variables
  Serial.println(F("Recieved data: "));
  for(uint8_t i = 0; i < 64; i++){//print all recieved data to USB serial
    Serial.print(data_buf[i], HEX);
    Serial.print(F(" "));
  }
  Serial.println("\n");
  //update and print stats
  //speed
  speed = data_buf[16] * 2;
  Serial.print(F("Speed KPH: "));
  Serial.println(speed, DEC);
  //rpm
  rpm = (data_buf[17] * 10) + (data_buf[18] / 10);
  Serial.print(F("RPM: "));
  Serial.println(rpm, DEC);
  //throttle position sensor
  tps = (data_buf[19] - 0x37)/0xA6;
  Serial.print(F("TPS %: "));
  Serial.println(tps, DEC);
  //
}

uint8_t iap1 = 0; //intake air pressure 1, byte 20
uint8_t engine_temp = 0; //engine oil temperature in celsius, (byte 21-48)/1.6
uint8_t intake_temp = 0; //intake air temperature in celsius, (byte 22-48)/1.6
uint8_t v_bat = 0; //battery voltage, byte 24*20/255
uint8_t gear = 0; //gear, byte 26
uint8_t iap2 = 0; //intake air pressure 2, byte 27
uint8_t throt_valv = 0; //throttle valve, byte 46
uint8_t clutch = 0; //clutch switch, byte 52 bit 4
uint8_t neutral = 0; //neutral switch, byte 53 bit 1

void fastInit(){ //fast KWP2000 init protocol on k and l lines
  initialized = 0;//stop keepalives if retrying intialization
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
  timer_ms = 0;//reset keepalive clock
  initialized = 1;//begin sending keepalives
}

void writeFromProgmem(char* msg, uint8_t msglen){//allows saving messages in flash
  uint8_t count = 0;
  while(count < msglen){
    write_buf[count] = pgm_read_byte_near(msg + count);
    count++;
  }
  Serial1.write(write_buf, count);
}
