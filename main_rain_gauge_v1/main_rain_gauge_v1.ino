// Main rain gauge file to run on Arduino
// Authors: Reagan G, Andrew S
#include <LiquidCrystal.h>

#define cw_motor 8
#define ccw_motor 9
#define l_button 12
#define c_button 11
#define r_button 10
#define valve 13
#define hall A0

const int rs = 7, en = 6, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// These are the profiles for the different crops.
// The first number in a pair is the day from start to change once
// The second number in a pair is the rainfall in inches per day that the crop
// will recieve until the next change
float corn[] = {0, 0.5, 100, 1.5, 150, 1.0, 250, 0.5};
float wheat[] = {0, 0.6, 110, 1.4, 140, 1.1, 240, 0.4};
float rice[] = {0, 0.4, 90, 1.6, 150, 0.9, 260, 0.6};
float *profiles[] = {corn, wheat, rice};
byte profileLen[] = {8, 8, 8};
const char *profileNames[] = {"corn", "wheat", "rice"};
const char *screens[] = {"current profile", "rainfall (in/s)", "motor duty %", "irrigation(in/s)", "days since start"};
const byte buttons[] = {l_button, c_button, r_button};

const float bucketSize = 0.01;    // Size of each bucket drop in inches of rain
const unsigned long hourLength = 5000; // Length of an "hour" in milliseconds
const unsigned long rainfallCheckInterval = 100; // Time between rainfall checks
const unsigned long summaryInterval = 10000; // Time between serial print summaries
const float maxFlow = 2.0;  // Assuming that the maximum water output is 2.0in/h

byte screen = 0;
byte currentProfile = 0;

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
  pinMode(cw_motor, OUTPUT);
  pinMode(ccw_motor, OUTPUT);
  pinMode(valve, OUTPUT);
  pinMode(l_button, INPUT);
  pinMode(c_button, INPUT);
  pinMode(r_button, INPUT);
  pinMode(hall, INPUT);

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
  static unsigned long previousSummary = 0;
  static unsigned int currentHour = 0;
  static float rainfallRate = 0;

  // Check if an hour has passed
  if(currentMillis - previousHour >= hourLength) {
    previousHour = currentMillis;
    currentHour++;
  }

  rainfallRate = readRainfall();

  float irrigationLevel = getIrrigation(currentProfile, currentHour);

  // Calculate motor duty
  byte motorDuty = calcMotorDuty(irrigationLevel, rainfallRate);
  byte motorOverride = 0;   // 0 = no override, 1 = ccw, 2 = cw

  // check if any buttons were pressed
  byte* currentPressed = checkButtons();

  // Set up the LCD change tracker
  static byte lcdRefresh = 1;

  // Display management block
  switch(screen) {
    case 0: {
      // Shows current profile
      byte* profileState = profileScreen(currentPressed, lcdRefresh, currentProfile);
      screen = profileState[0];
      lcdRefresh = profileState[1];
      currentProfile = profileState[2];
      //Serial.print("Profile");
      }break;
    case 1: {
      // Shows current rainfall
      screen = rainfallScreen(currentPressed, rainfallRate);
      //Serial.print("Rainfall");
      }break;
    case 2: {
      // Shows current motor speed
      byte* motorState = motorScreen(currentPressed, lcdRefresh, motorDuty);
      screen = motorState[0];
      lcdRefresh = motorState[1];
      motorOverride = motorState[2];
      //Serial.print("Motor");
      }break;
    case 3: {
      // Shows current irrigation level
      screen = irrigationScreen(currentPressed, irrigationLevel);
      //Serial.print("Irrigation");
      }break;
    case 4: {
      // Shows current day in program
      screen = dateScreen(currentPressed, currentHour);
      //Serial.print("Date");
      }break;
    default: {
      screen = 0;
      //Serial.print("exeception");
      }break;
  }

  // Print out a summary to the serial monitor
  if(currentMillis - previousSummary >= summaryInterval){
    Serial.print("hour = ");
    Serial.print(currentHour);
    Serial.print(", rain = ");
    Serial.print(rainfallRate);
    Serial.print("(in/h), motor = ");
    Serial.print(motorDuty);
    Serial.print("%, water = ");
    Serial.print(irrigationLevel);
    Serial.println("(in/h)");
    previousSummary = currentMillis;
  }

  // Lastly, move the motor
  runMotor(motorDuty, motorOverride);

  delay(10);
}

