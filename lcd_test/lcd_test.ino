#include <Wire.h>
#include "rgb_lcd.h"

// ── Hardware ──────────────────────────────────────────────────────────────────
rgb_lcd lcd;
const int POT_PIN    = A0;
const int BUZZER_PIN = 2;

// ── Backlight colours ─────────────────────────────────────────────────────────
const byte COL_PINK[3]  = {255,   0, 128};
const byte COL_GREEN[3] = {  0, 255,   0};

// ── Scroll speed: 1 (slowest) – 5 (fastest) ──────────────────────────────────
int scrollSpeed = 3;

// ── Delay before scrolling begins after entering a state ──────────────────────
const unsigned long SCROLL_START_DELAY = 750UL;

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

// ── Sort scratch buffers (max N = 500) ────────────────────────────────────────
int sortBuf[500];
int mergeTmp[500];

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
  lcd.setRGB(COL_PINK[0], COL_PINK[1], COL_PINK[2]);
  lcd.clear();
}

void enterSortState(SortTestState next) {
  sortState      = next;
  stateEnteredAt = millis();
  scrollOffset   = 0;
  scrollTickAt   = millis();
  potHasMoved    = false;
  if (next == SORT_CONFIRM_N) lcd.setRGB(COL_GREEN[0], COL_GREEN[1], COL_GREEN[2]);
  lcd.clear();
}

// ── Handler forward declarations ──────────────────────────────────────────────
void handleWelcome(unsigned long now);
void handleProgramSelect(unsigned long now);
void handleSortTest(unsigned long now);
void handlePrimes(unsigned long now);

// ── Sort sub-handler forward declarations ─────────────────────────────────────
void handleSortTitle(unsigned long now);
void handleSortQuestion(unsigned long now);
void handleSortSelectSize(unsigned long now);
void handleSortShowN(unsigned long now);
void handleSortConfirmN(unsigned long now);
void handleSortRunning(unsigned long now);
void handleSortResults(unsigned long now);
void handleSortWinner(unsigned long now);

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
  remappedPotValue = map(potValue, 0, 1023, 10, 350);

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
void tickScroll(const char* str, uint8_t row, unsigned long now, int wrapGap = 8) {
  int len   = strlen(str);
  int cycle = len + wrapGap;  // string length + blank gap before wrap

  if (now >= scrollTickAt) {
    scrollOffset = (scrollOffset + 1) % cycle;
    scrollTickAt = now + 100UL + (unsigned long)(6 - scrollSpeed) * 50UL;
  }

  lcd.setCursor(0, row);
  for (int i = 0; i < 16; i++) {
    int idx = scrollOffset + i;
    lcd.write((idx < len) ? (uint8_t)str[idx] : (uint8_t)' ');
  }
}

