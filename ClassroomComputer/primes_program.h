#ifndef PRIMES_PROGRAM_H
#define PRIMES_PROGRAM_H

#include <Arduino.h>

// ── Primes program states ─────────────────────────────────────────────────────
enum PrimesState {
  PRIMES_TITLE,        // "Calculate Primes" for 1 s
  PRIMES_INTRO_1,      // "Choose the # of / primes calc'd" for 1.5 s
  PRIMES_INTRO_2,      // "Move slider to / specify the #" for 1.5 s
  PRIMES_SHOW_N,       // "N = [n]" until slider static for 1.5 s
  PRIMES_CALCULATING,  // "Calc'ing the 1st / [n] primes" until done
  PRIMES_RESULT        // "The [n]th prime / is [result] X" for 4.5 s
};

// ── Primes-specific state ─────────────────────────────────────────────────────
static PrimesState   primesState  = PRIMES_TITLE;
static int           primesN      = 500;   // locked-in N (how many primes to find)
static unsigned long primesResult = 0;     // the Nth prime, set by PRIMES_CALCULATING

// ── Forward declarations (need to be visible to other modules) ────────────────
void enterPrimesState(PrimesState next);
void handlePrimes(unsigned long now);

// ── Prime helper ──────────────────────────────────────────────────────────────
static bool isPrime(unsigned long n) {
  if (n < 2) return false;
  if (n == 2) return true;
  if (n % 2 == 0) return false;
  for (unsigned long i = 3; i * i <= n; i += 2) {
    if (n % i == 0) return false;
  }
  return true;
}

// ── Ordinal suffix helper ─────────────────────────────────────────────────────
static const char* ordinalSuffix(int n) {
  int mod100 = abs(n) % 100;
  if (mod100 >= 11 && mod100 <= 13) return "th";
  switch (abs(n) % 10) {
    case 1:  return "st";
    case 2:  return "nd";
    case 3:  return "rd";
    default: return "th";
  }
}

// ── Primes sub-handler forward declarations ───────────────────────────────────
static void handlePrimesTitle(unsigned long now);
static void handlePrimesIntro1(unsigned long now);
static void handlePrimesIntro2(unsigned long now);
static void handlePrimesShowN(unsigned long now);
static void handlePrimesCalculating(unsigned long now);
static void handlePrimesResult(unsigned long now);

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
extern unsigned long potLastMovedAt;
extern byte celebFrame0[8];
extern byte celebFrame1[8];
extern byte celebFrame2[8];
extern int celebFrameIdx;
extern unsigned long celebTickAt;
extern const unsigned long SCROLL_START_DELAY;
extern void tickScroll(const char* str, uint8_t row, unsigned long now, int wrapGap, bool loop);
extern void enterAppState(int nextState);  // forward declaration; APP_PROGRAM_SELECT = 1

void enterPrimesState(PrimesState next) {
  primesState    = next;
  stateEnteredAt = millis();
  scrollOffset   = 0;
  scrollTickAt   = millis();
  potHasMoved    = false;
  // Green backlight from PRIMES_CALCULATING through PRIMES_RESULT; pink otherwise
  if (next == PRIMES_CALCULATING || next == PRIMES_RESULT)
    lcd.setRGB(COL_GREEN[0], COL_GREEN[1], COL_GREEN[2]);
  else
    lcd.setRGB(COL_PINK[0],  COL_PINK[1],  COL_PINK[2]);
  lcd.clear();
}

void handlePrimes(unsigned long now) {
  switch (primesState) {
    case PRIMES_TITLE:       handlePrimesTitle(now);       break;
    case PRIMES_INTRO_1:     handlePrimesIntro1(now);      break;
    case PRIMES_INTRO_2:     handlePrimesIntro2(now);      break;
    case PRIMES_SHOW_N:      handlePrimesShowN(now);       break;
    case PRIMES_CALCULATING: handlePrimesCalculating(now); break;
    case PRIMES_RESULT:      handlePrimesResult(now);      break;
  }
}

// ── Primes sub-handlers ───────────────────────────────────────────────────────

// State 1 – "Calculate Primes" for 1 s, then advance.
// "Calculate Primes" is exactly 16 chars so no scrolling needed.
static void handlePrimesTitle(unsigned long now) {
  lcd.setCursor(0, 0);
  lcd.print("Calculate Primes");

  if (now - stateEnteredAt >= 1000UL) {
    enterPrimesState(PRIMES_INTRO_1);
  }
}

