// Main rain gauge file to run on Arduino
// Authors: Reagan G, Andrew A
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
const float soybeans[] = {1, 0.5, 100, 1.5, 150, 1.0, 250, 0.5};

void setup(){
  // RUN THE CODES ONCE
  Serial.begin(9600);
  lcd.begin(16, 2);

  lcd.print("Irrigator v0.1");
  lcd.setCursor(0, 1);
  lcd.print("loading...");
  delay(2000);
  lcd.clear();
}


void loop(){
  // RUN THE CODES MANY TIMES

  /* program structure:
  Starting up:
    - Gets the current "time and date"
    - Shows a loading screen.
  Program selection menu: (optional)
    - Reads off the names of the crop programs
    - Display these on the LCD screen.
    -
  Running mode:
    -
  Manual move mode:
    -
  */
}
