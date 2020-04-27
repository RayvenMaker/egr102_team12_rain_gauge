// Main rain gauge file to run on Arduino
// Authors: Reagan G, Andrew S
#include <LiquidCrystal.h>

#define cw_motor 8
#define ccw_motor 9
#define l_button 10
#define c_button 11
#define r_button 12
#define hall 13

const int rs = 7, en = 6, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// These are the profiles for the different crops.
// The first number in a pair is the day from start to change once
// The second number in a pair is the rainfall in inches per day that the crop
// will recieve until the next change
const float corn[] = {1, 0.5, 100, 1.5, 150, 1.0, 250, 0.5};
const float wheat[] = {1, 0.5, 100, 1.5, 150, 1.0, 250, 0.5};
const float rice[] = {1, 0.5, 100, 1.5, 150, 1.0, 250, 0.5};
const float *profiles[] = {corn, wheat, rice};
const char *profileNames[] = {"corn", "wheat", "rice"};
const char *screens[] = {"current profile", "rainfall (in/s)", "motor duty %", "irrigation(in/s)", "days since start"};
const byte buttons[] = {l_button, c_button, r_button};

const float bucketSize = 0.01;    // Size of each bucket drop in inches of rain
const unsigned long hourLength = 5000; // Length of an "hour" in milliseconds
const unsigned long rainfallCheckInterval = 100; // Time between rainfall checks

byte screen = 0;
byte motorDuty = 0;

int currentProfile = 0;
byte previousBucketState = 0;

/* Program structure:
Starting up:
  - Gets the current "time and date"
  - Shows a loading screen.
Running mode:
  - Displays one value at a time:
  - Current profile (profileNames[currentProfile], select to change)
    * LB RB to highlight, CB to select
  - Current rainfall (reads rain gauge)
  - Motor speed (dutyCycle, select to change)
    * LB decreases duty cycle, RB decreases duty cycle
    * Resets to profile default after 1 day
  - Current irrigation (calculated irrigation, select to change)
    * LB decreases inches per day, RB decreases inches per day
    * Resets to profile default after 1 day
  - Current day (reads the clock)
*/

void setup() {
  // RUN THE CODES ONCE
  Serial.begin(9600);
  lcd.begin(16, 2);

  lcd.print("Irrigator v0.1");
  lcd.setCursor(0, 1);
  lcd.print("loading...");
  delay(3000);
  lcd.clear();

  screen = 0; // Or otherwise run profileScreen once first
}

void loop() {
  // RUN THE CODES MANY TIMES

  // Get current time and set up timers
  unsigned long currentMillis = millis();
  static unsigned long previousMotorM = 0;
  static unsigned long previousHour = 0;
  static unsigned long previousRainCheck = 0;
  static unsigned int currentHour = 0;

  // Check if an hour has passed
  if(currentMillis - previousHour >= hourLength) {
    previousHour = currentMillis;
    currentHour++;
  }

  // Read rain gauge every checking interval
  if(currentMillis - previousRainCheck >= rainfallCheckInterval){
    float rainfallRate = readRainfall();
  }

  // check if any buttons were pressed
  byte* currentPressed = checkButtons();

  // Set up the LCD change tracker
  static byte lcdRefresh = 1;

  // Display management block
  switch(screen) {
    case 0:
      // Shows current profile
      byte* profileState = profileScreen(currentPressed, lcdRefresh, currentProfile);
      screen = profileState[0];
      lcdRefresh = profileState[1];
      currentProfile = profileState[2];
      break;
    case 1:
      // Shows current rainfall
      screen = rainfallScreen(currentPressed, lcdRefresh);
      break;
    case 2:
      // Shows current motor speed
      byte* motorState = motorScreen(currentPressed, lcdRefresh);
      screen = motorState[0];
      motorDuty = motorState[1];
      break;
    case 3:
      // Shows current irrigation level
      byte* irrigationState = irrigationScreen(currentPressed, lcdRefresh);
      screen = irrigationState[0];
      irrigationLevel = irrigationState[1];
      break;
    case 4:
      // Shows current day in program
      screen = dateScreen(currentPressed, lcdRefresh);
      break;
  }


  // Calculate irrigation levels
  moveDuration = 60000 * (100 / calcIrrigation(rainfallRate));
  // Motor movement block
  if(currentMillis - previousMotorM >= moveDuration) {

  }


}

