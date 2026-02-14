#include <Wire.h>
#include "rgb_lcd.h"

// ── Hardware ──────────────────────────────────────────────────────────────────
rgb_lcd lcd;
const int POT_PIN    = A0;
const int BUZZER_PIN = 2;

// ── Backlight colours ─────────────────────────────────────────────────────────
const byte COL_PINK[3]  = {255,   0, 128};
const byte COL_GREEN[3] = {  0, 255,   0};

// ── Scroll speed: 0 (fastest) – 5 (slowest) ──────────────────────────────────
int scrollSpeed = 2;

// ── Programs ──────────────────────────────────────────────────────────────────
enum Program {
  PROG_SORT_TEST,
  PROG_PRIMES
};

// ── Top-level application states ──────────────────────────────────────────────
enum AppState {
  APP_WELCOME,
  APP_PROGRAM_SELECT,
  APP_SORT_TEST,
  APP_PRIMES
};

// ── Sort Test program states ───────────────────────────────────────────────────
enum SortTestState {
  SORT_TITLE,        // "Sort Test" for 0.75 s
  SORT_QUESTION,     // "Bubble or merge: / which is faster?" for 1.5 s
  SORT_SELECT_SIZE,  // "Move slider to select problem size"
  SORT_SHOW_N,       // "N = [n]" until slider static for 0.75 s
  SORT_CONFIRM_N,    // "Starting sort for / N = [n]" for 1 s
  SORT_RUNNING,      // "Bubble = TBD ms… / Merge = TBD ms…"
  SORT_RESULTS,      // results for 2.5 s
  SORT_WINNER        // "Merge sort is / the winner! X" for 2 s
};

// ── Primes program states (placeholder) ───────────────────────────────────────
enum PrimesState {
  PRIMES_IDLE
};

// ── Runtime state ─────────────────────────────────────────────────────────────
AppState      appState    = APP_WELCOME;
SortTestState sortState   = SORT_TITLE;
PrimesState   primesState = PRIMES_IDLE;
Program       activeProgram;          // set on selection

// ── Timing ────────────────────────────────────────────────────────────────────
unsigned long stateEnteredAt = 0;   // millis() when current state began
unsigned long potLastMovedAt = 0;   // millis() of last pot movement
unsigned long scrollTickAt   = 0;   // millis() of next scroll step

// ── Potentiometer ─────────────────────────────────────────────────────────────
int  potValue         = 0;
int  potValuePrev     = -1;    // -1 = no previous reading (sentinel)
bool potHasMoved      = false; // has pot moved since entering current state?
int  remappedPotValue = 10;    // pot value mapped to [10, 500]
int  confirmedN       = 10;    // N locked in when leaving SORT_SHOW_N

// ── Sort durations ────────────────────────────────────────────────────────────
unsigned long bubbleDuration = 0;  // ms
unsigned long mergeDuration  = 0;  // ms

// ── Scroll state ──────────────────────────────────────────────────────────────
int scrollOffset = 0;   // leading-character index into the scroll string

// ── Celebratory animation frames (pulsing diamond, for SORT_WINNER) ───────────
// All three are written into custom-char slot 0 in turn each animation tick.
byte celebFrame0[8] = { 0b00100, 0b01110, 0b11111, 0b11111, 0b01110, 0b00100, 0b00000, 0b00000 };
byte celebFrame1[8] = { 0b00000, 0b00100, 0b01110, 0b11111, 0b01110, 0b00100, 0b00000, 0b00000 };
byte celebFrame2[8] = { 0b00000, 0b00000, 0b00100, 0b01110, 0b00100, 0b00000, 0b00000, 0b00000 };
int           celebFrameIdx = 0;
unsigned long celebTickAt   = 0;

// ─────────────────────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(9600);

  lcd.begin(16, 2);
  lcd.setRGB(COL_PINK[0], COL_PINK[1], COL_PINK[2]);
  lcd.createChar(0, celebFrame0);  // slot 0 = animation frame (overwritten each tick)

  pinMode(BUZZER_PIN, OUTPUT);

  // Seed potValuePrev so the first reading doesn't register as a spurious change
  potValuePrev = analogRead(POT_PIN);

  // Stamp the start time for the welcome state
  stateEnteredAt = millis();
  scrollTickAt   = millis();
}

