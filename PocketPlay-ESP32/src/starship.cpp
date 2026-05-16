#include <Arduino.h>

#include "button.hpp"
#include "starship.hpp"
#include "display.hpp"
#include "app.hpp"

const int SS_OPTION_COUNT = 2;
int selectedSSOption = 0;

String ssOptions[SS_OPTION_COUNT] = {
  "PLAY",
  "BACK"
};

enum StarshipState {
  SS_MENU,
  SS_PLAYING,
  SS_GAME_OVER
};

StarshipState ssState = SS_MENU;

// Button states
bool ssLastUpState = HIGH;
bool ssLastDownState = HIGH;
bool ssLastLeftState = HIGH;
bool ssLastRightState = HIGH;
bool ssLastSelectState = HIGH;

// Debounce for menu/select
unsigned long ssLastUpPressTime = 0;
unsigned long ssLastDownPressTime = 0;
unsigned long ssLastSelectPressTime = 0;

const int ssDebounceDelay = 150;

// Sliding movement
unsigned long ssLastShipMoveTime = 0;

// Player movement
int ssShipMoveDelay = 18;
int ssShipSpeed = 3;

const int ssMaxShipSpeed = 6;
const int ssMinShipMoveDelay = 10;

// Player ship
int ssPlayerX = 80;
int ssOldPlayerX = 80;
int ssPlayerY = 108;

// Bullet
int ssBulletX = 0;
int ssBulletY = 0;
int ssOldBulletY = 0;
bool ssBulletActive = false;

// Multiple enemies
const int SS_ENEMY_COUNT = 3;

int ssEnemyX[SS_ENEMY_COUNT];
int ssEnemyY[SS_ENEMY_COUNT];
int ssOldEnemyX[SS_ENEMY_COUNT];
int ssOldEnemyY[SS_ENEMY_COUNT];

int ssEnemySpeed = 1;
int ssScore = 0;

// Timing
unsigned long ssLastFrameTime = 0;
const int ssFrameDelay = 42;

// Speed increase over time
unsigned long ssLastSpeedIncreaseTime = 0;
const int ssSpeedIncreaseDelay = 7000; // every 7 seconds
const int ssMaxEnemySpeed = 4;

// Function declarations
void drawStarshipMenu();
void drawStarshipOption(int index, bool selected);
void drawStarshipIcon(int x, int y);
static void drawCenteredText(String text, int y, int textSize, uint16_t color);

void startStarshipGame();
void updateStarshipGame();

void moveStarshipLeft();
void moveStarshipRight();

void drawPlayerShip(int x, int y, uint16_t color);
void erasePlayerShip(int x, int y);

void drawEnemyShip(int x, int y, uint16_t color);
void eraseEnemyShip(int x, int y);

void drawStarshipBullet(int x, int y);
void eraseStarshipBullet(int x, int y);

void resetStarshipEnemy(int index);
void drawStarshipHud();

bool starshipBulletHitEnemy(int index);
bool starshipEnemyHitPlayer(int index);

void gameOverStarship();

void initStarship() {
  selectedSSOption = 0;
  ssState = SS_MENU;

  ssLastUpState = HIGH;
  ssLastDownState = HIGH;
  ssLastLeftState = HIGH;
  ssLastRightState = HIGH;
  ssLastSelectState = HIGH;

  ssLastUpPressTime = 0;
  ssLastDownPressTime = 0;
  ssLastSelectPressTime = 0;
  ssLastShipMoveTime = 0;

  drawStarshipMenu();
}

