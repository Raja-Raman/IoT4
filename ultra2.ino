/* Ultrasonic Sensor HC-SR04; send moving average  
*
*/

// pins numbers
const int trigPin = 9;
const int echoPin = 10;
const int ledPin = 7;   // active LOW

// globals
const int numSamples = 30;
float samples[numSamples];
float distance;  //  average distance
int index = 0;
int threshold = 30;  // in cm

void setup() {
	pinMode(ledPin , OUTPUT); // Sets the led Pin as an Output
	pinMode(trigPin, OUTPUT); // Sets the trigger Pin as an Output
	pinMode(echoPin, INPUT);  // Sets the echo Pin as an Input
	Serial.begin(9600);       // Starts the serial communication
  blink(4);
}

void loop() {
  read();
  delay(10);  
}

void read() {
  samples[index] = measureDistance();
  index = (index+1)%numSamples;
  
  // mean distance
  distance = 0.0f;
  for (int i=0; i<numSamples; i++) 
    distance += samples[i];
  distance = distance/numSamples;
  
  // Prints the distance on the Serial Monitor
  Serial.println((int)distance);
  // indicate proximity
  if (distance < threshold)
    digitalWrite(ledPin, LOW);
  else
    digitalWrite(ledPin, HIGH);  
}

long duration;
float distCm;
float measureDistance () {
	// prepare the trigPin
	digitalWrite(trigPin, LOW);
	delayMicroseconds(2);
	// Sets the trigPin on HIGH state for 10 micro seconds
	digitalWrite(trigPin, HIGH);
	delayMicroseconds(10);
	digitalWrite(trigPin, LOW);
	// Reads the echoPin, returns the sound wave travel time in microseconds
	duration = pulseIn(echoPin, HIGH);
	// Calculating the distance
	distCm = duration*0.034/2;
  return (distCm);
}  

void blink(int times) {
  for (int i=0; i<times; i++) {
    digitalWrite(ledPin, LOW);
    delay(500);
    digitalWrite(ledPin, HIGH);
    delay(500);   
  }
}

