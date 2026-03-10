#ifndef ASI_PROGRAM_H
#define ASI_PROGRAM_H

#include <Arduino.h>

// ── ASI Program states ────────────────────────────────────────────────────────
enum ASIState {
  ASI_DENIED  // Single state: "ACCESS DENIED!" for 3 seconds
};

// ── ASI-specific state ────────────────────────────────────────────────────────
static ASIState asiState = ASI_DENIED;
static bool warningBlipsPlayed = false;

// ── Forward declarations ──────────────────────────────────────────────────────
void enterASIState(ASIState next);
void handleASI(unsigned long now);

// ── External references (defined in main sketch) ──────────────────────────────
extern rgb_lcd lcd;
extern const byte COL_GREEN[3];
extern unsigned long stateEnteredAt;
extern const int BUZZER_PIN;
extern void enterAppState(int nextState);

// ── Implementations ───────────────────────────────────────────────────────────

void enterASIState(ASIState next) {
  asiState = next;
  stateEnteredAt = millis();
  warningBlipsPlayed = false;

  // Green screen for denied state
  lcd.setRGB(COL_GREEN[0], COL_GREEN[1], COL_GREEN[2]);
  lcd.clear();
}

void handleASI(unsigned long now) {
  unsigned long elapsed = now - stateEnteredAt;

  // Display ACCESS DENIED message
  lcd.setCursor(0, 0);
  lcd.print("ACCESS DENIED!  ");

  // Play warning earcon (4 quick blips) once at start
  if (!warningBlipsPlayed) {
    for (int i = 0; i < 4; i++) {
      tone(BUZZER_PIN, 1000, 80);  // 1kHz blip, 80ms duration
      delay(100);                   // 100ms between blips
    }
    warningBlipsPlayed = true;
  }

  // Return to program select after 3 seconds
  if (elapsed >= 3000UL) {
    enterAppState(1);  // APP_PROGRAM_SELECT
  }
}

#endif
