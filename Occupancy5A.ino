// Smart building with US parking sensor: occupancy alone
// TODO: periodically try to connect to the RF mesh; but if not, keep running the lights
/*---------------------------------------------------------------------------- 
   NRF24 connections
   http://arduino-info.wikispaces.com/Nrf24L01-2.4GHz-HowTo   
   1 - GND
   2 - VCC 3.3V  *** (5V allowed Only if you are using the NRF base module)
   3 - CE   to Arduino pin 7
   4 - CSN  to Arduino pin 8
   5 - SCK  to Arduino pin 13
   6 - MOSI to Arduino pin 11
   7 - MISO to Arduino pin 12
   8 - IRQ (UNUSED)
 ---------------------------------------------------------------------------- */
#include <EEPROM.h>     // bundled with Arduino IDE
#include <SPI.h>       // bundled with Arduino IDE
#include "Timer.h"    // Simon Monk:  http://www.simonmonk.org -> Jack Christensen
#include "RF24.h"    // TMRH2o : https://github.com/TMRh20/RF24
#include "RF24Network.h"  // http://tmrh20.github.io/RF24Network/
#include "RF24Mesh.h"    // http://tmrh20.github.io/RF24Mesh/
#include <dht.h>  // Rob Tillaart: https://github.com/RobTillaart/Arduino/tree/master/libraries/DHTlib

// *** increment the following node id before burning it to every Arduino: ***
#define  nodeID         5
#define  SIMULATION     0      // 0 for production hardware 
//#define  NORELAY      1

const byte FREE_PIN   = A7;    // any unconnected analog pin
const byte pir1       = A4;     // HC-SR 501
const byte radar      = A5;     // RCWL 0516 
const byte led        = 3;     // blue; active high  
const byte buzzer     = 5;    // actually red LED

//const byte pir2       = 6;    // HC-SR 501
//const byte relay      = 6;     // green LED; active high
//const byte dhtsensor  = A2;    // DHT 11


dht D;    
Timer T;
RF24 radio(9, 8); // (CE, CS pins)
RF24Network network(radio);
RF24Mesh mesh(radio, network);

// * Timer durations SHOULD be unsigned LONG int, if they are > 16 bit! 
unsigned int tick_interval = 100;      // in milliSec; 10 ticks = 1 second 
unsigned int status_ticks  = 5*10;     // n*10 ticks = n seconds  
unsigned int release_ticks = 120*10;   // n*10 ticks = n seconds  
unsigned int buzzer_ticks  = 110*10;   // n*10 ticks = n seconds

unsigned int data_interval = 10*1000;  // directly in milli seconds  
unsigned int tick_counter = 0;
unsigned int status_counter = 0;
unsigned long network_check_interval = 60000UL;


byte  num_hits = 0;           // number of sensors that fired
byte  release_duration = 2;   // can be 1,2 or 3 minutes
int   temperature = 0;        // DHT11 only gives integers  
int   humidity = 0;           // DHT22 can give decimal accuracy
boolean auto_mode = true;     // false->manual mode; TODO:implement manual mode
byte mute_buzzer = 0;         // TODO: 1 for production
boolean occupied = true;      // occupancy status of the room

struct {
    byte node_id;   // 1 to 255
    byte occupied;
    unsigned int temperature;
    unsigned int humidity;    
} status_payload;

struct {  
    char command;        // command name;  TODO: implement manual commands
    unsigned int value;  // some parameter, in the context of the command
} command_payload;

void(* software_reset) (void) = 0; //declare reset function - jump to address 0

void setup() {
    pinMode(led, OUTPUT);   
    //pinMode(relay, OUTPUT); 
    pinMode(buzzer, OUTPUT); 
    //if (!NORELAY)
    //    digitalWrite(relay, HIGH);    // active high; start with relay operated   
    digitalWrite(led, HIGH);      // active low       
    digitalWrite(buzzer, HIGH);   // active low   
    //pinMode(dhtsensor, INPUT);     
    pinMode(radar, INPUT);   
    pinMode(pir1, INPUT);           
    //pinMode(pir2, INPUT);      
    //beeper(0);  // TODO: disable this for production
    blinker();      
    
    Serial.begin(9600);
    Serial.println(F("Occupancy sensor starting..."));
    //EEPROM.get(0, mute_buzzer); // TODO: save tick interval also in EEPROM
    // Set the nodeID manually
    mesh.setNodeID(nodeID);  // increment this when burning every device
    status_payload.node_id = nodeID;
    Serial.print(F("Slave Node ID: "));    
    Serial.println(nodeID);    
    Serial.print(F("Simulation = "));Serial.println(SIMULATION);
    //Serial.print(F("NORELAY = "));Serial.println(NORELAY);
    Serial.println(F("Connecting to the mesh..."));
    unsigned long ms = millis();
    bool result = mesh.begin(MESH_DEFAULT_CHANNEL,RF24_1MBPS,8000);    
    Serial.println(F("Time taken: "));
    Serial.println(millis()-ms);
    if (result)
        Serial.println(F("Connected to mesh."));
    else
        Serial.println(F("Connection timed out."));    
    /*
    int palevel = radio.getPALevel();
    Serial.print(F("Radio PA level: "));
    Serial.println(palevel);
    radio.setPALevel (RF24_PA_LOW);
    Serial.print(F("New PA level: "));
    palevel = radio.getPALevel();
    Serial.println(palevel);     
    Serial.println(F("Joined the meash."));
    */
    randomSeed(analogRead(FREE_PIN));  // noise from an unconnected pin
    status_ticks = status_ticks + random(0, 10); // stagger the transmissions
    Serial.print(F("Status tick interval: "));
    Serial.println(status_ticks*100U);  // convert to mSec
    T.every(tick_interval, ticker);     
    T.every(data_interval, read_temperature);  // just update readings; do not send it
    T.every(network_check_interval, renew_network); // check connection and renew if necessary
    occupy_room();    // start life in occupied state (this needs the mesh running)
}

