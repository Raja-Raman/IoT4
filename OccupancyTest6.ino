// 3D printed  box smart building: one RCWL 0516 and two PIRs and a DHT11 
// communication is through NRF24
// This is the gateway code; test it with occupancyTest5.ino
/* 
  http://arduino-info.wikispaces.com/Nrf24L01-2.4GHz-HowTo
  Uses the RF24 Library by TMRH2o here:
  https://github.com/TMRh20/RF24
   1 - GND
   2 - VCC 3.3V !!! NOT 5V (Only if you are using base module, you can use 5V)
   3 - CE to Arduino pin 7
   4 - CSN to Arduino pin 8
   5 - SCK to Arduino pin 13
   6 - MOSI to Arduino pin 11
   7 - MISO to Arduino pin 12
   8 - IRQ (UNUSED)
 */
#include "RF24.h"    // Download and Install (See above) 
#include "RF24Network.h"
#include "RF24Mesh.h"
#include <SPI.h>     // Comes with Arduino IDE
#include "Timer.h"
#include <dht.h>
  
Timer T;  // software timer
RF24 radio(7, 8);  // CE,CS pins  
RF24Network network(radio);
RF24Mesh mesh(radio, network);

unsigned long reset_interval = 120*1000UL;  // periodically reset the slave

struct {
    byte node_id;   
    char node_name;
    unsigned int temperature;
    unsigned int humidity;
} data_payload;

struct {  
    char command;
    unsigned int value;
} command_payload;

void(* software_reset) (void) = 0; // reset: jump to address 0

void setup() {  
    Serial.begin (9600); 
    // node ID 0 is reserved for the master 
    mesh.setNodeID(0);
    Serial.print(F("This is the Mesh Master: Node #0"));  // print 0 for octal prefix
    Serial.println(mesh.getNodeID());  // read back that id
    // Connect to the mesh
    mesh.begin();
    Serial.println("Mesh started...");
    T.every(reset_interval, tick);  // start software timer 
}

void loop() {
    T.update();   
    mesh.update();   
    mesh.DHCP();     // keep the 'DHCP service' running on the master     
    while(network.available())  
        read_data();    
}

void tick () {
    Serial.println("\r\nPreparing to reset the client...");
    reset_slave();
}

RF24NetworkHeader in_header;
void read_data() { 
      network.read(in_header, &data_payload, sizeof(data_payload));
      Serial.print (F("Header.From: 0"));
      Serial.print (in_header.from_node, OCT);
      Serial.print (F("  [Node #: "));
      Serial.print (data_payload.node_id);
      Serial.print (F("  Name: "));          
      Serial.print (data_payload.node_name);
      Serial.print (F("  Data: "));          
      Serial.print (data_payload.temperature);
      Serial.print (F(" C, "));   
      Serial.print (data_payload.humidity);          
      Serial.print (F(" %"));   
      Serial.println (F("]"));     
}

boolean result;
void reset_slave() {
    Serial.print(mesh.addrListTop);
    Serial.println (F(" client(s) connected."));
    for (int i = 0; i < mesh.addrListTop; i++) {
        command_payload.command = 'R';
        command_payload.value = 123;
        RF24NetworkHeader out_header (mesh.addrList[i].address, 'C');
        result = network.write(out_header, &command_payload, sizeof(command_payload));
        if (!result){
            Serial.print(F("Cannot send reset command to addrList.nodeID "));
            Serial.println (mesh.addrList[i].nodeID); 
            continue;   
        }         
        Serial.print(F("Reset command sent to addrList.nodeID: "));
        Serial.print (mesh.addrList[i].nodeID);     
        Serial.print(F(" / Address: 0"));
        Serial.println (mesh.addrList[i].address);             
    } // for
} 