void updateStarship() {
  bool currentUpState = digitalRead(UP_BUTTON_GPIO);
  bool currentDownState = digitalRead(DOWN_BUTTON_GPIO);
  bool currentLeftState = digitalRead(LEFT_BUTTON_GPIO);
  bool currentRightState = digitalRead(RIGHT_BUTTON_GPIO);
  bool currentSelectState = digitalRead(SELECT_BUTTON_GPIO);

  unsigned long currentTime = millis();

  if (ssState == SS_MENU) {
    if (
      currentDownState == LOW &&
      ssLastDownState == HIGH &&
      currentTime - ssLastDownPressTime > ssDebounceDelay
    ) {
      ssLastDownPressTime = currentTime;

      int oldOption = selectedSSOption;

      selectedSSOption++;

      if (selectedSSOption >= SS_OPTION_COUNT) {
        selectedSSOption = 0;
      }

      drawStarshipOption(oldOption, false);
      drawStarshipOption(selectedSSOption, true);
    }

    if (
      currentUpState == LOW &&
      ssLastUpState == HIGH &&
      currentTime - ssLastUpPressTime > ssDebounceDelay
    ) {
      ssLastUpPressTime = currentTime;

      int oldOption = selectedSSOption;

      selectedSSOption--;

      if (selectedSSOption < 0) {
        selectedSSOption = SS_OPTION_COUNT - 1;
      }

      drawStarshipOption(oldOption, false);
      drawStarshipOption(selectedSSOption, true);
    }

    if (
      currentSelectState == LOW &&
      ssLastSelectState == HIGH &&
      currentTime - ssLastSelectPressTime > ssDebounceDelay
    ) {
      ssLastSelectPressTime = currentTime;

      if (selectedSSOption == 0) {
        Serial.println("Starship started");
        startStarshipGame();
      } 
      else if (selectedSSOption == 1) {
        Serial.println("Back to home");
        showHomeMenuScreen();
      }
    }

    ssLastUpState = currentUpState;
    ssLastDownState = currentDownState;
    ssLastSelectState = currentSelectState;

    return;
  }

  if (ssState == SS_PLAYING) {
    // Hold left/right to slide
    if (currentTime - ssLastShipMoveTime >= ssShipMoveDelay) {
      if (currentLeftState == LOW) {
        moveStarshipLeft();
        ssLastShipMoveTime = currentTime;
      }

      if (currentRightState == LOW) {
        moveStarshipRight();
        ssLastShipMoveTime = currentTime;
      }
    }

    // Shoot once per SELECT press
    if (
      currentSelectState == LOW &&
      ssLastSelectState == HIGH &&
      currentTime - ssLastSelectPressTime > ssDebounceDelay
    ) {
      ssLastSelectPressTime = currentTime;

      if (!ssBulletActive) {
        ssBulletActive = true;
        ssBulletX = ssPlayerX;
        ssBulletY = ssPlayerY - 9;
        ssOldBulletY = ssBulletY;

        drawStarshipBullet(ssBulletX, ssBulletY);
      }
    }

    if (currentTime - ssLastFrameTime >= ssFrameDelay) {
      ssLastFrameTime = currentTime;
      updateStarshipGame();
    }
  }

  ssLastUpState = currentUpState;
  ssLastDownState = currentDownState;
  ssLastLeftState = currentLeftState;
  ssLastRightState = currentRightState;
  ssLastSelectState = currentSelectState;
}

void drawStarshipMenu() {
  tft.fillScreen(ST77XX_BLACK);

  drawCenteredText("STARSHIP", 10, 2, ST77XX_YELLOW);

  drawStarshipIcon(tft.width() / 2, 62);

  for (int i = 0; i < SS_OPTION_COUNT; i++) {
    drawStarshipOption(i, i == selectedSSOption);
  }
}

static void drawCenteredText(String text, int y, int textSize, uint16_t color) {
  int textWidth = text.length() * 6 * textSize;
  int x = (tft.width() - textWidth) / 2;

  tft.setCursor(x, y);
  tft.setTextSize(textSize);
  tft.setTextColor(color, ST77XX_BLACK);
  tft.print(text);
}

void drawStarshipOption(int index, bool selected) {
  int y = 98 + (index * 16);

  tft.fillRect(0, y, tft.width(), 14, ST77XX_BLACK);

  String optionText;

  if (selected) {
    optionText = "> " + ssOptions[index];
    tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  } else {
    optionText = "  " + ssOptions[index];
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  }

  int textWidth = optionText.length() * 6;
  int x = (tft.width() - textWidth) / 2;

  tft.setCursor(x, y);
  tft.setTextSize(1);
  tft.print(optionText);
}

void drawStarshipIcon(int x, int y) {
  // Center player ship icon
  tft.fillTriangle(x, y - 12, x - 7, y + 5, x + 7, y + 5, ST77XX_CYAN);
  tft.fillRect(x - 2, y - 2, 4, 11, ST77XX_CYAN);
  tft.fillTriangle(x - 2, y + 5, x - 9, y + 12, x - 2, y + 12, ST77XX_CYAN);
  tft.fillTriangle(x + 2, y + 5, x + 9, y + 12, x + 2, y + 12, ST77XX_CYAN);

  // Bullet
  tft.fillRect(x - 1, y - 25, 3, 6, ST77XX_WHITE);

  // Small enemies around it
  tft.fillTriangle(x - 36, y - 4, x - 42, y - 16, x - 30, y - 16, ST77XX_RED);
  tft.fillRect(x - 38, y - 17, 5, 8, ST77XX_RED);

  tft.fillTriangle(x + 36, y - 4, x + 30, y - 16, x + 42, y - 16, ST77XX_RED);
  tft.fillRect(x + 34, y - 17, 5, 8, ST77XX_RED);
}

