#ifndef SORT_PROGRAM_H
#define SORT_PROGRAM_H

#include <Arduino.h>

// ── Sort Test program states ───────────────────────────────────────────────────
enum SortTestState {
  SORT_TITLE,        // "Sort Test" for 0.75 s
  SORT_QUESTION,     // "Bubble or merge: / which is faster?" for 1.5 s
  SORT_SELECT_SIZE,  // "Move slider to select problem size"
  SORT_SHOW_N,       // "N = [n]" until slider static for 0.75 s
  SORT_CONFIRM_N,    // "Starting sort for / N = [n]" for 1 s
  SORT_RUNNING,      // "Bubble = TBD us… / Merge = TBD us…"
  SORT_RESULTS,      // results for 2.5 s
  SORT_WINNER        // "Merge sort is / the winner! X" for 2 s
};

// ── Sort-specific state ───────────────────────────────────────────────────────
static SortTestState sortState = SORT_TITLE;
static int           confirmedN = 10;        // N locked in when leaving SORT_SHOW_N
static unsigned long bubbleDuration = 0;     // µs
static unsigned long mergeDuration  = 0;     // µs
static int           sortBuf[500];           // scratch buffer (max N = 500)
static int           mergeTmp[500];          // merge sort temp buffer

// ── Forward declarations (need to be visible to other modules) ────────────────
// These are declared here but implemented below, and called from the main sketch
void enterSortState(SortTestState next);
void handleSortTest(unsigned long now);

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

// ── Sort sub-handler forward declarations ─────────────────────────────────────
static void handleSortTitle(unsigned long now);
static void handleSortQuestion(unsigned long now);
static void handleSortSelectSize(unsigned long now);
static void handleSortShowN(unsigned long now);
static void handleSortConfirmN(unsigned long now);
static void handleSortRunning(unsigned long now);
static void handleSortResults(unsigned long now);
static void handleSortWinner(unsigned long now);

// ── Implementations ───────────────────────────────────────────────────────────

// External references to shared state (defined in main sketch)
extern rgb_lcd lcd;
extern const byte COL_PINK[3];
extern const byte COL_GREEN[3];
extern unsigned long stateEnteredAt;
extern int scrollOffset;
extern unsigned long scrollTickAt;
extern bool potHasMoved;
extern int potValue;
extern int remappedPotValue;
extern unsigned long potLastMovedAt;
extern byte celebFrame0[8];
extern byte celebFrame1[8];
extern byte celebFrame2[8];
extern int celebFrameIdx;
extern unsigned long celebTickAt;
extern void enterAppState(int nextState);  // forward declaration; APP_PROGRAM_SELECT = 1

void enterSortState(SortTestState next) {
  sortState      = next;
  stateEnteredAt = millis();
  scrollOffset   = 0;
  scrollTickAt   = millis();
  potHasMoved    = false;
  if (next == SORT_RUNNING) lcd.setRGB(COL_GREEN[0], COL_GREEN[1], COL_GREEN[2]);
  else                      lcd.setRGB(COL_PINK[0],  COL_PINK[1],  COL_PINK[2]);
  lcd.clear();
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

// ── Sort sub-handlers ─────────────────────────────────────────────────────────

// State 1 – "Sort Test" for 1.75 s, then advance.
// Only 9 chars so no scrolling needed.
static void handleSortTitle(unsigned long now) {
  lcd.setCursor(0, 0);
  lcd.print("Sort Test");

  if (now - stateEnteredAt >= 1750UL) {
    enterSortState(SORT_QUESTION);
  }
}

// State 2 – "Bubble or merge: / which is faster?" for 2 s.
// Both lines fit in 16 chars exactly.
static void handleSortQuestion(unsigned long now) {
  lcd.setCursor(0, 0);
  lcd.print("Bubble or merge:");
  lcd.setCursor(0, 1);
  lcd.print("which is faster?");

  if (now - stateEnteredAt >= 2000UL) {
    enterSortState(SORT_SELECT_SIZE);
  }
}

// State 3 – "Move slider to / select prob size" until pot moves.
// Instructions fit in 16 chars.
static void handleSortSelectSize(unsigned long now) {
  lcd.setCursor(0, 0);
  lcd.print("Move slider to");
  lcd.setCursor(0, 1);
  lcd.print("select prob size");

  if (potHasMoved) {
    enterSortState(SORT_SHOW_N);
  }
}

// State 4 – "N = [n]" with pot mapped to [10, 350].
// Locks in once slider is static for 1.3 s.
static void handleSortShowN(unsigned long now) {
  lcd.setCursor(0, 0);
  lcd.print("N = ");
  lcd.print(remappedPotValue);
  lcd.print("     ");  // overwrite any leftover digits

  if (potHasMoved && (now - potLastMovedAt >= 1300UL)) {
    confirmedN = remappedPotValue;
    enterSortState(SORT_CONFIRM_N);
  }
}

// State 5 – "Starting sort / for N = [n]" for 1.3 s.
// Confirmation message before running the test.
static void handleSortConfirmN(unsigned long now) {
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

// State 6 – "Bubble = X µs... / Merge = Y µs..." while computing.
// Blocks while running both sorts, then transitions immediately.
static void handleSortRunning(unsigned long now) {
  lcd.setCursor(0, 0);
  lcd.print("Bubble = X ");
  lcd.write(1);  // µ custom character
  lcd.print("s...");
  lcd.setCursor(0, 1);
  lcd.print("Merge = Y ");
  lcd.write(1);  // µ custom character
  lcd.print("s...");

  // Bubble sort on a fresh random array
  for (int i = 0; i < confirmedN; i++) sortBuf[i] = random(10000);
  unsigned long t0 = micros();
  bubbleSort(sortBuf, confirmedN);
  bubbleDuration = micros() - t0;

  // Merge sort on a fresh random array
  for (int i = 0; i < confirmedN; i++) sortBuf[i] = random(10000);
  t0 = micros();
  mergeSortHelper(sortBuf, mergeTmp, confirmedN);
  mergeDuration = micros() - t0;

  enterSortState(SORT_RESULTS);
}

// State 7 – "Bubble = [time] µs / Merge  = [time] µs" for 3.5 s.
// Displays actual measured times in microseconds.
static void handleSortResults(unsigned long now) {
  lcd.setCursor(0, 0);
  lcd.print("Bubble = ");
  lcd.print(bubbleDuration);
  lcd.print(" ");
  lcd.write(1);  // µ custom character
  lcd.print("s     ");  // trailing spaces overwrite leftover digits

  lcd.setCursor(0, 1);
  lcd.print("Merge  = ");
  lcd.print(mergeDuration);
  lcd.print(" ");
  lcd.write(1);  // µ custom character
  lcd.print("s     ");

  if (now - stateEnteredAt >= 3500UL) {
    enterSortState(SORT_WINNER);
  }
}

// State 8 – "Merge sort is / the winner! X" for 3.6 s.
// Static text with pulsing diamond animation at end of bottom line.
static void handleSortWinner(unsigned long now) {
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
    enterAppState(1);  // APP_PROGRAM_SELECT = 1
  }
}

#endif
