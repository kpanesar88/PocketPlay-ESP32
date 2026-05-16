#include <Arduino.h>

#include "button.hpp"
#include "tower.hpp"
#include "display.hpp"
#include "app.hpp"

const int TOWER_OPTION_COUNT = 2;
int selectedTowerOption = 0;

String towerOptions[TOWER_OPTION_COUNT] = {
  "PLAY",
  "BACK"
};

enum TowerState {
  TOWER_MENU,
  TOWER_PLAYING,
  TOWER_GAME_OVER
};

TowerState towerState = TOWER_MENU;

// Button states
bool towerLastUpState = HIGH;
bool towerLastDownState = HIGH;
bool towerLastSelectState = HIGH;

// Debounce
unsigned long towerLastUpPressTime = 0;
unsigned long towerLastDownPressTime = 0;
unsigned long towerLastSelectPressTime = 0;

const int towerDebounceDelay = 150;

// Game settings
const int towerBlockH = 8;
const int towerBottomY = 112;
const int towerTopLimitY = 22;

int towerBaseX = 40;
int towerBaseW = 80;

int towerMovingX = 0;
int towerMovingY = 0;
int towerOldMovingX = 0;
int towerMovingW = 80;

int towerDirection = 1;
int towerSpeed = 2;

int towerScore = 0;
int towerLevel = 1;

unsigned long towerLastFrameTime = 0;
int towerFrameDelay = 28;

// Placed blocks
const int TOWER_MAX_BLOCKS = 25;

int towerBlockX[TOWER_MAX_BLOCKS];
int towerBlockY[TOWER_MAX_BLOCKS];
int towerBlockW[TOWER_MAX_BLOCKS];
uint16_t towerBlockColor[TOWER_MAX_BLOCKS];

int towerPlacedCount = 0;

// Function declarations
void drawTowerMenu();
void drawTowerOption(int index, bool selected);
void drawTowerIcon(int x, int y);

void startTowerGame();
void updateTowerGame();

void drawTowerBlock(int x, int y, int w, uint16_t color);
void eraseTowerBlock(int x, int y, int w);

void drawTowerHud();
void placeTowerBlock();

void addPlacedTowerBlock(int x, int y, int w, uint16_t color);
void redrawPlacedTowerBlocks();
void scrollTowerDown();

uint16_t getTowerBlockColor(int level);
void updateTowerDifficulty();

void gameOverTower();

void initTower() {
  selectedTowerOption = 0;
  towerState = TOWER_MENU;

  towerLastUpState = HIGH;
  towerLastDownState = HIGH;
  towerLastSelectState = HIGH;

  towerLastUpPressTime = 0;
  towerLastDownPressTime = 0;
  towerLastSelectPressTime = 0;

  drawTowerMenu();
}

void updateTower() {
  bool currentUpState = digitalRead(UP_BUTTON_GPIO);
  bool currentDownState = digitalRead(DOWN_BUTTON_GPIO);
  bool currentSelectState = digitalRead(SELECT_BUTTON_GPIO);

  unsigned long currentTime = millis();

  if (towerState == TOWER_MENU) {
    if (
      currentDownState == LOW &&
      towerLastDownState == HIGH &&
      currentTime - towerLastDownPressTime > towerDebounceDelay
    ) {
      towerLastDownPressTime = currentTime;

      int oldOption = selectedTowerOption;

      selectedTowerOption++;

      if (selectedTowerOption >= TOWER_OPTION_COUNT) {
        selectedTowerOption = 0;
      }

      drawTowerOption(oldOption, false);
      drawTowerOption(selectedTowerOption, true);
    }

    if (
      currentUpState == LOW &&
      towerLastUpState == HIGH &&
      currentTime - towerLastUpPressTime > towerDebounceDelay
    ) {
      towerLastUpPressTime = currentTime;

      int oldOption = selectedTowerOption;

      selectedTowerOption--;

      if (selectedTowerOption < 0) {
        selectedTowerOption = TOWER_OPTION_COUNT - 1;
      }

      drawTowerOption(oldOption, false);
      drawTowerOption(selectedTowerOption, true);
    }

    if (
      currentSelectState == LOW &&
      towerLastSelectState == HIGH &&
      currentTime - towerLastSelectPressTime > towerDebounceDelay
    ) {
      towerLastSelectPressTime = currentTime;

      if (selectedTowerOption == 0) {
        Serial.println("Tower started");
        startTowerGame();
      } 
      else if (selectedTowerOption == 1) {
        Serial.println("Back to home");
        showHomeMenuScreen();
      }
    }

    towerLastUpState = currentUpState;
    towerLastDownState = currentDownState;
    towerLastSelectState = currentSelectState;

    return;
  }

  if (towerState == TOWER_PLAYING) {
    if (
      currentSelectState == LOW &&
      towerLastSelectState == HIGH &&
      currentTime - towerLastSelectPressTime > towerDebounceDelay
    ) {
      towerLastSelectPressTime = currentTime;
      placeTowerBlock();
    }

    if (currentTime - towerLastFrameTime >= towerFrameDelay) {
      towerLastFrameTime = currentTime;
      updateTowerGame();
    }
  }

  towerLastUpState = currentUpState;
  towerLastDownState = currentDownState;
  towerLastSelectState = currentSelectState;
}

