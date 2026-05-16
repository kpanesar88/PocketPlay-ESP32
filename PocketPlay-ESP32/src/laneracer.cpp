#include <Arduino.h>

#include "button.hpp"
#include "laneracer.hpp"
#include "display.hpp"
#include "app.hpp"

const int LANE_COUNT = 3;

// Menu
const int LANE_OPTION_COUNT = 2;
int selectedLaneOption = 0;

String laneOptions[LANE_OPTION_COUNT] = {
  "PLAY",
  "BACK"
};

enum LaneRacerState {
  LANE_MENU,
  LANE_PLAYING
};

LaneRacerState laneRacerState = LANE_MENU;

// Game variables
int playerLane = 1;
int oldPlayerLane = 1;

int enemyLane = 0;
int enemyY = -30;
int oldEnemyY = -30;

int score = 0;
int enemySpeed = 5;

unsigned long lastFrameTime = 0;

const int frameDelay = 25;

const int carWidth = 20;
const int carHeight = 24;

// Button edge detection
bool lastLeftState = HIGH;
bool lastRightState = HIGH;
bool lastUpState = HIGH;
bool lastDownState = HIGH;
bool lastSelectState = HIGH;

// Function declarations
void drawLaneRacerMenu();
void drawLaneRacerOption(int index, bool selected);
void drawMenuCar(int x, int y);
void startLaneRacerGame();

void drawRoad();
void redrawLaneLinesInArea(int y, int h);
void drawPlayerCar(int lane, uint16_t color);
void drawEnemyCar(int lane, int y, uint16_t color);
void erasePlayerCar(int lane);
void eraseEnemyCar(int lane, int y);
void resetEnemy();
int getLaneCenterX(int lane);
bool checkCollision();
void gameOverLaneRacer();

void initLaneRacer() {
  selectedLaneOption = 0;
  laneRacerState = LANE_MENU;

  lastLeftState = HIGH;
  lastRightState = HIGH;
  lastUpState = HIGH;
  lastDownState = HIGH;
  lastSelectState = HIGH;

  drawLaneRacerMenu();
}

void updateLaneRacer() {
  bool currentLeftState = digitalRead(LEFT_BUTTON_GPIO);
  bool currentRightState = digitalRead(RIGHT_BUTTON_GPIO);
  bool currentUpState = digitalRead(UP_BUTTON_GPIO);
  bool currentDownState = digitalRead(DOWN_BUTTON_GPIO);
  bool currentSelectState = digitalRead(SELECT_BUTTON_GPIO);

  if (laneRacerState == LANE_MENU) {
    if (currentDownState == LOW && lastDownState == HIGH) {
      int oldSelectedOption = selectedLaneOption;

      selectedLaneOption++;

      if (selectedLaneOption >= LANE_OPTION_COUNT) {
        selectedLaneOption = 0;
      }

      drawLaneRacerOption(oldSelectedOption, false);
      drawLaneRacerOption(selectedLaneOption, true);
    }

    if (currentUpState == LOW && lastUpState == HIGH) {
      int oldSelectedOption = selectedLaneOption;

      selectedLaneOption--;

      if (selectedLaneOption < 0) {
        selectedLaneOption = LANE_OPTION_COUNT - 1;
      }

      drawLaneRacerOption(oldSelectedOption, false);
      drawLaneRacerOption(selectedLaneOption, true);
    }

    if (currentSelectState == LOW && lastSelectState == HIGH) {
      if (selectedLaneOption == 0) {
        Serial.println("Lane Racer started");
        startLaneRacerGame();
      } 
      else if (selectedLaneOption == 1) {
        Serial.println("Back to home");
        showHomeMenuScreen();
      }
    }
  }

  else if (laneRacerState == LANE_PLAYING) {
    if (currentLeftState == LOW && lastLeftState == HIGH) {
      oldPlayerLane = playerLane;

      playerLane--;

      if (playerLane < 0) {
        playerLane = 0;
      }

      if (oldPlayerLane != playerLane) {
        erasePlayerCar(oldPlayerLane);
        drawPlayerCar(playerLane, ST77XX_GREEN);
      }
    }

    if (currentRightState == LOW && lastRightState == HIGH) {
      oldPlayerLane = playerLane;

      playerLane++;

      if (playerLane >= LANE_COUNT) {
        playerLane = LANE_COUNT - 1;
      }

      if (oldPlayerLane != playerLane) {
        erasePlayerCar(oldPlayerLane);
        drawPlayerCar(playerLane, ST77XX_GREEN);
      }
    }

    if (millis() - lastFrameTime >= frameDelay) {
      lastFrameTime = millis();

      oldEnemyY = enemyY;
      enemyY += enemySpeed;

      eraseEnemyCar(enemyLane, oldEnemyY);
      drawEnemyCar(enemyLane, enemyY, ST77XX_RED);

      if (checkCollision()) {
        gameOverLaneRacer();
        return;
      }

      if (enemyY > tft.height() + 10) {
        eraseEnemyCar(enemyLane, enemyY);

        score++;

        if (score % 5 == 0) {
          enemySpeed++;
        }

        resetEnemy();
        drawEnemyCar(enemyLane, enemyY, ST77XX_RED);
      }
    }
  }

  lastLeftState = currentLeftState;
  lastRightState = currentRightState;
  lastUpState = currentUpState;
  lastDownState = currentDownState;
  lastSelectState = currentSelectState;
}

