#ifndef CALCULATOR_PROGRAM_H
#define CALCULATOR_PROGRAM_H

#include <Arduino.h>

// ── Calculator program states ─────────────────────────────────────────────────
enum CalcState {
  CALC_TITLE,            // "Calculator Program" for 1.2 s
  CALC_INTRO,            // "Select two #s to / +, -, *, or /" for 3.3 s
  CALC_SELECT_A_INTRO,   // "Move slider to / select 1st #" for 2 s
  CALC_SELECT_A,         // "A = [value]" until slider static for 1.3 s
  CALC_SELECT_B_INTRO,   // "Move slider to / select 2nd #" for 1.2 s
  CALC_SELECT_B,         // "B = [value]" until slider static for 1.3 s
  CALC_SELECT_OP_INTRO,  // "Move slider to / select operation" for 1.2 s
  CALC_SELECT_OP,        // "       [op]" until slider static for 1.3 s
  CALC_RESULT            // "A [op] B = / [result] X" for 5 s, then back to program select
};

// ── Calculator-specific state ─────────────────────────────────────────────────
static CalcState calcState = CALC_TITLE;
static int       calcA     = 1;      // First number (1-1000)
static int       calcB     = 1;      // Second number (1-1000)
static char      calcOp    = '+';    // Operation: '+', '-', '*', '/'
static float     calcResult = 0.0;   // Computation result (float for division)

// ── Forward declarations (need to be visible to other modules) ────────────────
// These are declared here but implemented below, and called from the main sketch
void enterCalcState(CalcState next);
void handleCalculator(unsigned long now);

// ── Calculator helper: map pot to operation ───────────────────────────────────
static char mapPotToOp(int pot) {
  if (pot < 256)       return '+';
  else if (pot < 512)  return '-';
  else if (pot < 768)  return '*';
  else                 return '/';
}

// ── Calculator sub-handler forward declarations ───────────────────────────────
static void handleCalcTitle(unsigned long now);
static void handleCalcIntro(unsigned long now);
static void handleCalcSelectAIntro(unsigned long now);
static void handleCalcSelectA(unsigned long now);
static void handleCalcSelectBIntro(unsigned long now);
static void handleCalcSelectB(unsigned long now);
static void handleCalcSelectOpIntro(unsigned long now);
static void handleCalcSelectOp(unsigned long now);
static void handleCalcResult(unsigned long now);

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
extern byte celebFrame3[8];
extern byte celebFrame4[8];
extern byte celebFrame5[8];
extern byte celebFrame6[8];
extern byte celebFrame7[8];
extern const int CELEB_FRAME_COUNT;
extern int celebFrameIdx;
extern unsigned long celebTickAt;
extern const int BUZZER_PIN;
extern void tickCelebrationSound(unsigned long now);
extern void enterAppState(int nextState);  // forward declaration; APP_PROGRAM_SELECT = 1

void enterCalcState(CalcState next) {
  calcState      = next;
  stateEnteredAt = millis();
  scrollOffset   = 0;
  scrollTickAt   = millis();
  potHasMoved    = false;
  if (next == CALC_RESULT) lcd.setRGB(COL_GREEN[0], COL_GREEN[1], COL_GREEN[2]);
  else                     lcd.setRGB(COL_PINK[0],  COL_PINK[1],  COL_PINK[2]);
  lcd.clear();
}

void handleCalculator(unsigned long now) {
  switch (calcState) {
    case CALC_TITLE:           handleCalcTitle(now);          break;
    case CALC_INTRO:           handleCalcIntro(now);          break;
    case CALC_SELECT_A_INTRO:  handleCalcSelectAIntro(now);   break;
    case CALC_SELECT_A:        handleCalcSelectA(now);        break;
    case CALC_SELECT_B_INTRO:  handleCalcSelectBIntro(now);   break;
    case CALC_SELECT_B:        handleCalcSelectB(now);        break;
    case CALC_SELECT_OP_INTRO: handleCalcSelectOpIntro(now);  break;
    case CALC_SELECT_OP:       handleCalcSelectOp(now);       break;
    case CALC_RESULT:          handleCalcResult(now);         break;
  }
}

// ── Calculator sub-handlers ───────────────────────────────────────────────────