void drawTowerMenu() {
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);

  String title = "TOWER";
  int titleWidth = title.length() * 6 * 2;
  int titleX = (tft.width() - titleWidth) / 2;

  tft.setCursor(titleX, 10);
  tft.println(title);

  drawTowerIcon(tft.width() / 2, 64);

  for (int i = 0; i < TOWER_OPTION_COUNT; i++) {
    drawTowerOption(i, i == selectedTowerOption);
  }
}

void drawTowerOption(int index, bool selected) {
  int y = 100 + (index * 16);

  tft.fillRect(0, y, tft.width(), 14, ST77XX_BLACK);

  String optionText;

  if (selected) {
    optionText = "> " + towerOptions[index];
    tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  } else {
    optionText = "  " + towerOptions[index];
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  }

  int textWidth = optionText.length() * 6;
  int x = (tft.width() - textWidth) / 2;

  tft.setCursor(x, y);
  tft.setTextSize(1);
  tft.print(optionText);
}

void drawTowerIcon(int x, int y) {
  tft.fillRect(x - 28, y + 18, 56, 8, ST77XX_BLUE);
  tft.fillRect(x - 22, y + 8, 44, 8, ST77XX_CYAN);
  tft.fillRect(x - 16, y - 2, 32, 8, ST77XX_GREEN);
  tft.fillRect(x - 10, y - 12, 20, 8, ST77XX_YELLOW);

  tft.drawRect(x - 35, y - 18, 70, 50, ST77XX_WHITE);
}
void startTowerGame() {
  towerState = TOWER_PLAYING;

  towerBaseW = 80;
  towerBaseX = (tft.width() - towerBaseW) / 2;

  towerMovingW = towerBaseW;
  towerMovingX = 0;
  towerOldMovingX = towerMovingX;

  towerMovingY = towerBottomY - towerBlockH;

  towerDirection = 1;
  towerSpeed = 2;

  towerScore = 0;
  towerLevel = 1;
  towerFrameDelay = 28;

  towerPlacedCount = 0;

  tft.fillScreen(ST77XX_BLACK);

  drawTowerHud();

  addPlacedTowerBlock(towerBaseX, towerBottomY, towerBaseW, ST77XX_BLUE);
  drawTowerBlock(towerBaseX, towerBottomY, towerBaseW, ST77XX_BLUE);

  drawTowerBlock(towerMovingX, towerMovingY, towerMovingW, ST77XX_GREEN);

  towerLastSelectState = HIGH;
  towerLastSelectPressTime = 0;
  towerLastFrameTime = millis();
}

void updateTowerGame() {
  towerOldMovingX = towerMovingX;

  towerMovingX += towerDirection * towerSpeed;

  if (towerMovingX <= 0) {
    towerMovingX = 0;
    towerDirection = 1;
  }

  if (towerMovingX + towerMovingW >= tft.width()) {
    towerMovingX = tft.width() - towerMovingW;
    towerDirection = -1;
  }

  eraseTowerBlock(towerOldMovingX, towerMovingY, towerMovingW);
  drawTowerBlock(towerMovingX, towerMovingY, towerMovingW, ST77XX_GREEN);
}

