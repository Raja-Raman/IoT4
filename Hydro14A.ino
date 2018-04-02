/*
MAC address of Hydro Tube Bluetooth HC-05 is 98:D3:32:20:EA:01 
*/
#include <SoftwareSerial.h> 
#include <dht.h>         // Rob Tillaart: https://github.com/RobTillaart/Arduino/tree/master/libraries/DHTlib
#include <Timer.h>       // J.Christensen: by https://github.com/JChristensen/Timer
#include <TimeLib.h>
#include <Time.h>         // http://www.arduino.cc/playground/Code/Time  
#include <Wire.h>         // http://arduino.cc/en/Reference/Wire (included with Arduino IDE)
#include <DS3232RTC.h>    // J.Christensen: http://github.com/JChristensen/DS3232RTC

#define  pump_led       5
#define  bubbler_led    6 
#define  red_led        13
#define  green_led      8
#define  buzzer         4  
#define  pump_relay     A0
#define  bubbler_relay  A1
#define  temp_sensor    9
#define  light_sensor   A6

// The following pins use internal pullup, driven by TTL outputs
#define  flow_pin       2     // DB9 pin 8  
#define  rtc_pin        3
// The following are active LOW input, with internal pullup
#define  reservoir_alarm_pin      11    // DB9 pin  3  
#define  tube_alarm_pin           12    // DB9 pin  4   
// DB9 pin 6,7 : ground; pin 9: Vcc  

#define MAX_BUFFER      10
char buffer[MAX_BUFFER];  // command buffer
byte ptr = 0;   // buffer pointer

char time_stamp[25];  
char status_str[50];  // *** sufficient memory is needed for this string!

int  light_level = 0;       // brighter light -> lower value of light_intensity
float temperature = 0.0f;   
float humidity = 0.0f;
volatile int pulse_count = 0;  // count per 2 second interval
int pulse_count_copy = 0;      // make a copy for leisurely processing

boolean is_flowing = false;
boolean is_alarm = 0;
boolean tube_alarm = false;
boolean reservoir_alarm = false;
boolean flow_alarm = false;

SoftwareSerial SoftSerial(A3, A2);  // RX, TX from Arduino's point of view
Timer T;  // software timer
dht DHT;
time_t time_now;

void setup() {
    pinMode(pump_relay, OUTPUT);       // active low   
    pinMode(bubbler_relay, OUTPUT);    // active low
    pinMode(buzzer, OUTPUT);           // active low      
    digitalWrite(pump_relay, HIGH);    // active low   
    digitalWrite(bubbler_relay, HIGH); // active low    
    digitalWrite(buzzer, HIGH);        // active low        
    
    pinMode(pump_led, OUTPUT);         // active high
    pinMode(bubbler_led, OUTPUT);      // active high
    pinMode(red_led, OUTPUT);          // active high
    pinMode(green_led, OUTPUT);        // active high 
    digitalWrite(pump_led ,LOW);       
    digitalWrite(bubbler_led ,LOW); 
    digitalWrite(red_led ,LOW); 
    digitalWrite(green_led ,LOW);    

    pinMode(temp_sensor, INPUT);       // no external pullup (the module has it)
    pinMode(light_sensor, INPUT);      // external 10K POT for sensitivity    
    pinMode(flow_pin, INPUT_PULLUP);   // flow meter interrupt 
    pinMode(rtc_pin, INPUT_PULLUP);    // RTC interrupt    
    pinMode(reservoir_alarm_pin, INPUT_PULLUP);       // active LOW input
    pinMode(tube_alarm_pin, INPUT_PULLUP);        // active LOW input
  
    Serial.begin(9600);
    SoftSerial.begin(9600);      
    Serial.println(F("Hydroponics tube starting..."));   
    SoftSerial.println(F("Hydroponics tube starting.."));         
    blink ();
    beep(0);
    attachInterrupt(0, pulse_ISR, RISING);  //rising edges from the flow meter     

    if (init_RTC()) {  // TODO: revisit this logic     
        Serial.println(F("Entering main loop..."));   
    }   else {
        beep(1);
        Serial.println(F("*** RTC FAILURE. ABORTING ***"));  
        // TODO: replace RTC with backup software timer
    }      
    print_time_stamp();
    Serial.println();    
}
#-------------------------------------------------------------------------------

