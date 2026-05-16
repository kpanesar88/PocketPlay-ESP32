#include <Arduino.h>

#include "button.hpp"
#include "snake.hpp"
#include "display.hpp"
#include "app.hpp"

const int SNAKE_OPTION_COUNT = 2;
int selectedSnakeOption = 0;

String snakeOptions[SNAKE_OPTION_COUNT] = {
  "PLAY",
  "BACK"
};

enum SnakeState {
  SNAKE_MENU,
  SNAKE_PLAYING
};

SnakeState snakeState = SNAKE_MENU;

// Button edge detection
bool snakeLastUpState = HIGH;
bool snakeLastDownState = HIGH;
bool snakeLastLeftState = HIGH;
bool snakeLastRightState = HIGH;
bool snakeLastSelectState = HIGH;

// Game settings
const int snakeCellSize = 8;
const int snakeMaxLength = 200;

int snakeGridWidth;
int snakeGridHeight;

int snakeX[snakeMaxLength];
int snakeY[snakeMaxLength];

int snakeLength = 3;

int foodX = 0;
int foodY = 0;

int snakeDirX = 1;
int snakeDirY = 0;

int nextDirX = 1;
int nextDirY = 0;

int snakeScore = 0;

unsigned long snakeLastMoveTime = 0;
const int snakeMoveDelay = 140;

// Function declarations
void drawSnakeMenu();
void drawSnakeOption(int index, bool selected);
void drawSnakeIcon(int x, int y);

void startSnakeGame();
void moveSnake();
void spawnFood();

void drawSnakeBlock(int gridX, int gridY, uint16_t color);
void eraseSnakeBlock(int gridX, int gridY);
void drawFood();
void drawBorder();

bool snakeHitWall(int x, int y);
bool snakeHitSelf(int x, int y);
bool foodIsOnSnake(int x, int y);

void gameOverSnake();

void initSnake() {
  selectedSnakeOption = 0;
  snakeState = SNAKE_MENU;

  snakeLastUpState = HIGH;
  snakeLastDownState = HIGH;
  snakeLastLeftState = HIGH;
  snakeLastRightState = HIGH;
  snakeLastSelectState = HIGH;

  drawSnakeMenu();
}

void updateSnake() {
  bool currentUpState = digitalRead(UP_BUTTON_GPIO);
  bool currentDownState = digitalRead(DOWN_BUTTON_GPIO);
  bool currentLeftState = digitalRead(LEFT_BUTTON_GPIO);
  bool currentRightState = digitalRead(RIGHT_BUTTON_GPIO);
  bool currentSelectState = digitalRead(SELECT_BUTTON_GPIO);

  if (snakeState == SNAKE_MENU) {
    if (currentDownState == LOW && snakeLastDownState == HIGH) {
      int oldSelectedOption = selectedSnakeOption;

      selectedSnakeOption++;

      if (selectedSnakeOption >= SNAKE_OPTION_COUNT) {
        selectedSnakeOption = 0;
      }

      drawSnakeOption(oldSelectedOption, false);
      drawSnakeOption(selectedSnakeOption, true);
    }

    if (currentUpState == LOW && snakeLastUpState == HIGH) {
      int oldSelectedOption = selectedSnakeOption;

      selectedSnakeOption--;

      if (selectedSnakeOption < 0) {
        selectedSnakeOption = SNAKE_OPTION_COUNT - 1;
      }

      drawSnakeOption(oldSelectedOption, false);
      drawSnakeOption(selectedSnakeOption, true);
    }

    if (currentSelectState == LOW && snakeLastSelectState == HIGH) {
      if (selectedSnakeOption == 0) {
        Serial.println("Snake started");
        startSnakeGame();
      } 
      else if (selectedSnakeOption == 1) {
        Serial.println("Back to home");
        showHomeMenuScreen();
      }
    }

    snakeLastUpState = currentUpState;
    snakeLastDownState = currentDownState;
    snakeLastSelectState = currentSelectState;

    return;
  }

  if (snakeState == SNAKE_PLAYING) {
    if (currentUpState == LOW && snakeLastUpState == HIGH) {
      if (snakeDirY != 1) {
        nextDirX = 0;
        nextDirY = -1;
      }
    }

    if (currentDownState == LOW && snakeLastDownState == HIGH) {
      if (snakeDirY != -1) {
        nextDirX = 0;
        nextDirY = 1;
      }
    }

    if (currentLeftState == LOW && snakeLastLeftState == HIGH) {
      if (snakeDirX != 1) {
        nextDirX = -1;
        nextDirY = 0;
      }
    }

    if (currentRightState == LOW && snakeLastRightState == HIGH) {
      if (snakeDirX != -1) {
        nextDirX = 1;
        nextDirY = 0;
      }
    }

    if (millis() - snakeLastMoveTime >= snakeMoveDelay) {
      snakeLastMoveTime = millis();
      moveSnake();
    }

    snakeLastUpState = currentUpState;
    snakeLastDownState = currentDownState;
    snakeLastLeftState = currentLeftState;
    snakeLastRightState = currentRightState;
    snakeLastSelectState = currentSelectState;
  }
}

void drawSnakeMenu() {
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);

  String title = "SNAKE";
  int titleWidth = title.length() * 6 * 2;
  int titleX = (tft.width() - titleWidth) / 2;

  tft.setCursor(titleX, 10);
  tft.println(title);

  drawSnakeIcon(tft.width() / 2, 58);

  for (int i = 0; i < SNAKE_OPTION_COUNT; i++) {
    drawSnakeOption(i, i == selectedSnakeOption);
  }
}