// ── Pot deadband – absorbs ADC noise ──────────────────────────────────────────
const int POT_DEADBAND = 8;

// ── State-transition helpers ──────────────────────────────────────────────────
// Common bookkeeping whenever we move to a new top-level or sort state.

void enterAppState(AppState next) {
  appState       = next;
  stateEnteredAt = millis();
  scrollOffset   = 0;
  scrollTickAt   = millis();
  potHasMoved    = false;
  lcd.clear();
}

void enterSortState(SortTestState next) {
  sortState      = next;
  stateEnteredAt = millis();
  scrollOffset   = 0;
  scrollTickAt   = millis();
  potHasMoved    = false;
  lcd.clear();
}

// ── Handler forward declarations ──────────────────────────────────────────────
void handleWelcome(unsigned long now);
void handleProgramSelect(unsigned long now);
void handleSortTest(unsigned long now);
void handlePrimes(unsigned long now);

// ─────────────────────────────────────────────────────────────────────────────

void loop() {
  unsigned long now = millis();

  // ── Read pot and detect movement ──────────────────────────────────────────
  potValue = analogRead(POT_PIN);
  if (abs(potValue - potValuePrev) > POT_DEADBAND) {
    potLastMovedAt = now;
    potHasMoved    = true;
    potValuePrev   = potValue;
  }
  remappedPotValue = map(potValue, 0, 1023, 10, 500);

  // ── Dispatch to current state handler ────────────────────────────────────
  switch (appState) {
    case APP_WELCOME:        handleWelcome(now);       break;
    case APP_PROGRAM_SELECT: handleProgramSelect(now); break;
    case APP_SORT_TEST:      handleSortTest(now);      break;
    case APP_PRIMES:         handlePrimes(now);        break;
  }
}

// ── Scroll helper ─────────────────────────────────────────────────────────────
// Renders a 16-char window of str on the given LCD row, advancing one character
// per tick.  scrollSpeed 0 = 100 ms/step (fastest), 5 = 350 ms/step (slowest).
// Uses globals scrollOffset and scrollTickAt.
void tickScroll(const char* str, uint8_t row, unsigned long now) {
  int len   = strlen(str);
  int cycle = len + 16;     // string length + 16-char blank gap before wrap

  if (now >= scrollTickAt) {
    scrollOffset = (scrollOffset + 1) % cycle;
    scrollTickAt = now + 100UL + (unsigned long)scrollSpeed * 50UL;
  }

  lcd.setCursor(0, row);
  for (int i = 0; i < 16; i++) {
    int idx = scrollOffset + i;
    lcd.write((idx < len) ? (uint8_t)str[idx] : (uint8_t)' ');
  }
}

// ── handleWelcome ─────────────────────────────────────────────────────────────
void handleWelcome(unsigned long now) {
  tickScroll("Welcome to the Classroom Computer!", 0, now);

  lcd.setCursor(0, 1);
  lcd.print("(C) 2026 by R.R.");

  if (now - stateEnteredAt >= 2500UL) {
    enterAppState(APP_PROGRAM_SELECT);
  }
}

// ── Remaining handlers (to be implemented) ────────────────────────────────────
// ── handleProgramSelect ───────────────────────────────────────────────────────
void handleProgramSelect(unsigned long now) {
  tickScroll("Use the slider to select the program you wish to run", 0, now);

  lcd.setCursor(0, 1);
  lcd.print("Sort | Primes   ");  // 16 chars padded to clear any leftover chars

  if (potHasMoved && (now - potLastMovedAt >= 500UL)) {
    if (potValue <= 600) {
      sortState = SORT_TITLE;
      enterAppState(APP_SORT_TEST);
    }
    // 601-1023: Primes – do nothing for now
  }
}
void handleSortTest(unsigned long now)      { /* TODO */ }
void handlePrimes(unsigned long now)        { /* TODO */ }