// State 1 – "Calculator Program" for 1 s, then advance.
static void handleCalcTitle(unsigned long now) {
  lcd.setCursor(0, 0);
  lcd.print("Calculator");
  lcd.setCursor(0, 1);
  lcd.print("Program");

  if (now - stateEnteredAt >= 1200UL) {
    enterCalcState(CALC_INTRO);
  }
}

// State 2 – "Select two #s to / +, -, *, or /" for 2 s.
static void handleCalcIntro(unsigned long now) {
  lcd.setCursor(0, 0);
  lcd.print("Select two #s to");
  lcd.setCursor(0, 1);
  lcd.print("+, -, *, or /");

  if (now - stateEnteredAt >= 3300UL) {
    enterCalcState(CALC_SELECT_A_INTRO);
  }
}

// State 3 – "Move slider to / select 1st #" for 2 s.
static void handleCalcSelectAIntro(unsigned long now) {
  lcd.setCursor(0, 0);
  lcd.print("Move slider to");
  lcd.setCursor(0, 1);
  lcd.print("select 1st #");

  if (now - stateEnteredAt >= 2000UL) {
    enterCalcState(CALC_SELECT_A);
  }
}

// State 4 – "A = [remappedPotValue]" until slider static for 1.3 s.
// Map pot 0-1023 to 1-1000 locally for calculator.
static void handleCalcSelectA(unsigned long now) {
  int displayValue = map(potValue, 0, 1023, 1, 1000);

  lcd.setCursor(0, 0);
  lcd.print("A = ");
  lcd.print(displayValue);
  lcd.print("     ");  // overwrite any leftover digits

  if (potHasMoved && (now - potLastMovedAt >= 1300UL)) {
    calcA = displayValue;
    enterCalcState(CALC_SELECT_B_INTRO);
  }
}

// State 5 – "Move slider to / select 2nd #" for 1.2 s.
static void handleCalcSelectBIntro(unsigned long now) {
  lcd.setCursor(0, 0);
  lcd.print("Move slider to");
  lcd.setCursor(0, 1);
  lcd.print("select 2nd #");

  if (now - stateEnteredAt >= 1200UL) {
    enterCalcState(CALC_SELECT_B);
  }
}

// State 6 – "B = [remappedPotValue]" until slider static for 1.3 s.
// Map pot 0-1023 to 1-1000 locally for calculator.
static void handleCalcSelectB(unsigned long now) {
  int displayValue = map(potValue, 0, 1023, 1, 1000);

  lcd.setCursor(0, 0);
  lcd.print("B = ");
  lcd.print(displayValue);
  lcd.print("     ");  // overwrite any leftover digits

  if (potHasMoved && (now - potLastMovedAt >= 1300UL)) {
    calcB = displayValue;
    enterCalcState(CALC_SELECT_OP_INTRO);
  }
}

// State 7 – "Move slider to / select operation" for 1.2 s.
static void handleCalcSelectOpIntro(unsigned long now) {
  lcd.setCursor(0, 0);
  lcd.print("Move slider to");
  lcd.setCursor(0, 1);
  lcd.print("select operation");

  if (now - stateEnteredAt >= 1200UL) {
    enterCalcState(CALC_SELECT_OP);
  }
}

// State 8 – "       [op]" (centered operation symbol) until slider static for 1.3 s.
// Pot is divided into 4 quartiles: 0-255 = '+', 256-511 = '-', 512-767 = '*', 768-1023 = '/'.
static void handleCalcSelectOp(unsigned long now) {
  char op = mapPotToOp(potValue);

  lcd.setCursor(0, 0);
  lcd.print("       ");  // 7 spaces for centering
  lcd.print(op);
  lcd.print("        ");  // trailing spaces

  if (potHasMoved && (now - potLastMovedAt >= 1300UL)) {
    calcOp = op;
    enterCalcState(CALC_RESULT);
  }
}

// ── Result formatting helpers ─────────────────────────────────────────────────

// Inserts commas into a string of digits with an optional leading '-'.
// E.g. "1000000" -> "1,000,000", "-1000" -> "-1,000", "999" -> "999"
static void addCommasToIntStr(const char* digits, char* out) {
  int outIdx = 0;
  int start = 0;
  int len = strlen(digits);
  if (len > 0 && digits[0] == '-') { out[outIdx++] = '-'; start = 1; len--; }
  int first = (len % 3 == 0) ? 3 : len % 3;
  for (int i = 0; i < first; i++) out[outIdx++] = digits[start + i];
  for (int i = first; i < len; i++) {
    if ((i - first) % 3 == 0) out[outIdx++] = ',';
    out[outIdx++] = digits[start + i];
  }
  out[outIdx] = '\0';
}