void drawLaneRacerMenu() {
  tft.fillScreen(ST77XX_BLACK);

  // Centered title: LANE
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);

  String title1 = "LANE";
  int title1Width = title1.length() * 6 * 2;
  int title1X = (tft.width() - title1Width) / 2;

  tft.setCursor(title1X, 6);
  tft.println(title1);

  // Centered title: RACER
  String title2 = "RACER";
  int title2Width = title2.length() * 6 * 2;
  int title2X = (tft.width() - title2Width) / 2;

  tft.setCursor(title2X, 28);
  tft.println(title2);

  // Centered side-view car under title
  drawMenuCar(tft.width() / 2, 60);

  // Centered PLAY / BACK options
  for (int i = 0; i < LANE_OPTION_COUNT; i++) {
    drawLaneRacerOption(i, i == selectedLaneOption);
  }
}
void drawLaneRacerOption(int index, bool selected) {
  int y = 102 + (index * 16);

  tft.fillRect(0, y, tft.width(), 14, ST77XX_BLACK);

  String optionText;

  if (selected) {
    optionText = "> " + laneOptions[index];
    tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  } else {
    optionText = "  " + laneOptions[index];
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  }

  tft.setTextSize(1);

  int textWidth = optionText.length() * 6;
  int x = (tft.width() - textWidth) / 2;

  tft.setCursor(x, y);
  tft.print(optionText);
}
void drawMenuCar(int x, int y) {
  // Side-view car centered around x

  // Body
  tft.fillRect(x - 18, y + 8, 36, 10, ST77XX_GREEN);

  // Roof
  tft.fillRect(x - 8, y, 16, 9, ST77XX_GREEN);

  // Windows
  tft.fillRect(x - 6, y + 2, 5, 5, ST77XX_CYAN);
  tft.fillRect(x + 2, y + 2, 5, 5, ST77XX_CYAN);

  // Wheels
  tft.fillCircle(x - 11, y + 20, 4, ST77XX_WHITE);
  tft.fillCircle(x + 11, y + 20, 4, ST77XX_WHITE);

  // Wheel centers
  tft.fillCircle(x - 11, y + 20, 2, ST77XX_BLACK);
  tft.fillCircle(x + 11, y + 20, 2, ST77XX_BLACK);

  // Headlight
  tft.fillRect(x + 16, y + 11, 2, 3, ST77XX_YELLOW);

  // Tail light
  tft.fillRect(x - 18, y + 11, 2, 3, ST77XX_RED);
}
void startLaneRacerGame() {
  laneRacerState = LANE_PLAYING;

  playerLane = 1;
  oldPlayerLane = 1;

  score = 0;
  enemySpeed = 5;

  lastLeftState = HIGH;
  lastRightState = HIGH;
  lastUpState = HIGH;
  lastDownState = HIGH;
  lastSelectState = HIGH;

  resetEnemy();

  tft.fillScreen(ST77XX_BLACK);

  drawRoad();

  drawPlayerCar(playerLane, ST77XX_GREEN);
  drawEnemyCar(enemyLane, enemyY, ST77XX_RED);

  lastFrameTime = millis();
}

