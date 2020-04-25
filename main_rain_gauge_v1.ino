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
const unsigned long hourLength = 5000 // Length of an "hour" in milliseconds

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

  // Read rain gauge
  static int rainfallRate = 0;
  float* rainfallState[] = readRainfall(totalRainfall, previousBucketState);
  rainfallRate = rainfallState[0];
  previousBucketState = rainfallState[1];

  // Stores the buttons that were pressed from last loop
  static byte pressed[] = {0, 0, 0, 0};
  // check if any buttons were pressed
  byte* currentPressed = checkButtons(pressed);

  // Set up the LCD change tracker
  static byte lcdRefresh = 1;
  static byte lcdClear = 0;

  // Get current time and set up timers
  unsigned long currentMillis = millis();
  static unsigned long previousMotorM = 0;
  static unsigned long previousHour = 0;
  static unsigned int currentHour = 0;

  // Check if an hour has passed
  if(currentMillis - previousHour >= hourLength) {
    previousHour = currentMillis;
    currentHour++;
  }

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
  moveDuration = 60000 * (100 / calcIrrigation(, rainfall));
  // Motor movement block
  if(currentMillis - previousMotorM >= moveDuration) {

  }


}

byte* profileScreen(byte buttonStates[], byte lcdChanged, byte currentProfile) {
  static byte editing = 0;
  byte screen = 0;          // Current screen

  if(lcdChanged == 1) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("current profile");
    lcd.setCursor(0, 1);
    byte profileLen = sizeof(profiles[currentProfile]);
    lcd.print("<");
    lcd.setCursor(((16 - profileLen)/2 + 1), 1);
    lcd.print(profiles[currentProfile]);
    lcd.setCursor(15, 1);
    lcd.print(">");
    lcdChanged = 0;
  }

  if(buttonStates[4] == 1) { // Checks if a button state has been changed
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

byte* checkButtons(byte previousButtonState[]) {
  bool currentButtonState[i] = {0, 0, 0, 0};    // L, C, R, (changed?)

  for(byte i = 0; i < 3; i++) {
    currentButtonState[i] = digitalRead(buttons[i]);
  }

  for(byte i = 0; i < 3; i++) {
    if(currentButtonState[i] != previousButtonState[i]) {
      currentButtonState[4] = 1;
      break;
    }
    else if {
      currentButtonState[4] = 0;
    }
  }

  return currentButtonState;
}

float* readRainfall(float totalRainfall, byte previousState) {
  byte currentState = digitalRead(hall);

  if(currentState != previousState) {

  }

  // check how long it's been since the last reading
  // calculate the rainfall rate per "hour" based on this
}

byte rainfallScreen() {

}

byte* motorScreen() {

  static byte outputValues[] = {0, 0, 0, 0};
  return outputValues;
}

void runMotor(byte motorDuty) {

}

byte calcIrrigation(float target, float rainfall) {
  return motorDuty; // Motor duty is a number between
}
