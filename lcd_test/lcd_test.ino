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
  SORT_QUESTION,     // "Is bubble or merge / sort faster?" for 1.5 s
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

// ── Stub handlers (to be implemented) ────────────────────────────────────────
void handleWelcome(unsigned long now)       { /* TODO */ }
void handleProgramSelect(unsigned long now) { /* TODO */ }
void handleSortTest(unsigned long now)      { /* TODO */ }
void handlePrimes(unsigned long now)        { /* TODO */ }