void startStarshipGame() {
  ssState = SS_PLAYING;

  ssPlayerX = tft.width() / 2;
  ssOldPlayerX = ssPlayerX;
  ssPlayerY = tft.height() - 18;

  ssBulletActive = false;

  ssEnemySpeed = 1;

  // Reset player movement speed every new game
  ssShipSpeed = 3;
  ssShipMoveDelay = 18;

  ssScore = 0;
  ssLastSpeedIncreaseTime = millis();

  for (int i = 0; i < SS_ENEMY_COUNT; i++) {
    ssEnemyX[i] = random(12, tft.width() - 12);
    ssEnemyY[i] = -20 - (i * 45);

    ssOldEnemyX[i] = ssEnemyX[i];
    ssOldEnemyY[i] = ssEnemyY[i];
  }

  ssLastLeftState = HIGH;
  ssLastRightState = HIGH;
  ssLastSelectState = HIGH;

  ssLastSelectPressTime = 0;
  ssLastShipMoveTime = 0;

  tft.fillScreen(ST77XX_BLACK);

  drawStarshipHud();
  drawPlayerShip(ssPlayerX, ssPlayerY, ST77XX_CYAN);

  for (int i = 0; i < SS_ENEMY_COUNT; i++) {
    drawEnemyShip(ssEnemyX[i], ssEnemyY[i], ST77XX_RED);
  }

  ssLastFrameTime = millis();
}

void updateStarshipGame() {
  unsigned long currentTime = millis();

  // Slowly increase difficulty over time
  if (
    currentTime - ssLastSpeedIncreaseTime >= ssSpeedIncreaseDelay &&
    ssEnemySpeed < ssMaxEnemySpeed
  ) {
    ssLastSpeedIncreaseTime = currentTime;

    // Enemies get faster
    ssEnemySpeed++;

    // Player ship also gets faster
    if (ssShipSpeed < ssMaxShipSpeed) {
      ssShipSpeed++;
    }

    // Player movement refreshes faster
    if (ssShipMoveDelay > ssMinShipMoveDelay) {
      ssShipMoveDelay -= 2;
    }

    drawStarshipHud();

    Serial.print("Enemy speed increased to: ");
    Serial.println(ssEnemySpeed);

    Serial.print("Player speed increased to: ");
    Serial.println(ssShipSpeed);

    Serial.print("Ship move delay changed to: ");
    Serial.println(ssShipMoveDelay);
  }

  for (int i = 0; i < SS_ENEMY_COUNT; i++) {
    ssOldEnemyX[i] = ssEnemyX[i];
    ssOldEnemyY[i] = ssEnemyY[i];

    ssEnemyY[i] += ssEnemySpeed;

    eraseEnemyShip(ssOldEnemyX[i], ssOldEnemyY[i]);
    drawEnemyShip(ssEnemyX[i], ssEnemyY[i], ST77XX_RED);
  }

  if (ssBulletActive) {
    ssOldBulletY = ssBulletY;
    ssBulletY -= 7;

    eraseStarshipBullet(ssBulletX, ssOldBulletY);

    if (ssBulletY < 14) {
      ssBulletActive = false;
    } else {
      drawStarshipBullet(ssBulletX, ssBulletY);
    }
  }

  if (ssBulletActive) {
    for (int i = 0; i < SS_ENEMY_COUNT; i++) {
      if (starshipBulletHitEnemy(i)) {
        eraseStarshipBullet(ssBulletX, ssBulletY);
        eraseEnemyShip(ssEnemyX[i], ssEnemyY[i]);

        ssBulletActive = false;
        ssScore++;

        resetStarshipEnemy(i);
        drawStarshipHud();
        drawEnemyShip(ssEnemyX[i], ssEnemyY[i], ST77XX_RED);

        break;
      }
    }
  }

  for (int i = 0; i < SS_ENEMY_COUNT; i++) {
    if (ssEnemyY[i] > tft.height() + 12 || starshipEnemyHitPlayer(i)) {
      gameOverStarship();
      return;
    }
  }
}

void moveStarshipLeft() {
  ssOldPlayerX = ssPlayerX;
  ssPlayerX -= ssShipSpeed;

  if (ssPlayerX < 9) {
    ssPlayerX = 9;
  }

  if (ssOldPlayerX != ssPlayerX) {
    erasePlayerShip(ssOldPlayerX, ssPlayerY);
    drawPlayerShip(ssPlayerX, ssPlayerY, ST77XX_CYAN);
  }
}

