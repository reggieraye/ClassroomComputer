#ifndef PADDLE_GAME_H
#define PADDLE_GAME_H

#include <Arduino.h>

// ── Paddle Game states ────────────────────────────────────────────────────────
enum PaddleGameState {
  GAME_TITLE,         // "Paddle Ball" for 1.5 s
  GAME_INSTRUCTIONS,  // Instructions for 2.5 s
  GAME_READY,         // "Ready..." countdown for 1.5 s
  GAME_PLAYING,       // Active gameplay until miss
  GAME_OVER,          // "Game Over! / Score: X" for 3 s
  GAME_RESULT         // Celebration screen for 4.5 s
};

// ── Level speed table (10 levels, each ~20% faster than previous) ─────────────
// Delays in ms: level 1 = 350ms, each subsequent level = prev / 1.20
static const unsigned long LEVEL_DELAYS[10] = {350, 292, 243, 203, 169, 141, 117, 98, 82, 68};
static const int NUM_LEVELS = 10;
static const int HITS_PER_LEVEL = 5;   // paddle hits needed to advance one level

// ── Game-specific state ───────────────────────────────────────────────────────
static PaddleGameState gameState = GAME_TITLE;
static int ballX = 1, ballY = 0;       // Ball position (X: 0-14, Y: 0-1)
static int ballDX = 1, ballDY = 1;     // Ball velocity (-1 or +1)
static int paddlePos = 0;              // 0=row0, 2=row1 (no middle position)
static int score = 0;                  // Number of successful paddle hits
static int finalScore = 0;             // Saved score for display
static int level = 1;                  // Current difficulty level (1-10)
static unsigned long ballDelay = 350;  // ms between ball moves (set from LEVEL_DELAYS)
static unsigned long lastBallMove = 0; // timestamp of last ball movement

// ── Forward declarations ──────────────────────────────────────────────────────
void enterGameState(PaddleGameState next);
void handlePaddleGame(unsigned long now);

// ── Game helpers ──────────────────────────────────────────────────────────────
static void moveBall();
static void updatePaddleFromPot();
static void drawGameScreen();

// ── State handler forward declarations ────────────────────────────────────────
static void handleGameTitle(unsigned long now);
static void handleGameInstructions(unsigned long now);
static void handleGameReady(unsigned long now);
static void handleGamePlaying(unsigned long now);
static void handleGameOver(unsigned long now);
static void handleGameResult(unsigned long now);

// ── Implementations ───────────────────────────────────────────────────────────

// External references (defined in main sketch)
extern rgb_lcd lcd;
extern const byte COL_PINK[3];
extern const byte COL_GREEN[3];
extern unsigned long stateEnteredAt;
extern bool potHasMoved;
extern int potValue;
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
extern void enterAppState(int nextState);

// Custom character definitions (created on entry to GAME_PLAYING)
byte ballChar[8] = {
  0b00000,
  0b01110,
  0b11111,
  0b11111,
  0b11111,
  0b01110,
  0b00000,
  0b00000
};

byte paddleChar[8] = {
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111
};

void enterGameState(PaddleGameState next) {
  gameState = next;
  stateEnteredAt = millis();
  potHasMoved = false;

  if (next == GAME_PLAYING) {
    lcd.setRGB(COL_GREEN[0], COL_GREEN[1], COL_GREEN[2]);
    // Seed RNG from timing jitter so each game plays differently
    randomSeed(micros());
    // Reset game state
    ballX = 1;
    ballY = random(0, 2);           // random starting row
    ballDX = 1;
    ballDY = (random(0, 2) == 0) ? 1 : -1;  // random starting vertical direction
    paddlePos = 0;
    score = 0;
    level = 1;
    ballDelay = LEVEL_DELAYS[0];
    lastBallMove = millis();
    // Define custom characters for ball and paddle
    lcd.createChar(4, ballChar);
    lcd.createChar(5, paddleChar);
  } else if (next == GAME_RESULT) {
    lcd.setRGB(COL_GREEN[0], COL_GREEN[1], COL_GREEN[2]);
  } else {
    lcd.setRGB(COL_PINK[0], COL_PINK[1], COL_PINK[2]);
  }

  lcd.clear();
}

void handlePaddleGame(unsigned long now) {
  switch (gameState) {
    case GAME_TITLE:        handleGameTitle(now);        break;
    case GAME_INSTRUCTIONS: handleGameInstructions(now); break;
    case GAME_READY:        handleGameReady(now);        break;
    case GAME_PLAYING:      handleGamePlaying(now);      break;
    case GAME_OVER:         handleGameOver(now);         break;
    case GAME_RESULT:       handleGameResult(now);       break;
  }
}

// ── Game helper implementations ───────────────────────────────────────────────

