#include <Wire.h>
#include "rgb_lcd.h"
#include "sort_program.h"
#include "primes_program.h"
#include "calculator_program.h"

// ══════════════════════════════════════════════════════════════════════════════
// HARDWARE
// ══════════════════════════════════════════════════════════════════════════════

rgb_lcd lcd;
const int POT_PIN    = A0;
const int BUZZER_PIN = 8;

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

// ── Cooldown between page transitions (prevents jump-through) ─────────────────
const unsigned long PAGE_TRANSITION_COOLDOWN = 300UL;

// ── Top-level application states ──────────────────────────────────────────────
enum AppState {
  APP_WELCOME,
  APP_PROGRAM_SELECT,
  APP_SORT_TEST,
  APP_PRIMES,
  APP_CALCULATOR
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

// ── Program selection page (1, 2, or 3) ───────────────────────────────────────
int selectionPage = 1;
unsigned long pageChangedAt = 0;  // millis() of last page transition

// ── Welcome jingle state ──────────────────────────────────────────────────────
bool welcomeJinglePlayed = false;

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

// ── Rightwards arrow (→) custom character ─────────────────────────────────────
// Custom character slot 2 = → (arrow) for program selection display
byte arrowChar[8] = { 0b00000, 0b00100, 0b00010, 0b11111, 0b00010, 0b00100, 0b00000, 0b00000 };

// ── Leftwards arrow (←) custom character ──────────────────────────────────────
// Custom character slot 3 = ← (left arrow) for program selection display
byte leftArrowChar[8] = { 0b00000, 0b00100, 0b01000, 0b11111, 0b01000, 0b00100, 0b00000, 0b00000 };

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
  if (next == APP_WELCOME) {
    welcomeJinglePlayed = false;  // Reset jingle for welcome screen
  } else if (next == APP_PROGRAM_SELECT) {
    selectionPage = 1;  // Always start at page 1 when entering selection screen
    pageChangedAt = 0;  // Reset cooldown timer
  } else if (next == APP_PRIMES) {
    enterPrimesState(PRIMES_TITLE);
  } else if (next == APP_SORT_TEST) {
    enterSortState(SORT_TITLE);
  } else if (next == APP_CALCULATOR) {
    enterCalcState(CALC_TITLE);
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
  lcd.createChar(2, arrowChar);    // slot 2 = → (rightwards arrow)
  lcd.createChar(3, leftArrowChar);// slot 3 = ← (leftwards arrow)

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
    case APP_CALCULATOR:     handleCalculator(now);    break;
  }
}

// ══════════════════════════════════════════════════════════════════════════════
// ORCHESTRATOR SCREENS
// ══════════════════════════════════════════════════════════════════════════════

// ── handleWelcome ─────────────────────────────────────────────────────────────
// Layout: 0.75 s static, then 6 s scrolling (wrap gap 4). Total = 6.75 s.
void handleWelcome(unsigned long now) {
  const char* msg = "Welcome to the Classroom Computer!";

  // Play welcome jingle 1 second after entering welcome state
  if (!welcomeJinglePlayed && (now - stateEnteredAt >= 1000UL)) {
    tone(BUZZER_PIN, 262, 100);  // C4
    delay(120);
    tone(BUZZER_PIN, 330, 100);  // E4
    delay(120);
    tone(BUZZER_PIN, 392, 100);  // G4
    delay(120);
    tone(BUZZER_PIN, 523, 150);  // C5
    welcomeJinglePlayed = true;
  }

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

  // ── Handle page transitions based on pot position ──────────────────────────
  // Only allow transitions after cooldown period to prevent jump-through
  if (now - pageChangedAt >= PAGE_TRANSITION_COOLDOWN) {
    if (selectionPage == 1) {
      if (potValue >= 819) {  // 80% → transition to page 2
        selectionPage = 2;
        potHasMoved = false;  // Prevent immediate program entry
        pageChangedAt = now;
      }
    } else if (selectionPage == 2) {
      if (potValue <= 153) {  // 15% → back to page 1
        selectionPage = 1;
        potHasMoved = false;
        pageChangedAt = now;
      } else if (potValue >= 870) {  // 85% → forward to page 3
        selectionPage = 3;
        potHasMoved = false;
        pageChangedAt = now;
      }
    } else if (selectionPage == 3) {
      if (potValue <= 153) {  // 15% → back to page 2
        selectionPage = 2;
        potHasMoved = false;
        pageChangedAt = now;
      }
    }
  }

  // ── Display bottom line based on current page ──────────────────────────────
  lcd.setCursor(0, 1);
  if (selectionPage == 1) {
    lcd.print("Sort | Primes ");
    lcd.write((uint8_t)2);  // → right arrow
    lcd.print(" ");         // pad to 16 chars
  } else if (selectionPage == 2) {
    lcd.write((uint8_t)3);  // ← left arrow
    lcd.print(" Calculator ");
    lcd.write((uint8_t)2);  // → right arrow
    lcd.print(" ");         // pad to 16 chars
  } else {  // page 3
    lcd.write((uint8_t)3);  // ← left arrow
    lcd.print(" Game | ASI  ");
  }

  // ── Handle program selection after 500ms hold ──────────────────────────────
  if (potHasMoved && (now - potLastMovedAt >= 500UL)) {
    if (selectionPage == 1) {
      if (potValue <= 409) {        // 0-40% → Sort
        enterAppState(APP_SORT_TEST);
      } else if (potValue <= 818) {  // 40-80% → Primes
        enterAppState(APP_PRIMES);
      }
      // 80-100% is transition zone, no program entry
    } else if (selectionPage == 2) {
      if (potValue >= 154 && potValue <= 869) {  // 15-85% → Calculator
        enterAppState(APP_CALCULATOR);
      }
      // 0-15% and 85-100% are transition zones
    } else if (selectionPage == 3) {
      // Game and ASI not yet implemented
      // if (potValue >= 154 && potValue <= 583) {  // 15-57% → Game
      //   enterAppState(APP_GAME);
      // } else if (potValue >= 584) {  // 57-100% → ASI
      //   enterAppState(APP_ASI);
      // }
    }
  }
}
