// HC-SR04 ultrasonic range sensor; for parking lot
// sends distance metrics and Floor/Car code to Android via blue tooth
// Takes baseline command from Android and auto-calibrates 
// TODO: take continuous moving average and computes variance  
// https://forum.arduino.cc/index.php?topic=106043.0

#include <SoftwareSerial.h>
const int trigPin = 6;
const int echoPin = 7;
const int redLed = 9;       // Car; active LOW
const int greenLed = 10;    // Floor; active LOW 

// Software serial port
const int softTx = 3;  // TX pin for Arduino; connect the Rx of BT shield to this pin
const int softRx = 2;  // RX pin for Arduino; Tx of the BT shield
SoftwareSerial softSerial(softRx, softTx); // The order is RX, TX

const long timeout = 25000L;   // 25 mSec = 4.25 meters max range
const int maxrange = 500;  // represents infinity
int numsamples = 30;  // for moving average
int actualsamples = 0;  // how many returned without timing out
int minimumsamples = 10;
int interPeriod = 10;  // mSec between readings
long duration;

int carThresh = 20;    // car height
int floorThresh = 100;   // floor height
int min_car_height = 120; // shortest car's height in cm
unsigned long lastupdate; 
long updateinterval = 1000L; // milli sec, for sending to BT 

int avgdistance = 0; // average a bunch of samples for noise reduction
int baseline_distance = 0;  // temp variable for calibration
int baseline_count = 8;  // how many bunches of samples for a baseline 
// for baselining, (numsamples*2*baseline_count) samples are averaged

void setup() 
{
    Serial.begin(9600);  
    softSerial.begin(9600);  
    pinMode(redLed, OUTPUT);
    pinMode(greenLed, OUTPUT);
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    digitalWrite(redLed, HIGH);  // active LOW
    digitalWrite(greenLed, HIGH); 
    blink1();
    calibrateFloor(); // this estimates the floor height
    carThresh = floorThresh - min_car_height;
    lastupdate = millis(); 
}

char rxbyte = 0;
char statusChar = 'E';
void loop()
{
  // TODO: handle message from android
  if (softSerial.available() > 0) {
      rxbyte = softSerial.read();
      if (rxbyte=='C' || rxbyte=='c') {
        calibrateCar();
        Serial.print("Car threshold: ");
        Serial.println(carThresh);
      }
      else
      if (rxbyte=='F' || rxbyte=='f') {
        calibrateFloor();
        Serial.print("Floor threshold: ");
        Serial.println(floorThresh);
      }
      else {
        Serial.print("Command error: ");
        Serial.println(rxbyte);
        blink2();
      }
   }  
   
   measureRange();  // global avgdistance is calculated by this
   
    if (actualsamples < minimumsamples){
       digitalWrite(redLed, HIGH);  
       digitalWrite(greenLed, HIGH);   
       statusChar = 'E';
    } 
    else 
    if (abs(avgdistance-carThresh) < abs(avgdistance-floorThresh)){
       digitalWrite(redLed, LOW);  
       digitalWrite(greenLed, HIGH);   
       statusChar = 'C';
    } else {
       digitalWrite(redLed, HIGH);  
       digitalWrite(greenLed, LOW);   
       statusChar = 'F';  
    }    
    // data sent over BT must be in the form C999 or F999 followed by CR-LF
    if (millis()-lastupdate > updateinterval) {
        softSerial.print(statusChar);
        softSerial.println(avgdistance);
        //Serial.println(avgdistance);
        lastupdate = millis(); 
    }
}

// computes ultrasonic range 
// returns the value in the global variable avgdistance
// sets the global variable actualsamples
int tmpdistance = 0;
void measureRange ()
{
    avgdistance=0;
    actualsamples=0;
    for (int i=0; i<numsamples; i++) {
        tmpdistance = getDistance();
        if (tmpdistance < maxrange) { // did not time out
            avgdistance += tmpdistance;
            actualsamples++;
        }
        delay(interPeriod);  // recomended gap is 35 mSec between readings ?
    }
    if (actualsamples > 0)
        avgdistance = avgdistance/actualsamples;  
     // else avgdistance will be 0
}

int distance=0;   // distance to object in cm
int  getDistance()
{ 
    // The sensor is triggered by a HIGH pulse of 10 or more microseconds.
    // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:  
    digitalWrite(trigPin, LOW); 
    delayMicroseconds(2);
    // Sets the trigPin on HIGH state for 10 micro seconds
    digitalWrite(trigPin, HIGH); 
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    // Returns the sound's travel time in microseconds
    duration = pulseIn(echoPin, HIGH, timeout); 
    // sound speed: 340 meters/sec = .034 cm/microsec
    // divide by 2 for round trip
    if (duration == 0L)  // timed out
        distance = maxrange;
    else
        distance = duration*0.034/2;
    return distance;
}

// computes the floor height & baselines it
void calibrateFloor()
{
    baseline_distance=0;
    digitalWrite (redLed, HIGH);   
    for (int i=0; i<baseline_count; i++) {
        digitalWrite (greenLed, LOW); 
        measureRange();  // this sets the global avgdistance
        baseline_distance += avgdistance;
        delay(100);  
        digitalWrite (greenLed, HIGH); 
        measureRange();  // this sets the global avgdistance
        baseline_distance += avgdistance;      
        delay(100);  
    }
    baseline_distance = baseline_distance/(2*baseline_count);
    floorThresh = baseline_distance;
}

// computes the car height & baselines it
void calibrateCar()
{
    baseline_distance=0;
    digitalWrite (greenLed, HIGH);   
    for (int i=0; i<baseline_count; i++) {
        digitalWrite (redLed, LOW); 
        measureRange();  // this sets the global avgdistance
        baseline_distance += avgdistance;
        delay(100);  
        digitalWrite (redLed, HIGH); 
        measureRange();  // this sets the global avgdistance
        baseline_distance += avgdistance;      
        delay(100);  
    }
    baseline_distance = baseline_distance/(2*baseline_count);
    carThresh = baseline_distance;
}

// Restarting delay
void blink1()
{
    for (int i=0; i<5; i++) {
      digitalWrite (redLed, LOW);
      digitalWrite (greenLed, HIGH);    
      delay(1000);  
      digitalWrite (redLed, HIGH); 
      digitalWrite (greenLed, LOW);    
      delay(1000);  
    }
    digitalWrite (redLed, HIGH); 
    digitalWrite (greenLed, HIGH);     
}

// ERROR indicator
void blink2()
{
    for (int i=0; i<15; i++) {
      digitalWrite (redLed, LOW);
      digitalWrite (greenLed, LOW);    
      delay(100);  
      digitalWrite (redLed, HIGH); 
      digitalWrite (greenLed, HIGH);    
      delay(100);  
    }
}
