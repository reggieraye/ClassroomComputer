#include <Wire.h>
#include "rgb_lcd.h"
#include "sort_program.h"
#include "primes_program.h"

// ══════════════════════════════════════════════════════════════════════════════
// HARDWARE
// ══════════════════════════════════════════════════════════════════════════════

rgb_lcd lcd;
const int POT_PIN    = A0;
const int BUZZER_PIN = 2;

// ── Backlight colours ─────────────────────────────────────────────────────────
const byte COL_PINK[3]  = {255,   0, 128};
const byte COL_GREEN[3] = {  0, 255,   0};

// ══════════════════════════════════════════════════════════════════════════════
// SHARED STATE & CONFIGURATION
// ══════════════════════════════════════════════════════════════════════════════

// ── Scroll speed: 1 (slowest) – 5 (fastest) ──────────────────────────────────
int scrollSpeed = 3;

// ── Delay before scrolling begins after entering a state ──────────────────────
const unsigned long SCROLL_START_DELAY = 750UL;

// ── Top-level application states ──────────────────────────────────────────────
enum AppState {
  APP_WELCOME,
  APP_PROGRAM_SELECT,
  APP_SORT_TEST,
  APP_PRIMES
};

AppState appState = APP_WELCOME;

// ── Timing ────────────────────────────────────────────────────────────────────
unsigned long stateEnteredAt = 0;   // millis() when current state began
unsigned long potLastMovedAt = 0;   // millis() of last pot movement
unsigned long scrollTickAt   = 0;   // millis() of next scroll step

// ── Potentiometer ─────────────────────────────────────────────────────────────
int  potValue         = 0;
int  potValuePrev     = -1;    // -1 = no previous reading (sentinel)
bool potHasMoved      = false; // has pot moved since entering current state?
int  remappedPotValue = 10;    // pot value mapped to [10, 350]

// ── Scroll state ──────────────────────────────────────────────────────────────
int scrollOffset = 0;   // leading-character index into the scroll string

// ── Celebratory animation frames (pulsing diamond) ────────────────────────────
// All three are written into custom-char slot 0 in turn each animation tick.
byte celebFrame0[8] = { 0b00100, 0b01110, 0b11111, 0b11111, 0b01110, 0b00100, 0b00000, 0b00000 };
byte celebFrame1[8] = { 0b00000, 0b00100, 0b01110, 0b11111, 0b01110, 0b00100, 0b00000, 0b00000 };
byte celebFrame2[8] = { 0b00000, 0b00000, 0b00100, 0b01110, 0b00100, 0b00000, 0b00000, 0b00000 };
int           celebFrameIdx = 0;
unsigned long celebTickAt   = 0;

// ── Micro (µ) symbol custom character ─────────────────────────────────────────
// Custom character slot 1 = µ (mu) for microseconds display
byte microChar[8] = { 0b00000, 0b01010, 0b01010, 0b01010, 0b01110, 0b01000, 0b01000, 0b00000 };

// ── Pot deadband – absorbs ADC noise ──────────────────────────────────────────
const int POT_DEADBAND = 8;

// ══════════════════════════════════════════════════════════════════════════════
// SHARED HELPERS
// ══════════════════════════════════════════════════════════════════════════════

// ── Scroll helper ─────────────────────────────────────────────────────────────
// Renders a 16-char window of str on the given LCD row, advancing one character
// per tick.  scrollSpeed 0 = 100 ms/step (fastest), 5 = 350 ms/step (slowest).
// Uses globals scrollOffset and scrollTickAt.
// loop=true (default): wraps back to the start after wrapGap blank columns.
// loop=false: scrolls once and holds on a blank screen when text is gone.
void tickScroll(const char* str, uint8_t row, unsigned long now, int wrapGap = 4, bool loop = true) {
  int len   = strlen(str);
  int cycle = len + wrapGap;

  if (now >= scrollTickAt) {
    if (loop) {
      scrollOffset = (scrollOffset + 1) % cycle;
    } else {
      if (scrollOffset < len) scrollOffset++;  // clamp: blank screen once text is gone
    }
    scrollTickAt = now + 100UL + (unsigned long)(6 - scrollSpeed) * 50UL;
  }

  lcd.setCursor(0, row);
  for (int i = 0; i < 16; i++) {
    int idx = scrollOffset + i;
    if (loop) {
      int pos = idx % cycle;
      lcd.write((pos < len) ? (uint8_t)str[pos] : (uint8_t)' ');
    } else {
      lcd.write((idx < len) ? (uint8_t)str[idx] : (uint8_t)' ');
    }
  }
}