void loop() {
    T.update();   
    mesh.update();   // keep the network updated
}

void ticker(){
    read_sensors();  // read PIR and radar...
    update_status(); // ...and then compute occupancy status
    if (network.available())  // this is usually a while() loop  
        read_command();        
}

// You MUST call this before calling update_status
void read_sensors() { 
    num_hits = 0;   // global variable
    num_hits += digitalRead(radar);
    num_hits += digitalRead(pir1); 
    //num_hits += digitalRead(pir2); 
    digitalWrite (led, (num_hits==0));  // active low
}

// uses the global variable num_hits: Call read_sensors before calling this !
void update_status() {    
    if (!occupied && auto_mode)
        if (num_hits > 1)    // at least two sensors fired - occupy the room
            occupy_room();
    if (num_hits > 0)        // at least one fired; so the room is in use
          tick_counter = 0;  // keep resetting it, if there is any motion
    tick_counter++;          // Note: the sensors can keep tick_counter perpetually zero! So,
    status_counter++;        // you need a separate status_counter

    if (status_counter == status_ticks) { 
        send_status();
        status_counter = 0;
    }
    if (tick_counter == buzzer_ticks) {
        if (occupied && auto_mode)
            warn();  // warn about the imminent release
    }
    else        
    if (tick_counter >= release_ticks){
         tick_counter = 0;
         if (occupied && auto_mode)
            release_room();  
    }  
}

void occupy_room() {
  //if (!NORELAY)
  //      digitalWrite(relay, HIGH); // active high
    occupied = true; 
    if (!mute_buzzer)
        T.pulse (buzzer, 200, LOW); // active high  // 100
    // the end state is HIGH, i.e, buzzer is off *
    send_status(); 
}

// pre-release warning beeps
void warn() {
    T.oscillate (buzzer,50, LOW, 4);  
    // the end state is low, i.e, buzzer is off *  
}

void release_room() {
    //if (!NORELAY)  
    //    digitalWrite(relay, LOW);  // active high  
    occupied = false; 
    send_status(); 
}

// this is independently running under a timer
void read_temperature() { 
    // Reading the DHT11 takes about 250 milliseconds
    //D.read11(dhtsensor);
    temperature =0.0;  // D.temperature;
    humidity = 0.0; // D.humidity;  
}

// the temperature and humidity are updated independently by another timer
boolean result;
void send_status() {
    if (SIMULATION) {
        status_payload.occupied = random(0,2);  // [min, max)
        status_payload.temperature = random(0, 40);
        status_payload.humidity = random(0,100);    
    }
    else {
        status_payload.occupied = occupied;
        status_payload.temperature = temperature;
        status_payload.humidity = humidity;  
    }
    // TODO: disable this for production
    Serial.print(status_payload.occupied ? "Occupied, " : "Free, "); 
    Serial.print(status_payload.temperature); 
    Serial.print(F(", "));
    Serial.print(status_payload.humidity);  
       
    result = mesh.write(&status_payload, 'S', sizeof(status_payload));
    if (result)  
        Serial.println();
    else 
        Serial.println(F("  (Send failed)*"));
}

bool renew_network() {
    if (mesh.checkConnection()) {
        return (true);
    }
    else {
        Serial.println(F("Renewing network address..."));
        unsigned int addr = mesh.renewAddress(3000UL);  
        if (addr != 0) {        
            Serial.print(F("New address: "));
            Serial.println(addr);
            return (true);
        }
    }   
    return (false);
}

// TODO: combine this with execute_command()
RF24NetworkHeader in_header;
void read_command() {
    network.read(in_header, &command_payload, sizeof(command_payload));
    Serial.print(F("Received <- Header.From: 0"));
    Serial.print(in_header.from_node, OCT);
    Serial.print(F(" command: "));
    Serial.println(command_payload.command);
    //Serial.print(F("  value: "));
    //Serial.println(command_payload.value);   
    if (command_payload.command == 'R') {
         Serial.println(F("\r\nGot reset command from Master!! "));
         Serial.println(F("\nRestarting in 5 seconds.... "));
         delay(5000);
         software_reset();
    }
} 
  
void blinker() {
    for (int i=0; i<5; i++) {  // 10
        digitalWrite(led, LOW);    
        delay(100);      
        digitalWrite(led, HIGH);        
        delay(100); 
    }
    digitalWrite(led, LOW); 
}

void beeper (int mode) {
    switch (mode) {
      case 0: // useful before entering T.update loop
          digitalWrite(buzzer, LOW);  // active low
          delay(200);  
          digitalWrite(buzzer, HIGH);
          break;
      case 1:
          T.pulse(buzzer, 4000, HIGH); // active low
          break;
      default:
          T.pulse(buzzer, 200, HIGH); // active low
          break;
    }
}

