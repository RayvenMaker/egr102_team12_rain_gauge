// Main rain gauge file to run on Arduino
// Authors: Reagan G, Andrew A

#include <LiquidCrystal.h>

const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void setup(){
  // RUN THE CODES ONCE
  Serial.begin(9600);

}


void loop(){
  // RUN THE CODES MANY TIMES

}