void placeTowerBlock() {
  int overlapLeft = max(towerMovingX, towerBaseX);
  int overlapRight = min(towerMovingX + towerMovingW, towerBaseX + towerBaseW);
  int overlapWidth = overlapRight - overlapLeft;

  if (overlapWidth <= 0) {
    gameOverTower();
    return;
  }

  eraseTowerBlock(towerMovingX, towerMovingY, towerMovingW);

  towerBaseX = overlapLeft;
  towerBaseW = overlapWidth;

  uint16_t blockColor = getTowerBlockColor(towerLevel);

  addPlacedTowerBlock(towerBaseX, towerMovingY, towerBaseW, blockColor);
  drawTowerBlock(towerBaseX, towerMovingY, towerBaseW, blockColor);

  towerScore++;
  towerLevel++;

  updateTowerDifficulty();
  drawTowerHud();

  if (towerBaseW <= 6) {
    gameOverTower();
    return;
  }

  towerMovingY -= towerBlockH;

  if (towerMovingY < towerTopLimitY) {
    scrollTowerDown();
    towerMovingY += towerBlockH;
  }

  towerMovingW = towerBaseW;

  if (towerLevel % 2 == 0) {
    towerMovingX = 0;
    towerDirection = 1;
  } else {
    towerMovingX = tft.width() - towerMovingW;
    towerDirection = -1;
  }

  towerOldMovingX = towerMovingX;

  drawTowerBlock(towerMovingX, towerMovingY, towerMovingW, ST77XX_GREEN);
}

void addPlacedTowerBlock(int x, int y, int w, uint16_t color) {
  if (towerPlacedCount >= TOWER_MAX_BLOCKS) {
    for (int i = 1; i < TOWER_MAX_BLOCKS; i++) {
      towerBlockX[i - 1] = towerBlockX[i];
      towerBlockY[i - 1] = towerBlockY[i];
      towerBlockW[i - 1] = towerBlockW[i];
      towerBlockColor[i - 1] = towerBlockColor[i];
    }

    towerPlacedCount = TOWER_MAX_BLOCKS - 1;
  }

  towerBlockX[towerPlacedCount] = x;
  towerBlockY[towerPlacedCount] = y;
  towerBlockW[towerPlacedCount] = w;
  towerBlockColor[towerPlacedCount] = color;

  towerPlacedCount++;
}

void redrawPlacedTowerBlocks() {
  for (int i = 0; i < towerPlacedCount; i++) {
    if (towerBlockY[i] >= 14 && towerBlockY[i] <= tft.height()) {
      drawTowerBlock(towerBlockX[i], towerBlockY[i], towerBlockW[i], towerBlockColor[i]);
    }
  }
}

void scrollTowerDown() {
  tft.fillRect(0, 14, tft.width(), tft.height() - 14, ST77XX_BLACK);

  for (int i = 0; i < towerPlacedCount; i++) {
    towerBlockY[i] += towerBlockH;
  }

  redrawPlacedTowerBlocks();
  drawTowerHud();
}

uint16_t getTowerBlockColor(int level) {
  if (level % 5 == 0) {
    return ST77XX_MAGENTA;
  } 
  else if (level % 5 == 1) {
    return ST77XX_CYAN;
  } 
  else if (level % 5 == 2) {
    return ST77XX_GREEN;
  } 
  else if (level % 5 == 3) {
    return ST77XX_YELLOW;
  } 
  else {
    return ST77XX_BLUE;
  }
}

void updateTowerDifficulty() {
  if (towerScore % 3 == 0 && towerSpeed < 8) {
    towerSpeed++;
  }

  if (towerScore % 5 == 0 && towerFrameDelay > 13) {
    towerFrameDelay -= 2;
  }
}

void drawTowerBlock(int x, int y, int w, uint16_t color) {
  tft.fillRect(x, y, w, towerBlockH, color);
  tft.drawRect(x, y, w, towerBlockH, ST77XX_WHITE);
}

void eraseTowerBlock(int x, int y, int w) {
  tft.fillRect(x - 1, y - 1, w + 2, towerBlockH + 2, ST77XX_BLACK);
}

void drawTowerHud() {
  tft.fillRect(0, 0, tft.width(), 14, ST77XX_BLACK);

  tft.setTextSize(1);

  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
  tft.setCursor(2, 3);
  tft.print("S:");
  tft.print(towerScore);

  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(48, 3);
  tft.print("LVL:");
  tft.print(towerLevel);

  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
  tft.setCursor(100, 3);
  tft.print("V:");
  tft.print(towerSpeed);
}

void gameOverTower() {
  towerState = TOWER_GAME_OVER;

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
  tft.print(towerScore);

  delay(2000);

  showPlayAgainScreen();
}