// ── State-transition helper ──────────────────────────────────────────────────
// Common bookkeeping whenever we move to a new top-level state.
void enterAppState(int next) {
  appState       = (AppState)next;
  stateEnteredAt = millis();
  scrollOffset   = 0;
  scrollTickAt   = millis();
  potHasMoved    = false;
  lcd.setRGB(COL_PINK[0], COL_PINK[1], COL_PINK[2]);
  lcd.clear();

  // Reset program-specific states to their initial values
  if (next == APP_PRIMES) {
    enterPrimesState(PRIMES_TITLE);
  } else if (next == APP_SORT_TEST) {
    enterSortState(SORT_TITLE);
  }
}

// ══════════════════════════════════════════════════════════════════════════════
// SETUP & MAIN LOOP
// ══════════════════════════════════════════════════════════════════════════════

void setup() {
  Serial.begin(9600);

  lcd.begin(16, 2);
  lcd.setRGB(COL_PINK[0], COL_PINK[1], COL_PINK[2]);
  lcd.createChar(0, celebFrame0);  // slot 0 = animation frame (overwritten each tick)
  lcd.createChar(1, microChar);    // slot 1 = µ (micro) symbol

  pinMode(BUZZER_PIN, OUTPUT);

  // Seed potValuePrev so the first reading doesn't register as a spurious change
  potValuePrev = analogRead(POT_PIN);

  // Stamp the start time for the welcome state
  stateEnteredAt = millis();
  scrollTickAt   = millis();
}

void loop() {
  unsigned long now = millis();

  // ── Read pot and detect movement ──────────────────────────────────────────
  potValue = analogRead(POT_PIN);
  if (abs(potValue - potValuePrev) > POT_DEADBAND) {
    potLastMovedAt = now;
    potHasMoved    = true;
    potValuePrev   = potValue;
  }
  remappedPotValue = map(potValue, 0, 1023, 10, 350);

  // ── Dispatch to current state handler ────────────────────────────────────
  switch (appState) {
    case APP_WELCOME:        handleWelcome(now);       break;
    case APP_PROGRAM_SELECT: handleProgramSelect(now); break;
    case APP_SORT_TEST:      handleSortTest(now);      break;
    case APP_PRIMES:         handlePrimes(now);        break;
  }
}

// ══════════════════════════════════════════════════════════════════════════════
// ORCHESTRATOR SCREENS
// ══════════════════════════════════════════════════════════════════════════════

// ── handleWelcome ─────────────────────────────────────────────────────────────
// Layout: 0.75 s static, then 6 s scrolling (wrap gap 4). Total = 6.75 s.
void handleWelcome(unsigned long now) {
  const char* msg = "Welcome to the Classroom Computer!";

  if (now - stateEnteredAt < SCROLL_START_DELAY) {
    // Static display – show the first 16 characters before scrolling begins
    lcd.setCursor(0, 0);
    for (int i = 0; i < 16; i++) lcd.write((uint8_t)msg[i]);
  } else {
    tickScroll(msg, 0, now, 4, false);
  }

  lcd.setCursor(0, 1);
  lcd.print("(C) 2026 by R.R.");

  if (now - stateEnteredAt >= 6750UL) {
    enterAppState(APP_PROGRAM_SELECT);
  }
}

// ── handleProgramSelect ───────────────────────────────────────────────────────
void handleProgramSelect(unsigned long now) {
  const char* msg = "Use slider to select program";

  if (now - stateEnteredAt < SCROLL_START_DELAY) {
    lcd.setCursor(0, 0);
    for (int i = 0; i < 16; i++) lcd.write((uint8_t)msg[i]);
  } else {
    tickScroll(msg, 0, now, 4, true);
  }

  lcd.setCursor(0, 1);
  lcd.print("Sort | Primes   ");  // 16 chars padded to clear any leftover chars

  if (potHasMoved && (now - potLastMovedAt >= 500UL)) {
    if (potValue <= 383) {
      enterAppState(APP_SORT_TEST);
    } else {
      enterAppState(APP_PRIMES);
    }
  }
}