void drawSnakeOption(int index, bool selected) {
  int y = 96 + (index * 16);

  tft.fillRect(0, y, tft.width(), 14, ST77XX_BLACK);

  String optionText;

  if (selected) {
    optionText = "> " + snakeOptions[index];
    tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  } else {
    optionText = "  " + snakeOptions[index];
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  }

  tft.setTextSize(1);

  int textWidth = optionText.length() * 6;
  int x = (tft.width() - textWidth) / 2;

  tft.setCursor(x, y);
  tft.print(optionText);
}

void drawSnakeIcon(int x, int y) {
  int startX = x - 35;
  int startY = y;

  // Snake body
  tft.fillRect(startX, startY, 10, 10, ST77XX_GREEN);
  tft.fillRect(startX + 10, startY, 10, 10, ST77XX_GREEN);
  tft.fillRect(startX + 20, startY, 10, 10, ST77XX_GREEN);
  tft.fillRect(startX + 30, startY + 10, 10, 10, ST77XX_GREEN);
  tft.fillRect(startX + 40, startY + 10, 10, 10, ST77XX_GREEN);

  // Snake eye
  tft.fillRect(startX + 46, startY + 12, 2, 2, ST77XX_WHITE);

  // Food
  tft.fillCircle(startX + 65, startY + 15, 4, ST77XX_RED);
}

void startSnakeGame() {
  snakeState = SNAKE_PLAYING;

  snakeGridWidth = tft.width() / snakeCellSize;
  snakeGridHeight = tft.height() / snakeCellSize;

  snakeLength = 3;
  snakeScore = 0;

  snakeDirX = 1;
  snakeDirY = 0;
  nextDirX = 1;
  nextDirY = 0;

  int startX = snakeGridWidth / 2;
  int startY = snakeGridHeight / 2;

  snakeX[0] = startX;
  snakeY[0] = startY;

  snakeX[1] = startX - 1;
  snakeY[1] = startY;

  snakeX[2] = startX - 2;
  snakeY[2] = startY;

  tft.fillScreen(ST77XX_BLACK);

  drawBorder();

  for (int i = 0; i < snakeLength; i++) {
    drawSnakeBlock(snakeX[i], snakeY[i], ST77XX_GREEN);
  }

  spawnFood();
  drawFood();

  snakeLastMoveTime = millis();

  snakeLastUpState = HIGH;
  snakeLastDownState = HIGH;
  snakeLastLeftState = HIGH;
  snakeLastRightState = HIGH;
  snakeLastSelectState = HIGH;
}

void moveSnake() {
  snakeDirX = nextDirX;
  snakeDirY = nextDirY;

  int newHeadX = snakeX[0] + snakeDirX;
  int newHeadY = snakeY[0] + snakeDirY;

  if (snakeHitWall(newHeadX, newHeadY) || snakeHitSelf(newHeadX, newHeadY)) {
    gameOverSnake();
    return;
  }

  bool ateFood = (newHeadX == foodX && newHeadY == foodY);

  int oldTailX = snakeX[snakeLength - 1];
  int oldTailY = snakeY[snakeLength - 1];

  if (ateFood && snakeLength < snakeMaxLength) {
    snakeLength++;
    snakeScore++;
  }

  for (int i = snakeLength - 1; i > 0; i--) {
    snakeX[i] = snakeX[i - 1];
    snakeY[i] = snakeY[i - 1];
  }

  snakeX[0] = newHeadX;
  snakeY[0] = newHeadY;

  drawSnakeBlock(snakeX[0], snakeY[0], ST77XX_GREEN);

  if (ateFood) {
    spawnFood();
    drawFood();
  } else {
    eraseSnakeBlock(oldTailX, oldTailY);
  }

  drawBorder();
}

void spawnFood() {
  const int safeMargin = 2;

  do {
    foodX = random(safeMargin, snakeGridWidth - safeMargin);
    foodY = random(safeMargin, snakeGridHeight - safeMargin);
  } while (foodIsOnSnake(foodX, foodY));
}

void drawSnakeBlock(int gridX, int gridY, uint16_t color) {
  int x = gridX * snakeCellSize;
  int y = gridY * snakeCellSize;

  tft.fillRect(x, y, snakeCellSize, snakeCellSize, ST77XX_BLACK);
  tft.fillRect(x + 1, y + 1, snakeCellSize - 2, snakeCellSize - 2, color);
}

void eraseSnakeBlock(int gridX, int gridY) {
  int x = gridX * snakeCellSize;
  int y = gridY * snakeCellSize;

  tft.fillRect(x, y, snakeCellSize, snakeCellSize, ST77XX_BLACK);
}

void drawFood() {
  int x = foodX * snakeCellSize;
  int y = foodY * snakeCellSize;

  tft.fillCircle(x + snakeCellSize / 2, y + snakeCellSize / 2, 3, ST77XX_RED);
}

void drawBorder() {
  tft.drawRect(
    0,
    0,
    snakeGridWidth * snakeCellSize,
    snakeGridHeight * snakeCellSize,
    ST77XX_WHITE
  );
}

bool snakeHitWall(int x, int y) {
  if (x <= 0) {
    return true;
  }

  if (x >= snakeGridWidth - 1) {
    return true;
  }

  if (y <= 0) {
    return true;
  }

  if (y >= snakeGridHeight - 1) {
    return true;
  }

  return false;
}

bool snakeHitSelf(int x, int y) {
  for (int i = 0; i < snakeLength; i++) {
    if (snakeX[i] == x && snakeY[i] == y) {
      return true;
    }
  }

  return false;
}

bool foodIsOnSnake(int x, int y) {
  for (int i = 0; i < snakeLength; i++) {
    if (snakeX[i] == x && snakeY[i] == y) {
      return true;
    }
  }

  return false;
}

void gameOverSnake() {
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
  tft.print(snakeScore);

  delay(2000);

  showPlayAgainScreen();
}