// Manages the LCD display and the customizable functions
byte* profileScreen(byte buttonStates[], byte lcdChanged, byte currentProfile) {
  static byte editing = 0;
  static byte screen = 0;          // Current screen

  // Add a blinking cursor when editing
  int blinkDelay = 1000;
  static bool cursorState = 0;
  unsigned long currentMillis = millis();
  static unsigned long previousBlink = 0;

  if(editing == 1 && currentMillis - previousBlink >= blinkDelay){
    previousBlink = currentMillis;
    cursorState = !cursorState;
  } else if(editing == 0) {
    cursorState = 0;
  }

  int refreshInterval = 100;
  static bool lcdRefresh = 0;
  static unsigned long previousRefresh = 0;
  if(currentMillis - previousRefresh >= refreshInterval) {
    lcdRefresh = !lcdRefresh;
  }

  byte profileLen = strlen(profileNames[currentProfile]);
  lcd.setCursor(((16 - profileLen)/2), 1);
  if(cursorState == 1) {
    lcd.cursor();
  } else {
    lcd.noCursor();
  }

  // Display all necessary text
  if(lcdChanged == 1 || lcdRefresh == 1) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("current profile");
    lcd.setCursor(0, 1);
    lcd.print("<");
    lcd.setCursor(((16 - profileLen)/2), 1);
    lcd.print(profileNames[currentProfile]);
    lcd.setCursor(15, 1);
    lcd.print(">");
    lcdChanged = 0;
  }

  if(buttonStates[3] == 1) { // Checks if a button state has been changed
    lcdChanged = 1;
    if(editing == 0) {

      if(buttonStates[0] == 1) {
        screen = 4;
      }

      if(buttonStates[1] == 1){
        editing = 1;
      }

      if(buttonStates[2] == 1) {
        screen = 1;
      }

    } else if(editing == 1) {
      // Check if the left button is pressed
      if(buttonStates[0] == 1) {
        if(currentProfile == 0) {
          currentProfile = sizeof(profileNames)/sizeof(profileNames[0]) - 1;
        } else {
          currentProfile--;
        }
      }
      // Check if the center button is pressed
      if(buttonStates[1] == 1) {
        editing = 0;
      }
      // Check if the right button is pressed
      if(buttonStates[2] == 1) {
        if(currentProfile >= sizeof(profileNames)/sizeof(profileNames[0]) - 1) {
          currentProfile = 0;
        } else {
          currentProfile++;
        }
      }

    }
  } else {
    lcdChanged = 0;
  }

  byte outputValues[] = {screen, lcdChanged, currentProfile};
  return outputValues;
}

