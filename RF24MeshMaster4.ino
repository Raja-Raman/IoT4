/** USE THIS IN CONJUNCTION WITH RF24MESHSLAVE4.INO
   Based on RF24Mesh_Example_Master.ino and RF24Mesh_Example_Master_Statics by TMRh20
   Send data to nodes based on their ID.
   The nodes can change physical or logical position in the network, and reconnect through 
   different routing nodes as required. The master node manages address assignments 
   similar to DHCP.
 **/

#include "RF24Network.h"
#include "RF24.h"
#include "RF24Mesh.h"
#include <SPI.h>
#include <Timer.h>

Timer T;  // software timer
RF24 radio(7, 8);  // CE,CS pins  
RF24Network network(radio);
RF24Mesh mesh(radio, network);
unsigned long tick_interval = 12000UL; // cmd interval in milliSec

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
    // node ID 0 is reserved for the master 
    mesh.setNodeID(0);
    Serial.print(F("This is the Mesh Master: Node #0"));  // print 0 for octal prefix
    Serial.println(mesh.getNodeID());  // read back that id
    // Connect to the mesh
    mesh.begin();
    Serial.println("Mesh started...");
    T.every(tick_interval, tick);  // start software timer 
}

void loop() {
    T.update();
    mesh.update();   // keep the network updated
    mesh.DHCP();     // keep the 'DHCP service' running on the master 
    while(network.available())  
        read_data();
}

void tick () {
    send_command();
}

RF24NetworkHeader in_header;
void read_data() {
    network.peek(in_header);
    switch (in_header.type) {
      case 'S':  // status
          network.read(in_header, &status_payload, sizeof(status_payload));
          Serial.print (F("Header.From: 0"));  // 0 is for octal prefix
          Serial.print (in_header.from_node, OCT);
          Serial.print (F("  [Node #: "));      
          Serial.print (status_payload.node_id);          
          Serial.print (F("  Occupied: "));          
          Serial.print (status_payload.occupied);
          Serial.println (F("]"));
          break;       
      case 'D':  // data
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
          break;
      default:
          network.read(in_header, 0, 0);
          Serial.print (F("Header.From: 0"));  // 0 is for octal prefix
          Serial.print(in_header.from_node, OCT);          
          Serial.print (F("  Unknown header type: "));
          Serial.println(in_header.type);     
          break;
      }
}

byte cmd = 0;
unsigned int val = 1;
boolean result;
void send_command() {
    Serial.print(mesh.addrListTop);
    Serial.println (F(" client(s) connected."));
    for (int i = 0; i < mesh.addrListTop; i++) {
        cmd = (cmd+1) % 3;
        val = (val+1) % 100;
        command_payload.command = 'X' + cmd;
        command_payload.value = val;
        RF24NetworkHeader out_header (mesh.addrList[i].address, 'C');
        result = network.write(out_header, &command_payload, sizeof(command_payload));
        if (!result){
            Serial.print(F("Cannot send command to addrList.nodeID "));
            Serial.println (mesh.addrList[i].nodeID); 
            continue;   
        }         
        Serial.print(F("Command sent to addrList.nodeID "));
        Serial.print (mesh.addrList[i].nodeID);   
        Serial.print (" : [");  
        Serial.print (command_payload.command);    
        Serial.print (",");  
        Serial.print (command_payload.value);   
        Serial.println (F("]"));    
    } // for
}



