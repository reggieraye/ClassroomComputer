#include <Wire.h>
#include "rgb_lcd.h"

rgb_lcd lcd;

void setup() {
  lcd.begin(16, 2);
  lcd.setRGB(0, 255, 0);
  lcd.print("Hello, World!");
}

void loop() {
}
