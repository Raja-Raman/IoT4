
/**  USE THIS IN CONJUNCTION WITH RF24MESHMASTER4.INO
   Based on RF24Mesh_Example by TMRh20
   This example sketch shows how to manually configure a node using RF24Mesh, and send data 
   to the master node.
   The nodes refresh their network address as soon as a single write fails. This allows the
   nodes to change position in relation to each other and the master node.

   User Configuration: nodeID - A unique identifier for each radio. Allows addressing
   to change dynamically with physical changes to the mesh.
   In this example, configuration takes place below, prior to uploading the sketch to the device
   A unique value from 1-255 must be configured for each node.
   As this is stored in EEPROM, remains persistent between power cycles.
*/

#include "RF24.h"
#include "RF24Network.h"
#include "RF24Mesh.h"
#include <SPI.h>
#include <Timer.h>
//#include <printf.h>

// *** change the following node id before burning it to every Arduino: ***
#define nodeID  5

# define FREE_PIN   A7    // any unconnected pin
Timer T;  // software timer
RF24 radio(7, 8);  // CE,CS pins  
RF24Network network(radio);
RF24Mesh mesh(radio, network);
unsigned long tick_interval = 3000UL; // cmd interval in milliSec

struct {
    byte node_id;   
    char node_name;
    byte occupied;
} status_payload;

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
  Serial.println(F("Sensor node starting..."));
  //printf_begin();
  // Set the nodeID manually
  mesh.setNodeID(nodeID);
  
  status_payload.node_id = nodeID;
  status_payload.node_name = 'A' + (nodeID-1);
  data_payload.node_id = nodeID;
  data_payload.node_name = 'A' + (nodeID-1);
    
  Serial.print(F("Slave Node ID: "));    
  Serial.print(nodeID);
  Serial.print(F("  Node name: "));   
  Serial.println(status_payload.node_name);  
  
  randomSeed(analogRead(FREE_PIN));  // noise from an unconnected pin
  tick_interval = tick_interval + random(0, 3)*1000 + random (50, 950);  
  Serial.print(F("Tick interval: "));
  Serial.println(tick_interval);
  Serial.println(F("Connecting to the mesh..."));
  mesh.begin();
  T.every(tick_interval, tick);  // start software timer 
}

void loop() {
    T.update();  
    mesh.update();   // keep the network updated
    while(network.available())  
        read_command();
}  

byte packet_count = 0;
void tick () {
    send_status();
    packet_count = (packet_count+1) % 4;
    if (packet_count == 0)
        send_data();
}

boolean result;
void send_status() {
    status_payload.occupied = random(0,2);  // [min, max)
    result = mesh.write(&status_payload, 'S', sizeof(status_payload));
    if (result) {
        Serial.print(F("Sent status -> from node "));
        Serial.print(status_payload.node_name); 
        Serial.print(F(" : "));
        Serial.println(status_payload.occupied ? "Occupied" : "Free"); 
    }
    else
        renew_network();
}

void send_data() { 
    data_payload.temperature = random(16,46);  // [min, max)
    data_payload.humidity = random(5,101);  // [min, max)
    result = mesh.write(&data_payload, 'D', sizeof(data_payload));
    if (result) {
        Serial.print(F("Sent data -> from node "));
        Serial.print(data_payload.node_name); 
        Serial.print(F(" : "));
        Serial.print(data_payload.temperature); 
        Serial.print(F(", "));
        Serial.println(data_payload.humidity);         
    }
    else
        renew_network();
}

void renew_network() {
    if (mesh.checkConnection()) 
        Serial.println(F("Send failed; but connection is OK"));
    else {
        Serial.println(F("Renewing network address..."));
        mesh.renewAddress();          
    }   
}

RF24NetworkHeader in_header;
void read_command() {
    network.read(in_header, &command_payload, sizeof(command_payload));
    Serial.print(F("Received <- Header.From: 0"));
    Serial.print(in_header.from_node, OCT);
    Serial.print(F(" command: "));
    Serial.print(command_payload.command);
    Serial.print(F("  value: "));
    Serial.println(command_payload.value);    
}