// ── handleWelcome ─────────────────────────────────────────────────────────────
// Layout: 0.75 s static, then 6 s scrolling (wrap gap 4). Total = 6.75 s.
void handleWelcome(unsigned long now) {
  const char* msg = "Welcome to the Classroom Computer!";

  if (now - stateEnteredAt < SCROLL_START_DELAY) {
    // Static display – show the first 16 characters before scrolling begins
    lcd.setCursor(0, 0);
    for (int i = 0; i < 16; i++) lcd.write((uint8_t)msg[i]);
  } else {
    tickScroll(msg, 0, now, 4);
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
    tickScroll(msg, 0, now, 5);
  }

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
void handleSortTest(unsigned long now) {
  switch (sortState) {
    case SORT_TITLE:       handleSortTitle(now);       break;
    case SORT_QUESTION:    handleSortQuestion(now);    break;
    case SORT_SELECT_SIZE: handleSortSelectSize(now);  break;
    case SORT_SHOW_N:      handleSortShowN(now);       break;
    case SORT_CONFIRM_N:   handleSortConfirmN(now);    break;
    case SORT_RUNNING:     handleSortRunning(now);     break;
    case SORT_RESULTS:     handleSortResults(now);     break;
    case SORT_WINNER:      handleSortWinner(now);      break;
  }
}
void handlePrimes(unsigned long now)        { /* TODO */ }

// ── Sort sub-handlers  ───────────────────────────────────────────────────────
void handleSortTitle(unsigned long now) {
  lcd.setCursor(0, 0);
  lcd.print("Sort Test");

  if (now - stateEnteredAt >= 1750UL) {
    enterSortState(SORT_QUESTION);
  }
}
void handleSortQuestion(unsigned long now) {
  lcd.setCursor(0, 0);
  lcd.print("Bubble or merge:");
  lcd.setCursor(0, 1);
  lcd.print("which is faster?");

  if (now - stateEnteredAt >= 2000UL) {
    enterSortState(SORT_SELECT_SIZE);
  }
}
void handleSortSelectSize(unsigned long now) {
  lcd.setCursor(0, 0);
  lcd.print("Move slider to");
  lcd.setCursor(0, 1);
  lcd.print("select prob size");

  if (potHasMoved) {
    enterSortState(SORT_SHOW_N);
  }
}
void handleSortShowN(unsigned long now) {
  lcd.setCursor(0, 0);
  lcd.print("N = ");
  lcd.print(remappedPotValue);
  lcd.print("     ");  // overwrite any leftover digits

  if (potHasMoved && (now - potLastMovedAt >= 1300UL)) {
    confirmedN = remappedPotValue;
    enterSortState(SORT_CONFIRM_N);
  }
}
void handleSortConfirmN(unsigned long now) {
  lcd.setCursor(0, 0);
  lcd.print("Starting sort");
  lcd.setCursor(0, 1);
  lcd.print("for N = ");
  lcd.print(confirmedN);
  lcd.print("     ");  // clear any leftover digits

  if (now - stateEnteredAt >= 1300UL) {
    enterSortState(SORT_RUNNING);
  }
}
// ── Sort algorithm helpers ────────────────────────────────────────────────────
static void bubbleSort(int* a, int n) {
  for (int i = 0; i < n - 1; i++)
    for (int j = 0; j < n - 1 - i; j++)
      if (a[j] > a[j+1]) { int t = a[j]; a[j] = a[j+1]; a[j+1] = t; }
}

static void mergeSortHelper(int* a, int* tmp, int n) {
  if (n <= 1) return;
  int mid = n / 2;
  mergeSortHelper(a,       tmp, mid);
  mergeSortHelper(a + mid, tmp, n - mid);
  int i = 0, j = mid, k = 0;
  while (i < mid && j < n) tmp[k++] = (a[i] <= a[j]) ? a[i++] : a[j++];
  while (i < mid)           tmp[k++] = a[i++];
  while (j < n)             tmp[k++] = a[j++];
  for (int x = 0; x < n; x++) a[x] = tmp[x];
}

void handleSortRunning(unsigned long now) {
  lcd.setCursor(0, 0);
  lcd.print("Bubble = X ms...");
  lcd.setCursor(0, 1);
  lcd.print("Merge = Y ms...");

  // Bubble sort on a fresh random array
  for (int i = 0; i < confirmedN; i++) sortBuf[i] = random(10000);
  unsigned long t0 = micros();
  bubbleSort(sortBuf, confirmedN);
  bubbleDuration = (micros() - t0) / 1000UL;

  // Merge sort on a fresh random array
  for (int i = 0; i < confirmedN; i++) sortBuf[i] = random(10000);
  t0 = micros();
  mergeSortHelper(sortBuf, mergeTmp, confirmedN);
  mergeDuration = (micros() - t0) / 1000UL;

  enterSortState(SORT_RESULTS);
}
void handleSortResults(unsigned long now) {
  lcd.setCursor(0, 0);
  lcd.print("Bubble = ");
  lcd.print(bubbleDuration);
  lcd.print(" ms     ");  // trailing spaces overwrite leftover digits

  lcd.setCursor(0, 1);
  lcd.print("Merge  = ");
  lcd.print(mergeDuration);
  lcd.print(" ms     ");

  if (now - stateEnteredAt >= 3500UL) {
    enterSortState(SORT_WINNER);
  }
}
void handleSortWinner(unsigned long now) {
  // Write static text once on entry; also reset animation
  if (celebTickAt < stateEnteredAt) {
    celebFrameIdx = 0;
    lcd.createChar(0, celebFrame0);
    lcd.setCursor(0, 0);
    lcd.print("Merge sort is");
    lcd.setCursor(0, 1);
    lcd.print("the winner! ");
    celebTickAt = stateEnteredAt + 200UL;
  }

  // Advance animation frame on each tick
  if (now >= celebTickAt) {
    celebFrameIdx = (celebFrameIdx + 1) % 3;
    byte* frames[3] = {celebFrame0, celebFrame1, celebFrame2};
    lcd.createChar(0, frames[celebFrameIdx]);
    celebTickAt = now + 200UL;
  }

  // Redraw animated char at col 12, row 1 (createChar moves the cursor)
  lcd.setCursor(12, 1);
  lcd.write((uint8_t)0);

  if (now - stateEnteredAt >= 3600UL) {
    enterAppState(APP_PROGRAM_SELECT);
  }
}