// Formats a float result into out[] (must be ≥ 17 bytes):
//   Integer: no decimal point, commas every 3 digits  e.g. "1,000,000"
//   Non-integer: integer part with commas + '.' + as many decimal digits as
//                fit within 14 total characters (float precision capped at 6)
static void formatCalcResult(float result, char* out) {
  const int MAX_CHARS = 14;
  long longVal = (long)result;
  bool isInt = (result == (float)longVal);

  char digits[14];
  ltoa(longVal, digits, 10);

  if (isInt) {
    addCommasToIntStr(digits, out);
  } else {
    // Build integer part with commas, then append decimal portion
    char intBuf[14];
    addCommasToIntStr(digits, intBuf);
    int intLen = strlen(intBuf);

    int decPlaces = MAX_CHARS - intLen - 1;  // chars left after intPart + '.'
    if (decPlaces < 1) decPlaces = 1;
    // No upper cap: fill the full 14-char budget for maximum precision

    char fullBuf[24];
    dtostrf(result, 0, decPlaces, fullBuf);
    char* dot = strchr(fullBuf, '.');
    strcpy(out, intBuf);
    if (dot) strcat(out, dot);

    // Strip trailing zeros from decimal part (e.g. "499.5000000000" -> "499.5")
    char* outDot = strchr(out, '.');
    if (outDot) {
      int i = strlen(out) - 1;
      while (i > (outDot - out) && out[i] == '0') i--;
      out[i + 1] = '\0';
    }
  }
}

// State 9 – "A [op] B = / [result]" for 5 s, then back to program select.
static void handleCalcResult(unsigned long now) {
  // Compute result on first entry (or within first 10 ms of re-entry)
  static bool computed = false;
  if (!computed || (now - stateEnteredAt) < 10) {
    computed = true;
    switch (calcOp) {
      case '+': calcResult = (float)calcA + calcB; break;
      case '-': calcResult = (float)calcA - calcB; break;
      case '*': calcResult = (float)calcA * calcB; break;
      case '/': calcResult = (float)calcA / calcB; break;
    }
  }

  // Display "A [op] B =" on top line
  lcd.setCursor(0, 0);
  lcd.print(calcA);
  lcd.print(" ");
  lcd.print(calcOp);
  lcd.print(" ");
  lcd.print(calcB);
  lcd.print(" =");
  lcd.print("        ");  // clear any leftover

  // Format result string
  char resultStr[20];
  formatCalcResult(calcResult, resultStr);
  int len = strlen(resultStr);

  // Display result on bottom line.
  // Layout: result (≤14 chars) + " " + celebChar  — OR —
  //         first 14 chars + ".." when result exceeds 14 chars.
  lcd.setCursor(0, 1);
  if (len > 14) {
    // Truncate to 14 chars and fill cols 14-15 with ".."
    char buf[15];
    strncpy(buf, resultStr, 14);
    buf[14] = '\0';
    lcd.print(buf);
    lcd.print("..");
  } else {
    lcd.print(resultStr);
    lcd.print(" ");

    // Celebration animation (pulse → spin → sparkle) in remaining space
    if (celebTickAt < stateEnteredAt) {
      celebFrameIdx = 0;
      lcd.createChar(0, celebFrame0);
      celebTickAt = stateEnteredAt + 200UL;
    }
    if (now >= celebTickAt) {
      celebFrameIdx = (celebFrameIdx + 1) % CELEB_FRAME_COUNT;
      byte* frames[8] = {celebFrame0, celebFrame1, celebFrame2, celebFrame3,
                          celebFrame4, celebFrame5, celebFrame6, celebFrame7};
      lcd.createChar(0, frames[celebFrameIdx]);
      celebTickAt = now + 200UL;
    }
    lcd.write((uint8_t)0);  // celebration character right after result
  }

  // Celebration jingle
  tickCelebrationSound(now);

  // Return to program select after 5 seconds
  if (now - stateEnteredAt >= 5000UL) {
    computed = false;  // reset for next run
    enterAppState(1);  // APP_PROGRAM_SELECT = 1
  }
}

#endif
