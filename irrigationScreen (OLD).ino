// Displays the current amount of water being delivered to the crop per hour
byte* irrigationScreen(byte buttonStates[], byte lcdChanged, byte currentProfile, float irrigationLevel, unsigned int currentHour) {
  static byte editing = 0;
  byte screen = 3;          // Current screen

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

  if(editing == 1 && currentMillis - previousBlink >= blinkDelay){
    previousBlink = currentMillis;
    cursorState = !cursorState;
  } else if(editing == 0) {
    cursorState = 0;
  }

  lcd.setCursor(4, 1);
  if(cursorState == 1) {
    lcd.cursor();
  } else {
    lcd.noCursor();
  }

  // Display all necessary text
  if(lcdChanged == 1 || lcdRefresh == 1) {
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
    lcdChanged = 0;
  }

  // Checks if a button state has been changed
  if(buttonStates[3] == 1) {
    lcdChanged = 1;
    float currentIrrigation = getIrrigation(currentProfile, currentHour);
    // If not in editing mode, change the screen
    if(editing == 0) {
      // Check if left button is pressed
      if(buttonStates[0] == 1) {
        screen = 2;
      }
      // Check if center button is pressed
      if(buttonStates[1] == 1){
        editing = 1;
      }
      // Check if right button is pressed
      if(buttonStates[2] == 1) {
        screen = 4;
      }

    // If in editing mode, change the irrigation level
    } else if(editing == 1) {
      // Check if the left button is pressed
      if(buttonStates[0] == 1) {
        if(currentIrrigation > 0.05) {
          currentIrrigation -= 0.05;
        }
      }
      // Check if the center button is pressed
      if(buttonStates[1] == 1) {
        editing = 0;
      }
      // Check if the right button is pressed
      if(buttonStates[2] == 1) {
        currentIrrigation += 0.05;
      }

    }
    modifyIrrigation(currentProfile, currentHour, currentIrrigation);
  } else {
    lcdChanged = 0;
  }

  byte outputValues[] = {screen, lcdChanged};
  return outputValues;
}


// Finds the current irrigation level and replaces it
void modifyIrrigation(byte currentProfile, unsigned int currentHour, float irrigationLevel) {
  unsigned int currentDate = currentHour/24;

  bool sectionFound = 0;
  float stopDate = profiles[currentProfile][profileLen[currentProfile] - 2];

  // If the current date is past the end of the profile, end the program
  if(currentDate >= stopDate){
    return;
  } else {
    // Otherwise, search for the irrigation level for the current date range
    byte i = 0;
    while(!sectionFound) {
      if(currentDate >= profiles[currentProfile][2*i]) {
        sectionFound = 0;
        i++;
      } else {
        profiles[currentProfile][2*i-1] = irrigationLevel;
        sectionFound = 1;
      }
    }
  }

}