void loop() {
    T.update();
    read_command();    
    if (RTC.alarm(ALARM_1)) // tick every second
        second_tick();   
}
#-------------------------------------------------------------------------------

byte epoch = 4; // 5 seconds epoch, split at 1 second resolution
// Invoked once in a second
void second_tick() {  
    epoch = (epoch+1) % 4;
    switch(epoch) {
        case 0:
            read_flow_meter();   // sensitive to delays      
            check_alarms();     // first check the status...
            wink();             // .. and then indicate 
            break;
        case 1:
            if (RTC.alarm(ALARM_2))  // one minute tick raised?
                check_onoff();
            break;
        case 2:
            read_flow_meter();    // sensitive to delays   
            check_alarms();            
            wink();
            break;
        case 3:
            send_status();
            break; 
        default:
            Serial.println(F("Illegal state !!!"));                           
    }    

}

int minute_watcher = 0;
void check_onoff() {
    minute_watcher = RTC.get();
    if (minute_watcher % 5 == 0)
        minute_watcher = 0;
    swich (minute_watcher) {
        case 0: // * called every 5th minute *
            read_sensors();
            send_data();
            break;
        case 1:
            start_pump();
            break;   
        case 11:
            stop_pump();
            break; 
        case 29:
            start_bubbler();
            break; 
        case 59:
            stop_bubbler();
            break; 
        default :  
            break;                     
    }
}


void check_pump() {   
    if (!RTC.alarm(ALARM_2)) // once a minute alarm
        return;  
    time_now = RTC.get();
    if (minute(time_now) % 5==0) {
        // TODO: operate relay here
        T.after(watchdog_delay, enable_watchdog);
        print_time_stamp();
        Serial.println("Pump ON");
    }      
    else
    if (minute(time_now) % 5==2) {
        // TODO: release relay here
        disable_watchdog();
        print_time_stamp();
        Serial.println("Pump OFF");
    }
}

void check_bubbler(){
  
}

void check_alarms(){
  
}
/*      
void enable_watchdog () {
    print_time_stamp();
    Serial.println("Watchdog is on");    
}

void disable_watchdog () {
    print_time_stamp();
    Serial.println("Watchdog is off");    
}
*/
char c = '?';    // serial input char 
void read_command() { 
    while (Serial.available()) {   // TODO: use SoftSerial     
       c = Serial.read();
       switch(c) {
          case '(' :
              ptr = 0;  
              buffer[ptr] = c;
              ptr++;
              break;
          case ')' :    // closing bracket is not stored
              buffer[ptr] = '\0';
              ptr = (ptr+1) % MAX_BUFFER;                
              process_packet();
              break;
          default:
              buffer[ptr] = c;  
              ptr = (ptr+1) % MAX_BUFFER;  
       }
   }
}

/* 
Packet structure: (a999)
A single alphabet followed by an integer, all enclosed in parenthesis
*/ 
//int data = -1;  // can hold from -1 to 32,767
unsigned int data = 0;  // can hold from 0 to 65,535
void process_packet() {  // TODO 
    if (buffer[0] != '(') {
        SoftSerial.println(F("Packet Delimiter Error"));
        return;
    }
    if (buffer[1] < 'a'  || buffer[1] > 'z') {
        SoftSerial.println(F("Packet Type Error"));
        return;
    }
    SoftSerial.print(buffer);    
    SoftSerial.print(" ");  
    send_status();  // TODO: implement other commands also
}

void send_status() { // TODO    
    SoftSerial.print("--DUMMY STATUS--");  
    //SoftSerial.println(status, BIN);  
}
//----------------------------------------------------------------------------

// read the overflow and underflow alarm bits
void read_alarms() {
    tube_high = digitalRead(tube_high_pin);
    reservoir_low = digitalRead(reservoir_low_pin);   
}

// read light, temperature and humidity
void read_sensors() { 
    read_light();  // takes about 100 milliseconds
    // Reading the DHT11 takes about 250 milliseconds
    DHT.read11(temp_sensor);
    temperature = DHT.temperature;
    humidity = DHT.humidity;  
}

