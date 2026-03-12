#ifndef ASI_PROGRAM_H
#define ASI_PROGRAM_H

#include <Arduino.h>

// ── ASI Program states ────────────────────────────────────────────────────────
enum ASIState {
  ASI_WELCOME_1,   // "Welcome," / "Reginald Raye" – 2 s, welcome jingle 0.4 s in
  ASI_WELCOME_2,   // "Welcome," / scroll CitizenID – 0.5 s, scroll immediately
  ASI_MORALITY_1,  // "Your Morality" / "Quotient YTD is" – 1.7 s
  ASI_MORALITY_2,  // "72.4th" / "percentile" – 1.5 s
  ASI_IMPROVE,     // "Please improve" / "MQ by 15.2%" – 1.5 s
  ASI_CULL,        // "To survive the" / "next cull, 3/19" – 1.5 s
  ASI_THANKS,      // "Thank you and" / "have a great day" – 3.0 s, done jingle 0.4 s in
  ASI_BLACKOUT     // black backlight, blank display – 0.5 s, then program select
};

// ── ASI-specific state ────────────────────────────────────────────────────────
static ASIState asiState = ASI_WELCOME_1;

// ── Forward declarations ──────────────────────────────────────────────────────
void enterASIState(ASIState next);
void handleASI(unsigned long now);
static void tickASIWelcomeJingle(unsigned long now);
static void tickASIDoneJingle(unsigned long now);

// ── External references (defined in main sketch) ──────────────────────────────
extern rgb_lcd lcd;
extern const byte COL_PINK[3];
extern unsigned long stateEnteredAt;
extern int scrollOffset;
extern unsigned long scrollTickAt;
extern const int BUZZER_PIN;
extern void enterAppState(int nextState);
extern void tickScroll(const char* str, uint8_t row, unsigned long now, int wrapGap, bool loop);

// ── Implementations ───────────────────────────────────────────────────────────

void enterASIState(ASIState next) {
  asiState       = next;
  stateEnteredAt = millis();
  scrollOffset   = 0;
  scrollTickAt   = millis();

  if (next == ASI_BLACKOUT) {
    lcd.setRGB(0, 0, 0);
  } else {
    lcd.setRGB(COL_PINK[0], COL_PINK[1], COL_PINK[2]);
  }
  lcd.clear();
}

// Non-blocking welcome jingle: C5 → E5 → G5, fires starting at 0.4 s.
static void tickASIWelcomeJingle(unsigned long now) {
  static unsigned long lastStateStart = 0;
  static int noteIdx = -1;
  if (lastStateStart != stateEnteredAt) { lastStateStart = stateEnteredAt; noteIdx = -1; }

  unsigned long e = now - stateEnteredAt;
  if      (noteIdx < 0  && e >= 400UL) { tone(BUZZER_PIN, 523, 100); noteIdx = 0; }  // C5
  else if (noteIdx == 0 && e >= 550UL) { tone(BUZZER_PIN, 659, 100); noteIdx = 1; }  // E5
  else if (noteIdx == 1 && e >= 700UL) { tone(BUZZER_PIN, 784, 150); noteIdx = 2; }  // G5
}

// Non-blocking 3D-printer-done jingle: ascending fanfare, fires starting at 0.4 s.
// C5 E5 G5 C6 E6 G6 – last note ends ~1.65 s after state entry.
static void tickASIDoneJingle(unsigned long now) {
  static unsigned long lastStateStart = 0;
  static int noteIdx = -1;
  if (lastStateStart != stateEnteredAt) { lastStateStart = stateEnteredAt; noteIdx = -1; }

  unsigned long e = now - stateEnteredAt;
  if      (noteIdx < 0  && e >=  400UL) { tone(BUZZER_PIN,  523, 100); noteIdx = 0; }  // C5
  else if (noteIdx == 0 && e >=  550UL) { tone(BUZZER_PIN,  659, 100); noteIdx = 1; }  // E5
  else if (noteIdx == 1 && e >=  700UL) { tone(BUZZER_PIN,  784, 100); noteIdx = 2; }  // G5
  else if (noteIdx == 2 && e >=  850UL) { tone(BUZZER_PIN, 1047, 200); noteIdx = 3; }  // C6
  else if (noteIdx == 3 && e >= 1100UL) { tone(BUZZER_PIN, 1319, 100); noteIdx = 4; }  // E6
  else if (noteIdx == 4 && e >= 1250UL) { tone(BUZZER_PIN, 1568, 400); noteIdx = 5; }  // G6
}

void handleASI(unsigned long now) {
  unsigned long elapsed = now - stateEnteredAt;

  switch (asiState) {

    case ASI_WELCOME_1:
      lcd.setCursor(0, 0); lcd.print("Welcome,        ");
      lcd.setCursor(0, 1); lcd.print("Reginald Raye   ");
      tickASIWelcomeJingle(now);
      if (elapsed >= 2000UL) enterASIState(ASI_WELCOME_2);
      break;

    case ASI_WELCOME_2:
      lcd.setCursor(0, 0); lcd.print("Welcome,        ");
      tickScroll("Reginald Raye CitizenID 2718281828", 1, now, 4, false);
      if (elapsed >= 500UL) enterASIState(ASI_MORALITY_1);
      break;

    case ASI_MORALITY_1:
      lcd.setCursor(0, 0); lcd.print("Your Morality   ");
      lcd.setCursor(0, 1); lcd.print("Quotient YTD is ");
      if (elapsed >= 1700UL) enterASIState(ASI_MORALITY_2);
      break;

    case ASI_MORALITY_2:
      lcd.setCursor(0, 0); lcd.print("72.4th          ");
      lcd.setCursor(0, 1); lcd.print("percentile      ");
      if (elapsed >= 1500UL) enterASIState(ASI_IMPROVE);
      break;

    case ASI_IMPROVE:
      lcd.setCursor(0, 0); lcd.print("Please improve  ");
      lcd.setCursor(0, 1); lcd.print("MQ by 15.2%     ");
      if (elapsed >= 1500UL) enterASIState(ASI_CULL);
      break;

    case ASI_CULL:
      lcd.setCursor(0, 0); lcd.print("To survive the  ");
      lcd.setCursor(0, 1); lcd.print("next cull, 3/19 ");
      if (elapsed >= 1500UL) enterASIState(ASI_THANKS);
      break;

    case ASI_THANKS:
      lcd.setCursor(0, 0); lcd.print("Thank you and   ");
      lcd.setCursor(0, 1); lcd.print("have a great day");
      tickASIDoneJingle(now);
      if (elapsed >= 3000UL) enterASIState(ASI_BLACKOUT);
      break;

    case ASI_BLACKOUT:
      // Backlight already set to (0,0,0) and display cleared in enterASIState.
      if (elapsed >= 500UL) enterAppState(1);  // APP_PROGRAM_SELECT
      break;
  }
}

#endif
