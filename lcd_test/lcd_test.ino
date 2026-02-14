#include <Wire.h>
#include "rgb_lcd.h"

rgb_lcd lcd;

#define BUZZER_PIN 2

// Custom character: heart
byte heart[8] = {
  0b00000,
  0b01010,
  0b11111,
  0b11111,
  0b11111,
  0b01110,
  0b00100,
  0b00000
};

// Custom character: smiley
byte smiley[8] = {
  0b00000,
  0b01010,
  0b01010,
  0b00000,
  0b10001,
  0b01110,
  0b00000,
  0b00000
};

// Custom character: note
byte note[8] = {
  0b00100,
  0b00110,
  0b00101,
  0b00101,
  0b00100,
  0b11100,
  0b11100,
  0b00000
};

// Each entry: { red, green, blue, label }
struct ColorStep {
  byte r, g, b;
  const char* label;
};

const ColorStep colors[] = {
  {255,   0,   0, "Red"},
  {255, 128,   0, "Orange"},
  {255, 255,   0, "Yellow"},
  {  0, 255,   0, "Green"},
  {  0, 128, 255, "Cyan"},
  {  0,   0, 255, "Blue"},
  {128,   0, 255, "Violet"},
  {255,   0, 128, "Pink"},
  { 40,  40,  40, "Dim White"},
  {255, 255, 255, "Bright White"},
};
const int numColors = sizeof(colors) / sizeof(colors[0]);

const int potPin = A0;
int colorIndex = 0;

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.createChar(0, heart);
  lcd.createChar(1, smiley);
  lcd.createChar(2, note);

  // Startup buzzer sequence (ascending pitches)
  pinMode(BUZZER_PIN, OUTPUT);
  tone(BUZZER_PIN, 400, 150);
  delay(200);
  tone(BUZZER_PIN, 600, 150);
  delay(200);
  tone(BUZZER_PIN, 800, 150);
  delay(200);
  noTone(BUZZER_PIN);
}

void loop() {
  const ColorStep& c = colors[colorIndex];

  lcd.clear();
  lcd.setRGB(c.r, c.g, c.b);

  // Line 1: color name framed by special characters
  lcd.setCursor(0, 0);
  lcd.write((byte)0);              // heart
  lcd.print(" ");
  lcd.print(c.label);

  // Pad and end with a note symbol
  int pad = 14 - strlen(c.label);
  for (int i = 0; i < pad; i++) lcd.print(" ");
  lcd.write((byte)2);              // note

  // Line 2: step counter + smiley
  lcd.setCursor(0, 1);
  lcd.write((byte)1);              // smiley
  lcd.print(" Color ");
  lcd.print(colorIndex + 1);
  lcd.print("/");
  lcd.print(numColors);
  lcd.print("  ");
  lcd.write((byte)0);              // heart

  int potValue = analogRead(potPin);
  Serial.println(potValue);

  colorIndex = (colorIndex + 1) % numColors;
  delay(1500);
}