// internal helper method
unsigned int tmp_intensity = 0;
void read_light() {
    tmp_intensity = 0;
    for (int i=0; i<10; i++) { 
        tmp_intensity += analogRead(light_sensor);
        delay(10);    // TODO: should we read every second and average ?
    }
    light_intensity = tmp_intensity/10;
}

int flow_threshold = 20;  // revolutions per 2 sec; TODO: check this
void read_flow_meter(){
    cli();            // Disable interrupts
    pulse_count_copy = pulse_count;  
    pulse_count = 0;  // ready for next counting       
    sei();            // Enable interrupts 
    water_flowing = (pulse_count_copy > flow_threshold);  // this is a global variable 
}

void send_data(){
    sprintf (status_str,
        "[D,%d,%d.%1d,%d.%1d,%d]",
        light_intensity, 
        (int) temperature, ((int)(temperature*10))%10,  // sprintf doesn't accept floats;
        (int) humidity, ((int)(humidity*10))%10,         // so this is a work around 
        pulse_count_copy
        );
    SoftSerial.println(status_str);  // new line terminated
    Serial.println(status_str);
}
//----------------------------------------------------------------------

void make_time_stamp(){
    time_now = RTC.get();
    sprintf(time_stamp,
    "%d-%s-%d  %d:%02d:%02d ",
    day(time_now),monthShortStr(month(time_now)),year(time_now),
    hour(time_now),minute(time_now),second(time_now)
    );
}

// Makes a global time stamp string and then prints
void print_time_stamp() {
    make_time_stamp();
    Serial.print (time_stamp);    
}
//----------------------------------------------------------------------

void pulse_ISR () { // pulse counting ISR for flow meter
    pulse_count++; //
}
//----------------------------------------------------------------------

void(* software_reset) (void) = 0; //declare reset function - jump to address 0

//----------------------------------------------------------------------
byte wink_leds = {green_led, red_led};
void wink(){
    T.pulse (wink_leds[is_alarm], 200, HIGH);
}

void blink() {
    for (int i=0; i<3; i++) {
        digitalWrite(pump_led, HIGH);
        digitalWrite(red_led, HIGH);           
        digitalWrite(bubbler_led, LOW);  
        digitalWrite(green_led, LOW);  
        delay(500);                             
        digitalWrite(pump_led, LOW);
        digitalWrite(red_led, LOW);      
        digitalWrite(bubbler_led, HIGH);
        digitalWrite(green_led, HIGH);       
        delay(500);          
    }
    digitalWrite(pump_led, LOW);
    digitalWrite(red_led, LOW);      
    digitalWrite(bubbler_led, LOW);
    digitalWrite(green_led, LOW);     
}

void beep (int mode) {
    switch (mode) {
      case 0: // useful before entering T.update loop
          digitalWrite(buzzer, LOW);  // active LOW
          delay(200);  
          digitalWrite(buzzer, HIGH);
          break;
      case 1:
          T.pulse(buzzer, 4000, HIGH);  // end state is HIGH
          break;
      case 2:
          T.pulse(buzzer, 100, HIGH);
          break;          
      case 3:
          T.pulse(buzzer, 50, HIGH);
          break;           
      default:
          T.pulse(buzzer, 200, HIGH);
          break;
    }
}

boolean init_RTC() {
    setSyncProvider(RTC.get); // sync with RTC every 5 minutes
    if (timeStatus() != timeSet) {
        Serial.println(F("*** PANIC: Failed to Synchronize with RTC ***"));
        return (false);
    }
    else
        Serial.println(F("Synchronized with RTC.")); 
    //Disable the default square wave of the SQW pin
    RTC.squareWave(SQWAVE_NONE);  // use the SQW pin only for alarm triggering

    //daydate parameter should be between 1 and 7
    RTC.setAlarm(ALM1_EVERY_SECOND, 0, 0, 0, 1);    
    RTC.alarm(ALARM_1);     // clear RTC interrupt flag               
    RTC.setAlarm(ALM2_EVERY_MINUTE, 0, 0, 0, 1);   
    RTC.alarm(ALARM_2);                    
    return (true);    
}