static void moveBall() {
  // Calculate new position
  int newX = ballX + ballDX;
  int newY = ballY + ballDY;

  // Bounce off top/bottom walls
  if (newY < 0) {
    ballDY = 1;
    newY = 0;
    tone(BUZZER_PIN, 300, 50);
  } else if (newY > 1) {
    ballDY = -1;
    newY = 1;
    tone(BUZZER_PIN, 300, 50);
  }

  // Bounce off left wall — randomize vertical direction to break predictable pattern
  if (newX < 0) {
    ballDX = 1;
    newX = 0;
    // 50% chance to keep current Y direction, 50% chance to flip it
    if (random(0, 2) == 0) {
      ballDY = -ballDY;
    }
    // Clamp: if random flip would send ball out of bounds, correct it
    if (newY <= 0) ballDY = 1;
    if (newY >= 1) ballDY = -1;
    tone(BUZZER_PIN, 300, 50);
  }

  // Check right edge (paddle at column 15)
  if (newX >= 15) {
    // Check if paddle covers the ball's row
    bool paddleHit = false;
    if (paddlePos == 0 && newY == 0) paddleHit = true;
    if (paddlePos == 2 && newY == 1) paddleHit = true;

    if (paddleHit) {
      ballDX = -1;
      newX = 14;  // keep ball at column 14
      score++;
      tone(BUZZER_PIN, 600, 80);

      // 40% chance to randomize Y direction on paddle hit for extra unpredictability
      if (random(0, 5) < 2) {
        ballDY = -ballDY;
      }

      // Advance level every HITS_PER_LEVEL hits (up to NUM_LEVELS)
      int newLevel = (score / HITS_PER_LEVEL) + 1;
      if (newLevel > NUM_LEVELS) newLevel = NUM_LEVELS;
      if (newLevel != level) {
        level = newLevel;
        ballDelay = LEVEL_DELAYS[level - 1];
      }
    } else {
      // Miss! Play game-over sound and transition
      tone(BUZZER_PIN, 400, 200);
      delay(220);
      tone(BUZZER_PIN, 300, 200);
      delay(220);
      tone(BUZZER_PIN, 200, 300);
      finalScore = score;
      enterGameState(GAME_OVER);
      return;
    }
  }

  ballX = newX;
  ballY = newY;
}

static void updatePaddleFromPot() {
  // Map pot to paddle position (no debounce - immediate response needed)
  // Only two positions: commit to upper or lower row
  if (potValue < 512) {
    paddlePos = 0;  // row 0 only (upper)
  } else {
    paddlePos = 2;  // row 1 only (lower)
  }
}

static void drawGameScreen() {
  static int prevBallX = -1, prevBallY = -1;
  static int prevPaddlePos = -1;

  // Clear previous ball position
  if (prevBallX >= 0 && (prevBallX != ballX || prevBallY != ballY)) {
    lcd.setCursor(prevBallX, prevBallY);
    lcd.print(" ");
  }

  // Draw ball
  lcd.setCursor(ballX, ballY);
  lcd.write((uint8_t)4);

  // Update paddle if position changed
  if (prevPaddlePos != paddlePos) {
    // Clear old paddle
    lcd.setCursor(15, 0);
    lcd.print(" ");
    lcd.setCursor(15, 1);
    lcd.print(" ");

    // Draw new paddle
    if (paddlePos == 0) {
      lcd.setCursor(15, 0);
      lcd.write((uint8_t)5);
    }
    if (paddlePos == 2) {
      lcd.setCursor(15, 1);
      lcd.write((uint8_t)5);
    }

    prevPaddlePos = paddlePos;
  }

  prevBallX = ballX;
  prevBallY = ballY;
}

// ── State handlers ────────────────────────────────────────────────────────────

static void handleGameTitle(unsigned long now) {
  lcd.setCursor(0, 0);
  lcd.print("Paddle Ball");

  if (now - stateEnteredAt >= 1500UL) {
    enterGameState(GAME_INSTRUCTIONS);
  }
}

static void handleGameInstructions(unsigned long now) {
  lcd.setCursor(0, 0);
  lcd.print("Use slider to");
  lcd.setCursor(0, 1);
  lcd.print("move the paddle");

  if (now - stateEnteredAt >= 2500UL) {
    enterGameState(GAME_READY);
  }
}

static void handleGameReady(unsigned long now) {
  unsigned long elapsed = now - stateEnteredAt;

  lcd.setCursor(0, 0);
  if (elapsed < 500) {
    lcd.print("Ready...  3");
  } else if (elapsed < 1000) {
    lcd.print("Ready...  2");
  } else {
    lcd.print("Ready...  1");
  }

  if (elapsed >= 1500UL) {
    enterGameState(GAME_PLAYING);
  }
}

static void handleGamePlaying(unsigned long now) {
  // Update paddle position from pot (immediate response)
  updatePaddleFromPot();

  // Move ball at fixed intervals
  if (now - lastBallMove >= ballDelay) {
    moveBall();
    lastBallMove = now;
  }

  // Render game screen
  drawGameScreen();
}

static void handleGameOver(unsigned long now) {
  lcd.setCursor(0, 0);
  lcd.print("Game Over!");
  lcd.setCursor(0, 1);
  lcd.print("Score: ");
  lcd.print(finalScore);
  lcd.print("     ");  // clear leftover digits

  if (now - stateEnteredAt >= 3000UL) {
    enterGameState(GAME_RESULT);
  }
}

static void handleGameResult(unsigned long now) {
  // Static text with celebration animation
  if (celebTickAt < stateEnteredAt) {
    celebFrameIdx = 0;
    lcd.createChar(0, celebFrame0);
    lcd.setCursor(0, 0);
    lcd.print("Great job! ");
    lcd.setCursor(0, 1);
    lcd.print("Hits: ");
    lcd.print(finalScore);
    lcd.print("   ");  // clear leftover
    celebTickAt = stateEnteredAt + 200UL;
  }

  // Advance animation frame
  if (now >= celebTickAt) {
    celebFrameIdx = (celebFrameIdx + 1) % CELEB_FRAME_COUNT;
    byte* frames[8] = {celebFrame0, celebFrame1, celebFrame2, celebFrame3,
                        celebFrame4, celebFrame5, celebFrame6, celebFrame7};
    lcd.createChar(0, frames[celebFrameIdx]);
    celebTickAt = now + 200UL;
  }

  // Draw celebration character after "job! "
  lcd.setCursor(11, 0);
  lcd.write((uint8_t)0);

  // Celebration sound
  tickCelebrationSound(now);

  if (now - stateEnteredAt >= 4500UL) {
    enterAppState(1);  // APP_PROGRAM_SELECT
  }
}

#endif
