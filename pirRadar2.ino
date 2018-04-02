// PIR detector and microwave radar; software/usb serial output
#define ON  1
#define OFF 0

//#include <SoftwareSerial.h>
//SoftwareSerial softSerial(3,2); // RX, TX of Arduino

const int pirPin = 8;
const int radarPin = 11;
const int ledPin1 = 4;
const int ledPin2 = 5;

void setup() {
  Serial.begin(9600);
  //softSerial.begin(9600);
  pinMode(ledPin1 , OUTPUT);  
  pinMode(pirPin , INPUT);
  pinMode(ledPin2 , OUTPUT);  
  pinMode(radarPin , INPUT);  
  //Serial.println("PIR Radar starting1...");
  //softSerial.println("PIR Radar starting2...");
  blinker();
}

boolean input=0;
boolean input1=0;
boolean input2=0;
boolean prev=0;

void loop() {
  //repeater();
  detectMovement();
}

void detectMovement() {
   input1 = digitalRead(pirPin);
   input2 = digitalRead(radarPin);
   //input =  input1;
   //input =  input2;
   input = input1 & input2;

   if (input != prev) {
      //softSerial.println(input);
      Serial.print(input);
      prev = input;
   }
   digitalWrite(ledPin1, input1);
   digitalWrite(ledPin2, input2);
   delay(100);
}

/*
char inp;
void repeater() {
  if (softSerial.available()) {
    inp = softSerial.read();
    Serial.print(inp);
  }
  if (Serial.available()) {
    inp = Serial.read();
    softSerial.print(inp);
  }  
}
*/

void blinker() {
  for (int i=0; i<6; i++) {
    digitalWrite(ledPin1, ON);
    digitalWrite(ledPin2, ON);
    delay(250);
    digitalWrite(ledPin1, OFF);
    digitalWrite(ledPin2, OFF);
    delay(250);
  }
}