byte* motorScreen(byte buttonStates[], byte lcdChanged, byte currentSpeed) {
  static byte editing = 0;
  byte screen = 2;          // Current screen
  static byte motorOverride = 0;

  int refreshInterval = 100;
  unsigned long currentMillis = millis();
  static bool lcdRefresh = 0;
  static unsigned long previousRefresh = 0;
  if(currentMillis - previousRefresh >= refreshInterval) {
    lcdRefresh = !lcdRefresh;
  }

  // Add a blinking cursor when editing
  int blinkDelay = 500;
  static bool cursorState = 0;
  static unsigned long previousBlink = 0;

  if(editing == 1 && previousBlink - currentMillis >= blinkDelay){
    previousBlink = currentMillis;
    cursorState = !cursorState;
  } else if(editing == 0) {
    cursorState = 0;
  }

  if(motorOverride == 1){
    lcd.setCursor(0, 1);
  } else if(motorOverride == 2){
    lcd.setCursor(15, 1);
  } else {
    lcd.setCursor(8, 1);
  }

  if(cursorState == 1) {
    lcd.cursor();
  } else {
    lcd.noCursor();
  }

  // Display all necessary text
  if((lcdChanged == 1 || lcdRefresh == 1) && editing == 0) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("pivot duty cycle");
    lcd.setCursor(0, 1);
    lcd.print("<");
    lcd.setCursor(6, 1);
    lcd.print(currentSpeed);
    lcd.print("%");
    lcd.setCursor(15, 1);
    lcd.print(">");
    lcdChanged = 0;
  } else if((lcdChanged == 1 || lcdRefresh == 1) && editing == 1){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("manual move");
    lcd.setCursor(0, 1);
    lcd.print("< ccw");
    lcd.setCursor(12, 1);
    lcd.print("cw >");
    lcdChanged = 0;
  }

  if(buttonStates[3] == 1) { // Checks if a button state has been changed
    lcdChanged = 1;
    motorOverride = 0;
    if(editing == 0) {

      if(buttonStates[0] == 1) {
        screen = 1;
      }

      if(buttonStates[1] == 1){
        editing = 1;
      }

      if(buttonStates[2] == 1) {
        screen = 3;
      }

    } else if(editing == 1) {
      // Check if the left button is pressed
      if(buttonStates[0] == 1) {
        motorOverride = 1;
      }
      // Check if the center button is pressed
      if(buttonStates[1] == 1) {
        editing = 0;
      }
      // Check if the right button is pressed
      if(buttonStates[2] == 1) {
        motorOverride = 2;
      }

    }
  } else {
    lcdChanged = 0;
  }

  byte outputValues[] = {screen, lcdChanged, motorOverride};
  return outputValues;
}

// Displays the current amount of water being delivered to the crop per hour
byte irrigationScreen(byte buttonStates[], float irrigationLevel) {
  byte screen = 3;          // Current screen

  int refreshInterval = 100;
  unsigned long currentMillis = millis();
  static bool lcdRefresh = 0;
  static unsigned long previousRefresh = 0;
  if(currentMillis - previousRefresh >= refreshInterval) {
    lcdRefresh = !lcdRefresh;
  }

  // Display all necessary text
  if(lcdRefresh == 1) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("water delivered");
    lcd.setCursor(0, 1);
    lcd.print("<");
    lcd.setCursor(4, 1);
    lcd.print(irrigationLevel);
    lcd.setCursor(9, 1);
    lcd.print("in/h");
    lcd.setCursor(15, 1);
    lcd.print(">");
  }

  // Checks if a button state has been changed
  if(buttonStates[3] == 1) {
    // Check if left button is pressed
    if(buttonStates[0] == 1) {
      screen = 2;
    }
    // Check if right button is pressed
    if(buttonStates[2] == 1) {
      screen = 4;
    }
  } else {
    screen = 3;
  }

  return screen;
}

// Displays the current amount of rainfall in inches per hour
byte rainfallScreen(byte buttonStates[], float currentRainfall) {
  byte screen = 1;          // Current screen

  int refreshInterval = 100;
  static bool lcdRefresh = 0;
  unsigned long currentMillis = millis();
  static unsigned long previousRefresh = 0;
  if(currentMillis - previousRefresh >= refreshInterval) {
    lcdRefresh = !lcdRefresh;
  }

  // Display all necessary text
  if (lcdRefresh == 1){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("rainfall (in/h)");
    lcd.setCursor(0, 1);
    lcd.print("<");
    lcd.setCursor(6, 1);
    lcd.print(currentRainfall);
    lcd.setCursor(15, 1);
    lcd.print(">");
  }

  if(buttonStates[3] == 1) { // Checks if a button state has been changed

    if(buttonStates[0] == 1) {
      screen = 0;
    }

    if(buttonStates[2] == 1) {
      screen = 2;
    }

  } else {
    screen = 1;
  }

  return screen;
}

