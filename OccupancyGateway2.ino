// Gateway/ data aggregator for occupancy sensors

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
#include <SPI.h>       // bundled with Arduino IDE
#include "Timer.h"    // Simon Monk:  http://www.simonmonk.org
#include "RF24.h"    // TMRH2o : https://github.com/TMRh20/RF24
#include "RF24Network.h"  // http://tmrh20.github.io/RF24Network/
#include "RF24Mesh.h"    // http://tmrh20.github.io/RF24Mesh/4.h"    

// *** increment the following node id before burning it to every gateway: ***
#define gatewayID   1

Timer T;
RF24 radio(7, 8); // (CE, CS pins)
RF24Network network(radio);
RF24Mesh mesh(radio, network);
const byte led = 3;    // active low

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

// unsigned long tick_interval = 12000UL; // cmd interval in milliSec

void setup()  
{
    pinMode (led, OUTPUT);
    blinker();
    Serial.begin (9600);      
    Serial.println(F("NRF24 Gateway receiver starting.."));
    // node ID 0 is reserved for the master 
    mesh.setNodeID(0);
    Serial.print(F("This is the Mesh Master: Node #0"));  // print 0 for octal prefix
    Serial.println(mesh.getNodeID());  // read back that id
    // Connect to the mesh
    mesh.begin();
    Serial.println("Mesh started...");
    //T.every(tick_interval, tick);  // start software timer 
}

void loop()   
{
    T.update();
    mesh.update();   // keep the network updated
    mesh.DHCP();     // keep the 'DHCP service' running on the master 
    while(network.available()) {  
        read_data();
        toggle();
        upload_to_cloud ();
    }
}

RF24NetworkHeader header;
void read_data() {
    network.read(header, &status_payload, sizeof(status_payload));
    //Serial.print (F("Packet from: 0"));  // 0 is for octal prefix
    //Serial.print (header.from_node, OCT);         
}

char data_str[32];
void upload_to_cloud () {
    sprintf (data_str, "[G%d,N%d,S%d,T%d,H%d]\n", 
            gatewayID, status_payload.node_id, 
            status_payload.occupied, status_payload.temperature,
            status_payload.humidity);    
    Serial.print(data_str);          
}

void blinker() {
    for (int i=0; i<6; i++) {
        digitalWrite(led, 1);
        delay(200);
        digitalWrite(led, 0);
        delay(200);
    } 
}

boolean flag = 0;
void toggle() {
    digitalWrite(led, flag);
    flag = !flag;
}