// State 2 – "Choose the # of / primes calc'd" for 1.5 s
static void handlePrimesIntro1(unsigned long now) {
  lcd.setCursor(0, 0);
  lcd.print("Choose the # of");
  lcd.setCursor(0, 1);
  lcd.print("primes calc'd");

  if (now - stateEnteredAt >= 1500UL) {
    enterPrimesState(PRIMES_INTRO_2);
  }
}

// State 3 – "Move slider to / specify the #" for 1.5 s
static void handlePrimesIntro2(unsigned long now) {
  lcd.setCursor(0, 0);
  lcd.print("Move slider to");
  lcd.setCursor(0, 1);
  lcd.print("specify the #");

  if (now - stateEnteredAt >= 1500UL) {
    enterPrimesState(PRIMES_SHOW_N);
  }
}

// State 4 – "N = [n]" with pot mapped to [30000, 100000].
// Locks in once slider is static for 1.5 s.
static void handlePrimesShowN(unsigned long now) {
  int n = map(potValue, 0, 1023, 30000, 100000);

  lcd.setCursor(0, 0);
  lcd.print("N = ");
  lcd.print(n);
  lcd.print("      ");  // overwrite leftover digits

  if (potHasMoved && (now - potLastMovedAt >= 1500UL)) {
    primesN = n;
    enterPrimesState(PRIMES_CALCULATING);
  }
}

// State 5 – "Calc'ing the 1st / [n] primes" while computing.
// Blocks until the Nth prime is found, then transitions.
static void handlePrimesCalculating(unsigned long now) {
  lcd.setCursor(0, 0);
  lcd.print("Calc'ing the 1st");
  lcd.setCursor(0, 1);
  lcd.print(primesN);
  lcd.print(" primes");

  // Find the primesN-th prime by trial division
  unsigned long count = 1;       // 2 is the 1st prime
  unsigned long candidate = 3;
  while (count < (unsigned long)primesN) {
    if (isPrime(candidate)) count++;
    if (count < (unsigned long)primesN) candidate += 2;
  }
  primesResult = (primesN == 1) ? 2 : candidate;

  enterPrimesState(PRIMES_RESULT);
}

// State 6 – "The [n]th prime / is [result] X" for 4.5 s.
// Top line scrolls if >16 chars; bottom is always static with celeb animation.
static void handlePrimesResult(unsigned long now) {
  // ── Top line ───────────────────────────────────────────────────────────────
  char topLine[32];
  snprintf(topLine, sizeof(topLine), "The %d%s prime", primesN, ordinalSuffix(primesN));
  int topLen = strlen(topLine);

  if (topLen <= 16) {
    lcd.setCursor(0, 0);
    lcd.print(topLine);
  } else {
    if (now - stateEnteredAt < SCROLL_START_DELAY) {
      lcd.setCursor(0, 0);
      for (int i = 0; i < 16; i++) lcd.write((uint8_t)topLine[i]);
    } else {
      tickScroll(topLine, 0, now, 4, true);
    }
  }

  // ── Bottom line (always fits in 16) ────────────────────────────────────────
  char botText[16];
  snprintf(botText, sizeof(botText), "is %lu ", primesResult);
  int celebCol = strlen(botText);

  lcd.setCursor(0, 1);
  lcd.print(botText);

  // ── Celebratory animation (pulsing diamond) ───────────────────────────────
  if (celebTickAt < stateEnteredAt) {
    celebFrameIdx = 0;
    lcd.createChar(0, celebFrame0);
    celebTickAt = stateEnteredAt + 200UL;
  }

  if (now >= celebTickAt) {
    celebFrameIdx = (celebFrameIdx + 1) % 3;
    byte* frames[3] = {celebFrame0, celebFrame1, celebFrame2};
    lcd.createChar(0, frames[celebFrameIdx]);
    celebTickAt = now + 200UL;
  }

  // createChar moves the cursor, so reposition before writing the char
  lcd.setCursor(celebCol, 1);
  lcd.write((uint8_t)0);

  // ── Timeout ────────────────────────────────────────────────────────────────
  if (now - stateEnteredAt >= 4500UL) {
    enterAppState(1);  // APP_PROGRAM_SELECT = 1
  }
}

#endif