void drawRoad() {
  int screenWidth = tft.width();
  int screenHeight = tft.height();
  int laneWidth = screenWidth / LANE_COUNT;

  for (int i = 1; i < LANE_COUNT; i++) {
    int x = i * laneWidth;
    tft.drawLine(x, 0, x, screenHeight, ST77XX_WHITE);
  }
}

void redrawLaneLinesInArea(int y, int h) {
  int screenWidth = tft.width();
  int laneWidth = screenWidth / LANE_COUNT;

  for (int i = 1; i < LANE_COUNT; i++) {
    int x = i * laneWidth;
    tft.drawLine(x, y, x, y + h, ST77XX_WHITE);
  }
}

void drawPlayerCar(int lane, uint16_t color) {
  int x = getLaneCenterX(lane);
  int y = tft.height() - 28;

  tft.fillRect(x - 8, y, 16, 22, color);
  tft.fillRect(x - 5, y - 6, 10, 8, color);

  tft.fillRect(x - 12, y + 3, 4, 6, ST77XX_WHITE);
  tft.fillRect(x + 8, y + 3, 4, 6, ST77XX_WHITE);
  tft.fillRect(x - 12, y + 14, 4, 6, ST77XX_WHITE);
  tft.fillRect(x + 8, y + 14, 4, 6, ST77XX_WHITE);
}

void erasePlayerCar(int lane) {
  int x = getLaneCenterX(lane);
  int y = tft.height() - 36;

  tft.fillRect(x - 15, y, 30, 38, ST77XX_BLACK);
  redrawLaneLinesInArea(y, 38);
}

void drawEnemyCar(int lane, int y, uint16_t color) {
  if (y < -carHeight || y > tft.height()) {
    return;
  }

  int x = getLaneCenterX(lane);

  tft.fillRect(x - 8, y, 16, 22, color);
  tft.fillRect(x - 5, y - 6, 10, 8, color);

  tft.fillRect(x - 12, y + 3, 4, 6, ST77XX_WHITE);
  tft.fillRect(x + 8, y + 3, 4, 6, ST77XX_WHITE);
  tft.fillRect(x - 12, y + 14, 4, 6, ST77XX_WHITE);
  tft.fillRect(x + 8, y + 14, 4, 6, ST77XX_WHITE);
}

void eraseEnemyCar(int lane, int y) {
  if (y < -40 || y > tft.height()) {
    return;
  }

  int x = getLaneCenterX(lane);

  tft.fillRect(x - 15, y - 8, 30, 40, ST77XX_BLACK);
  redrawLaneLinesInArea(y - 8, 40);
}

void resetEnemy() {
  enemyLane = random(0, LANE_COUNT);
  enemyY = -30;
  oldEnemyY = -30;
}

int getLaneCenterX(int lane) {
  int laneWidth = tft.width() / LANE_COUNT;
  return (lane * laneWidth) + (laneWidth / 2);
}

bool checkCollision() {
  if (enemyLane != playerLane) {
    return false;
  }

  int playerY = tft.height() - 28;

  int playerTop = playerY + 3;
  int playerBottom = playerY + 22;

  int enemyTop = enemyY + 3;
  int enemyBottom = enemyY + 22;

  bool verticalOverlap = enemyBottom >= playerTop && enemyTop <= playerBottom;

  return verticalOverlap;
}

void gameOverLaneRacer() {
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
  tft.setCursor(20, 25);
  tft.println("CRASH");

  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
  tft.setCursor(15, 60);
  tft.print("SCORE");

  tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  tft.setCursor(95, 60);
  tft.print(score);

  delay(2000);

  showPlayAgainScreen();
}