void moveStarshipRight() {
  ssOldPlayerX = ssPlayerX;
  ssPlayerX += ssShipSpeed;

  if (ssPlayerX > tft.width() - 9) {
    ssPlayerX = tft.width() - 9;
  }

  if (ssOldPlayerX != ssPlayerX) {
    erasePlayerShip(ssOldPlayerX, ssPlayerY);
    drawPlayerShip(ssPlayerX, ssPlayerY, ST77XX_CYAN);
  }
}

void drawPlayerShip(int x, int y, uint16_t color) {
  // Smaller player ship
  tft.fillTriangle(x, y - 8, x - 5, y + 3, x + 5, y + 3, color);
  tft.fillRect(x - 1, y - 2, 3, 9, color);

  tft.fillTriangle(x - 1, y + 2, x - 8, y + 7, x - 1, y + 7, color);
  tft.fillTriangle(x + 2, y + 2, x + 9, y + 7, x + 2, y + 7, color);

  tft.fillRect(x - 4, y + 7, 8, 2, color);
}

void erasePlayerShip(int x, int y) {
  tft.fillRect(x - 10, y - 10, 21, 21, ST77XX_BLACK);
}

void drawEnemyShip(int x, int y, uint16_t color) {
  if (y < 14 || y > tft.height() + 20) {
    return;
  }

  // Smaller enemy ship
  tft.fillTriangle(x, y + 7, x - 5, y - 3, x + 5, y - 3, color);
  tft.fillRect(x - 1, y - 7, 3, 8, color);

  tft.fillTriangle(x - 1, y - 1, x - 7, y - 6, x - 1, y - 6, color);
  tft.fillTriangle(x + 2, y - 1, x + 8, y - 6, x + 2, y - 6, color);

  tft.fillRect(x - 4, y - 8, 8, 2, color);
}

void eraseEnemyShip(int x, int y) {
  if (y < 14 || y > tft.height() + 20) {
    return;
  }

  tft.fillRect(x - 9, y - 9, 19, 18, ST77XX_BLACK);
}

void drawStarshipBullet(int x, int y) {
  tft.fillRect(x - 1, y - 5, 3, 7, ST77XX_WHITE);
}

void eraseStarshipBullet(int x, int y) {
  tft.fillRect(x - 2, y - 6, 5, 9, ST77XX_BLACK);
}

void resetStarshipEnemy(int index) {
  ssEnemyX[index] = random(12, tft.width() - 12);
  ssEnemyY[index] = random(-90, -25);

  ssOldEnemyX[index] = ssEnemyX[index];
  ssOldEnemyY[index] = ssEnemyY[index];
}

void drawStarshipHud() {
  tft.fillRect(0, 0, tft.width(), 12, ST77XX_BLACK);

  tft.setTextSize(1);

  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
  tft.setCursor(2, 2);
  tft.print("S:");
  tft.print(ssScore);

  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(45, 2);
  tft.print("E:");
  tft.print(ssEnemySpeed);

  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
  tft.setCursor(85, 2);
  tft.print("P:");
  tft.print(ssShipSpeed);
}

bool starshipBulletHitEnemy(int index) {
  int bulletLeft = ssBulletX - 2;
  int bulletRight = ssBulletX + 2;
  int bulletTop = ssBulletY - 6;
  int bulletBottom = ssBulletY + 3;

  int enemyLeft = ssEnemyX[index] - 8;
  int enemyRight = ssEnemyX[index] + 8;
  int enemyTop = ssEnemyY[index] - 8;
  int enemyBottom = ssEnemyY[index] + 8;

  bool xOverlap = bulletRight >= enemyLeft && bulletLeft <= enemyRight;
  bool yOverlap = bulletBottom >= enemyTop && bulletTop <= enemyBottom;

  return xOverlap && yOverlap;
}

bool starshipEnemyHitPlayer(int index) {
  int enemyLeft = ssEnemyX[index] - 8;
  int enemyRight = ssEnemyX[index] + 8;
  int enemyTop = ssEnemyY[index] - 8;
  int enemyBottom = ssEnemyY[index] + 8;

  int playerLeft = ssPlayerX - 8;
  int playerRight = ssPlayerX + 8;
  int playerTop = ssPlayerY - 8;
  int playerBottom = ssPlayerY + 8;

  bool xOverlap = enemyRight >= playerLeft && enemyLeft <= playerRight;
  bool yOverlap = enemyBottom >= playerTop && enemyTop <= playerBottom;

  return xOverlap && yOverlap;
}

void gameOverStarship() {
  ssState = SS_GAME_OVER;

  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
  tft.setCursor(10, 25);
  tft.println("GAME OVER");

  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
  tft.setCursor(15, 60);
  tft.print("SCORE");

  tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  tft.setCursor(95, 60);
  tft.print(ssScore);

  delay(2000);

  showPlayAgainScreen();
}