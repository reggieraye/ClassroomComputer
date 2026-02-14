// Copyright @ardu_projects
// Simplified Asteroids Game with dynamic score
//
// Wiring diagram:
// - Button LEFT  -> Pin 12 (INPUT_PULLUP)
// - Button RIGHT -> Pin 11 (INPUT_PULLUP)
// - Button FIRE  -> Pin 10 (INPUT_PULLUP)
// - Buzzer       -> Pin 5  (OUTPUT)
// - OLED Display -> I2C (A4 = SDA, A5 = SCL for Uno/Nano)
//
// Configurable variables:
// - MAX_BULLETS: max number of on-screen bullets
// - MAX_ASTEROIDS: max number of asteroids
//
// Gameplay:
// - Move the ship with LEFT/RIGHT buttons
// - FIRE button shoots bullets upward
// - If an asteroid passes the bottom of the screen, score decreases by 1
// - If a bullet hits an asteroid, score increases by 2
// - If an asteroid hits the ship, it's GAME OVER
// - High score is stored in EEPROM (not yet implemented here)

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h>

// Display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pin definitions
#define BTN_LEFT 12
#define BTN_RIGHT 11
#define BTN_FIRE 10
#define BUZZER 5

// Game limits
#define MAX_BULLETS 5
#define MAX_ASTEROIDS 6

// Ship position
float shipX = SCREEN_WIDTH / 2;
const int shipY = SCREEN_HEIGHT - 5;

// Bullet structure
struct Bullet {
  float x, y;
  bool active;
};
Bullet bullets[MAX_BULLETS];

// Asteroid structure
struct Asteroid {
  float x, y;
  bool active;
};
Asteroid asteroids[MAX_ASTEROIDS];

// Game state
int score = 0;
bool gameOver = false;
bool gameStarted = false;

// Function to play a sound on the buzzer
void playTone(int f, int d) {
  tone(BUZZER, f, d);
  delay(d);
  noTone(BUZZER);
}

// Fires a bullet if one is available in the pool
void fireBullet() {
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (!bullets[i].active) {
      bullets[i].x = shipX;
      bullets[i].y = shipY - 4;
      bullets[i].active = true;
      playTone(800, 30);
      break;
    }
  }
}

// Initializes all asteroids at random off-screen positions
void spawnAsteroids() {
  for (int i = 0; i < MAX_ASTEROIDS; i++) {
    asteroids[i].x = random(0, SCREEN_WIDTH);
    asteroids[i].y = random(-SCREEN_HEIGHT, -5);
    asteroids[i].active = true;
  }
}

// Resets the game state to initial conditions
void resetGame() {
  shipX = SCREEN_WIDTH / 2;
  score = 0;
  gameOver = false;
  for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = false;
  spawnAsteroids();
}

// Displays the start screen and waits for the player to press FIRE
void drawStartScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 20);
  display.println("ASTEROIDS");
  display.setTextSize(1);
  display.setCursor(7, 40);
  display.println("Press FIRE to start");
  display.display();

  while (digitalRead(BTN_FIRE)) {
    delay(10);
  }
  delay(500);
  gameStarted = true;

  tone(BUZZER, 400, 150);  // Low tone for start
  delay(200);
  tone(BUZZER, 600, 150);  // Medium tone
  delay(200);
  tone(BUZZER, 800, 150);  // High tone
  delay(200);
  noTone(BUZZER);          // Stop the tone

  resetGame();
}

// Displays the game over screen and resets game
void drawGameOverScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 20);
  display.println("GAME OVER");
  display.setTextSize(1);
  display.setCursor(10, 40);
  display.print("Score: ");
  display.println(score);
  display.display();

  tone(BUZZER, 600, 300);  // Medium pitch
  delay(350);
  tone(BUZZER, 400, 300);  // Lower pitch
  delay(350);
  tone(BUZZER, 200, 500);  // Very low pitch for ending
  delay(550);
  noTone(BUZZER); 

  delay(3000);
  gameStarted = false;
  drawStartScreen();
}

// Setup runs once at startup
void setup() {
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_FIRE, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);

  // Initialize display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (true);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  randomSeed(analogRead(0));
  drawStartScreen();
}

// Main game loop
void loop() {
  if (!gameStarted) return;

  if (gameOver) {
    drawGameOverScreen();
    return;
  }

  // Read inputs
  if (!digitalRead(BTN_LEFT) && shipX > 0) shipX -= 2;
  if (!digitalRead(BTN_RIGHT) && shipX < SCREEN_WIDTH - 5) shipX += 2;
  if (!digitalRead(BTN_FIRE)) fireBullet();

  // Update bullets positions
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (bullets[i].active) {
      bullets[i].y -= 3;
      if (bullets[i].y < 0)
        bullets[i].active = false;
    }
  }

  // Update asteroids positions
  for (int i = 0; i < MAX_ASTEROIDS; i++) {
    if (asteroids[i].active) {
      asteroids[i].y += 1;
      // If asteroid exits screen bottom, reposition and decrease score
      if (asteroids[i].y > SCREEN_HEIGHT) {
        asteroids[i].y = random(-20, -5);
        asteroids[i].x = random(0, SCREEN_WIDTH);
        score -= 1;
      }

      // Check collision with ship
      if (abs(asteroids[i].x - shipX) < 4 && abs(asteroids[i].y - shipY) < 4) {
        gameOver = true;
        return;
      }

      // Check collision with bullets
      for (int j = 0; j < MAX_BULLETS; j++) {
        if (bullets[j].active && abs(asteroids[i].x - bullets[j].x) < 3 && abs(asteroids[i].y - bullets[j].y) < 3) {
          bullets[j].active = false;
          asteroids[i].y = random(-20, -5);
          asteroids[i].x = random(0, SCREEN_WIDTH);
          score += 2;
          playTone(1000, 30);
        }
      }
    }
  }

  // Draw the game frame
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  // Draw ship as triangle
  display.fillTriangle(
    shipX, shipY - 4,
    shipX - 4, shipY + 3,
    shipX + 4, shipY + 3,
    SSD1306_WHITE
  );

  // Draw bullets
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (bullets[i].active)
      display.drawPixel(bullets[i].x, bullets[i].y, SSD1306_WHITE);
  }

  // Draw asteroids as small circles
  for (int i = 0; i < MAX_ASTEROIDS; i++) {
    if (asteroids[i].active)
      display.drawCircle(asteroids[i].x, asteroids[i].y, 2, SSD1306_WHITE);
  }

  // Display current score
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.print("Score: ");
  display.print(score);

  display.display();
  delay(30);
}
