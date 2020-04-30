// Rainfall Simulator
// Author:Reagan,Andrew

int potPin = 0;    // select the input pin for the potentiometer
int waterPin = 13;   // select the pin for the LED
int val = 0;       // variable to store the value coming from the sensor

void setup() {
  pinMode(waterPin, OUTPUT);  // declare the ledPin as an OUTPUT
}

void loop() {
  val = map(analogRead(potPin),0,1023,0,5000);    // read the value from the sensor
  digitalWrite(waterPin, HIGH);  // turn the ledPin on
  delay(val);                  // stop the program for some time
  digitalWrite(waterPin, LOW);   // turn the ledPin off
  delay(val);                  // stop the program for some time
}