// Manages the LCD display and the customizable functions
byte* profileScreen(byte buttonStates[], byte lcdChanged, byte currentProfile) {
  static byte editing = 0;
  byte screen = 0;          // Current screen

  if(lcdChanged == 1) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("current profile");
    lcd.setCursor(0, 1);
    // This is broken now, it's not getting the length of the full word
    byte profileLen = sizeof(profiles[currentProfile])/sizeof(profiles[0]);
    lcd.print("<");
    lcd.setCursor(((16 - profileLen)/2 + 1), 1);
    lcd.print(profiles[currentProfile]);
    lcd.setCursor(15, 1);
    lcd.print(">");
    lcdChanged = 0;
  }

  if(buttonStates[3] == 1) { // Checks if a button state has been changed
    lcdChanged = 1;
    if(editing == 0) {

      if(buttonStates[0] == 1) {
        if(screen == 0){
          screen = sizeof(screens)/sizeof(screens[0]);
        } else {
          screen--;
        }
      }

      if(buttonStates[1] == 1){
        editing = !editing;
      }

      if(buttonStates[2] == 1) {
        if(screen == sizeof(screens)/sizeof(screens[0])) {
          screen = 0;
        } else {
          screen++;
        }
      }

    } else if(editing == 1) {
      // Check if the left button is pressed
      if(buttonStates[0] == 1) {
        if(currentProfile == 0) {
          currentProfile = sizeof(profiles)/sizeof(profiles[0]);
        } else {
          currentProfile--;
        }
      }
      // Check if the center button is pressed
      if(buttonStates[1] == 1) {
        editing = !editing;
      }
      // Check if the right button is pressed
      if(buttonStates[2] == 1) {
        if(currentProfile == sizeof(profiles)/sizeof(profiles[0])) {
          currentProfile = 0;
        } else {
          currentProfile++;
        }
      }

    }
  } else {
    lcdChanged = 0;
  }

  // if(editing == 1 && )
  // Add a blinking cursor when editing

  byte outputValues[] = {screen, lcdChanged, currentProfile};
  return outputValues;
}

// Passes out array with current button states and whether they have recently changed
byte* checkButtons() {
  static byte previousButtonState[] = {0, 0, 0}; // L, C, R
  static byte currentButtonState[] = {0, 0, 0, 0};    // L, C, R, (changed?)

  // Filling out the array with the current button states
  for(byte i = 0; i < 3; i++) {
    currentButtonState[i] = digitalRead(buttons[i]);
  }

  // Checking to see if any have changed since the last call
  for(byte i = 0; i < 3; i++) {
    if(currentButtonState[i] != previousButtonState[i]) {
      currentButtonState[3] = 1;
      break;
    } else {
      currentButtonState[3] = 0;
    }
  }

  // Recording the new state as previous
  for(byte i = 0; i < 3; i++){
    previousButtonState[i] = currentButtonState[i];
  }

  return currentButtonState;
}

// check how long it's been since the last reading
// calculate the rainfall rate per "hour" based on this
float readRainfall() {
  float rainfallRate = 0;
  static byte previousState = 0;

  static unsigned long previousDump = 0;
  unsigned long currentMillis = millis();

  byte currentState = digitalRead(hall);

  // Calculate rate based on the time since last dump
  if(currentMillis - previousDump < hourLength && previousDump != 0){
    rainfallRate = bucketSize/(float(currentMillis - previousDump)/float(hourLength));
  } else {
    rainfallRate = 0;
  }

  // Takes the average of the last ten readings
  float avgRainfallRate = rainfallAverage(rainfallRate);

  // Checks if the bucket has dumped
  // This is done after the rate calculator to make sure that there's no zeros
  // in the denomonator.
  if(currentState != previousState) {
    previousDump = currentMillis;
  }

  previousState = currentState;
  return avgRainfallRate;
}

byte rainfallScreen() {
  return 0;
}

byte* motorScreen() {

  static byte outputValues[] = {0, 0, 0, 0};
  return outputValues;
}

void runMotor(byte motorDuty) {

}

byte calcIrrigation(float target, float rainfallRate) {
  return motorDuty; // Motor duty is a number between 0 and 100
}

float rainfallAverage(float M) {
  #define LM_SIZE 10
  static float LM[LM_SIZE];      // LastMeasurements
  static byte index = 0;
  static float sum = 0;
  static byte count = 0;

  // keep sum updated to improve speed.
  sum -= LM[index];
  LM[index] = M;
  sum += LM[index];
  index++;
  index = index % LM_SIZE;
  if (count < LM_SIZE) count++;

  return sum / count;
}