// Displays the current day since the program was started
byte dateScreen(byte buttonStates[], unsigned int currentHour) {
  byte screen = 4;          // Current screen

  // Switch between showing the current date and current hour
  unsigned int switchDelay = 3000;
  static bool switchState = 0;
  unsigned long currentMillis = millis();
  static unsigned long previousSwitch = 0;

  int refreshInterval = 100;
  static bool lcdRefresh = 0;
  static unsigned long previousRefresh = 0;
  if(currentMillis - previousRefresh >= refreshInterval) {
    lcdRefresh = !lcdRefresh;
  }

  if(currentMillis - previousSwitch >= switchDelay){
    previousSwitch = currentMillis;
    switchState = !switchState;
    lcd.clear();
  }

  // Display all necessary text
  if(lcdRefresh == 1){
    if(switchState == 0){
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("days since start");
      lcd.setCursor(0, 1);
      lcd.print("<");
      lcd.setCursor(15, 1);
      lcd.print(">");
      lcd.setCursor(6, 1);
      lcd.print(currentHour/24);
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("current hour");
      lcd.setCursor(0, 1);
      lcd.print("<");
      lcd.setCursor(15, 1);
      lcd.print(">");
      lcd.setCursor(6, 1);
      lcd.print(currentHour);
    }
  }

  if(buttonStates[3] == 1) { // Checks if a button state has been changed

    if(buttonStates[0] == 1) {
      screen = 3;
    }

    if(buttonStates[2] == 1) {
      screen = 0;
    }

  }

  return screen;
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


// Finds the current irrigation level based on the profile selected
float getIrrigation(byte currentProfile, unsigned int currentHour) {
  unsigned int currentDate = currentHour/24;

  bool sectionFound = 0;
  float stopDate = profiles[currentProfile][profileLen[currentProfile] - 2];
  float irrigationLevel = 0;

  // If the current date is past the end of the profile, end the program
  if(currentDate >= stopDate){
    irrigationLevel = 0;
  } else {
    // Otherwise, search for the irrigation level for the current date range
    byte i = 0;
    while(!sectionFound) {
      if(currentDate >= profiles[currentProfile][2*i]) {
        sectionFound = 0;
        i++;
      } else {
        irrigationLevel = profiles[currentProfile][2*i-1];
        sectionFound = 1;
      }
    }
  }

  return irrigationLevel;
}


// Calculate the motor duty cycle given the rainfall and target irrigation
byte calcMotorDuty(float irrigationLevel, float rainfallRate) {
  float targetIrrigation = irrigationLevel - rainfallRate;
  byte motorDuty = 0;

  // Calculate the motor duty for the target irrigation
  if(targetIrrigation <= maxFlow && targetIrrigation > 0){
    motorDuty = (targetIrrigation - 0) * (1 - 100) / (maxFlow - 0) + 100;
  }

  return motorDuty; // Motor duty is a number between 0 and 100
}

// Uses the given motor duty to run the motor, and decides whether to open the valve
void runMotor(byte motorDuty, byte motorOverride) {
  // If the motor duty is 0%, then just shut off the motor and valve
  // Otherwise, run the motor and turn on the valve.
  unsigned long currentMillis = millis();
  static unsigned long previousMillis = 0;
  static bool moving = 0;

  if(motorDuty > 0){
    unsigned long moveInterval = map(motorDuty, 0, 100, 0, hourLength);
    unsigned long stopInterval = hourLength - moveInterval;

    if(moving == 1){
      if(currentMillis - previousMillis >= moveInterval){
        previousMillis = currentMillis;
        digitalWrite(cw_motor, LOW);
        digitalWrite(ccw_motor, LOW);
        moving = 0;
      }
    } else {
      if(currentMillis - previousMillis >= stopInterval){
        previousMillis = currentMillis;
        digitalWrite(cw_motor, HIGH);
        digitalWrite(ccw_motor, LOW);
        moving = 1;
      }
    }
    digitalWrite(valve, HIGH);

  } else {
    digitalWrite(cw_motor, LOW);
    digitalWrite(ccw_motor, LOW);
    digitalWrite(valve, LOW);
  }

  if(motorOverride == 1){
    digitalWrite(cw_motor, HIGH);
    digitalWrite(ccw_motor, LOW);
    digitalWrite(valve, HIGH);
  } else if(motorOverride == 2){
    digitalWrite(cw_motor, LOW);
    digitalWrite(ccw_motor, HIGH);
    digitalWrite(valve, HIGH);
  }

}
