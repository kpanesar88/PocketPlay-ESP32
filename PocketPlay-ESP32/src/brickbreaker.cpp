#include <Arduino.h>

#include "button.hpp"
#include "brickbreaker.hpp"
#include "display.hpp"
#include "app.hpp"

const int BB_OPTION_COUNT = 2;
int selectedBBOption = 0;

String bbOptions[BB_OPTION_COUNT] = {
  "PLAY",
  "BACK"
};

enum BrickBreakerState {
  BB_MENU,
  BB_PLAYING,
  BB_GAME_OVER
};

BrickBreakerState bbState = BB_MENU;

// Button edge detection
bool bbLastUpState = HIGH;
bool bbLastDownState = HIGH;
bool bbLastLeftState = HIGH;
bool bbLastRightState = HIGH;
bool bbLastSelectState = HIGH;

// Debounce for menu/select only
unsigned long bbLastUpPressTime = 0;
unsigned long bbLastDownPressTime = 0;
unsigned long bbLastSelectPressTime = 0;

const int bbDebounceDelay = 150;

// Paddle
int bbPaddleX = 60;
int bbOldPaddleX = 60;
int bbPaddleY = 114;
const int bbPaddleW = 34;
const int bbPaddleH = 5;

// Slower paddle movement while holding
const int bbPaddleSpeed = 2;
unsigned long bbLastPaddleMoveTime = 0;
const int bbPaddleMoveDelay = 18;

// Ball
int bbBallX = 80;
int bbBallY = 95;
int bbOldBallX = 80;
int bbOldBallY = 95;

int bbBallDX = 2;
int bbBallDY = -2;
const int bbBallRadius = 3;

// Bricks
const int BB_ROWS = 4;
const int BB_COLS = 7;

bool bbBricks[BB_ROWS][BB_COLS];

const int bbBrickW = 18;
const int bbBrickH = 8;
const int bbBrickGap = 2;

int bbBrickStartX = 0;
int bbBrickStartY = 24;

int bbScore = 0;
int bbTotalBricks = BB_ROWS * BB_COLS;

// Timing
unsigned long bbLastFrameTime = 0;
const int bbFrameDelay = 25;

// Function declarations
void drawBrickBreakerMenu();
void drawBrickBreakerOption(int index, bool selected);
void drawBrickBreakerIcon(int x, int y);

void startBrickBreakerGame();
void updateBrickBreakerGame();

void moveBrickBreakerPaddleLeft();
void moveBrickBreakerPaddleRight();

void drawBrickBreakerPaddle(int x, int y, uint16_t color);
void eraseBrickBreakerPaddle(int x, int y);

void drawBrickBreakerBall(int x, int y, uint16_t color);
void eraseBrickBreakerBall(int x, int y);

void drawBrickBreakerBricks();
void drawBrickBreakerBrick(int row, int col, uint16_t color);
void eraseBrickBreakerBrick(int row, int col);

void checkBrickBreakerCollision();
void gameOverBrickBreaker();
void winBrickBreaker();

void initBrickBreaker() {
  selectedBBOption = 0;
  bbState = BB_MENU;

  bbLastUpState = HIGH;
  bbLastDownState = HIGH;
  bbLastLeftState = HIGH;
  bbLastRightState = HIGH;
  bbLastSelectState = HIGH;

  bbLastUpPressTime = 0;
  bbLastDownPressTime = 0;
  bbLastSelectPressTime = 0;
  bbLastPaddleMoveTime = 0;

  drawBrickBreakerMenu();
}

void updateBrickBreaker() {
  bool currentUpState = digitalRead(UP_BUTTON_GPIO);
  bool currentDownState = digitalRead(DOWN_BUTTON_GPIO);
  bool currentLeftState = digitalRead(LEFT_BUTTON_GPIO);
  bool currentRightState = digitalRead(RIGHT_BUTTON_GPIO);
  bool currentSelectState = digitalRead(SELECT_BUTTON_GPIO);

  unsigned long currentTime = millis();

  if (bbState == BB_MENU) {
    if (
      currentDownState == LOW &&
      bbLastDownState == HIGH &&
      currentTime - bbLastDownPressTime > bbDebounceDelay
    ) {
      bbLastDownPressTime = currentTime;

      int oldOption = selectedBBOption;

      selectedBBOption++;

      if (selectedBBOption >= BB_OPTION_COUNT) {
        selectedBBOption = 0;
      }

      drawBrickBreakerOption(oldOption, false);
      drawBrickBreakerOption(selectedBBOption, true);
    }

    if (
      currentUpState == LOW &&
      bbLastUpState == HIGH &&
      currentTime - bbLastUpPressTime > bbDebounceDelay
    ) {
      bbLastUpPressTime = currentTime;

      int oldOption = selectedBBOption;

      selectedBBOption--;

      if (selectedBBOption < 0) {
        selectedBBOption = BB_OPTION_COUNT - 1;
      }

      drawBrickBreakerOption(oldOption, false);
      drawBrickBreakerOption(selectedBBOption, true);
    }

    if (
      currentSelectState == LOW &&
      bbLastSelectState == HIGH &&
      currentTime - bbLastSelectPressTime > bbDebounceDelay
    ) {
      bbLastSelectPressTime = currentTime;

      if (selectedBBOption == 0) {
        Serial.println("Brick Breaker started");
        startBrickBreakerGame();
      } 
      else if (selectedBBOption == 1) {
        Serial.println("Back to home");
        showHomeMenuScreen();
      }
    }

    bbLastUpState = currentUpState;
    bbLastDownState = currentDownState;
    bbLastSelectState = currentSelectState;

    return;
  }

  if (bbState == BB_PLAYING) {
    if (currentTime - bbLastPaddleMoveTime >= bbPaddleMoveDelay) {
      if (currentLeftState == LOW) {
        moveBrickBreakerPaddleLeft();
        bbLastPaddleMoveTime = currentTime;
      }

      if (currentRightState == LOW) {
        moveBrickBreakerPaddleRight();
        bbLastPaddleMoveTime = currentTime;
      }
    }

    if (millis() - bbLastFrameTime >= bbFrameDelay) {
      bbLastFrameTime = millis();
      updateBrickBreakerGame();
    }
  }

  bbLastUpState = currentUpState;
  bbLastDownState = currentDownState;
  bbLastLeftState = currentLeftState;
  bbLastRightState = currentRightState;
  bbLastSelectState = currentSelectState;
}

void drawBrickBreakerMenu() {
  tft.fillScreen(ST77XX_BLACK);

  // Centered title: BRICK
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);

  String title1 = "BRICK";
  int title1Width = title1.length() * 6 * 2;
  int title1X = (tft.width() - title1Width) / 2;

  tft.setCursor(title1X, 6);
  tft.println(title1);

  // Centered title: BREAKER
  String title2 = "BREAKER";
  int title2Width = title2.length() * 6 * 2;
  int title2X = (tft.width() - title2Width) / 2;

  tft.setCursor(title2X, 28);
  tft.println(title2);

  // Centered icon under title
  drawBrickBreakerIcon(tft.width() / 2, 67);

  // Centered play/back options under icon
  for (int i = 0; i < BB_OPTION_COUNT; i++) {
    drawBrickBreakerOption(i, i == selectedBBOption);
  }
}

void drawBrickBreakerOption(int index, bool selected) {
  int y = 105 + (index * 15);

  tft.fillRect(0, y, tft.width(), 13, ST77XX_BLACK);

  String optionText;

  if (selected) {
    optionText = "> " + bbOptions[index];
    tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  } else {
    optionText = "  " + bbOptions[index];
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  }

  tft.setTextSize(1);

  int textWidth = optionText.length() * 6;
  int x = (tft.width() - textWidth) / 2;

  tft.setCursor(x, y);
  tft.print(optionText);
}

void drawBrickBreakerIcon(int x, int y) {
  // Bricks centered around x
  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 4; col++) {
      int bx = x - 38 + (col * 20);
      int by = y - 18 + (row * 8);

      if (row == 0) {
        tft.fillRect(bx, by, 16, 6, ST77XX_RED);
      } else if (row == 1) {
        tft.fillRect(bx, by, 16, 6, ST77XX_YELLOW);
      } else {
        tft.fillRect(bx, by, 16, 6, ST77XX_GREEN);
      }
    }
  }

  // Ball
  tft.fillCircle(x, y + 13, 4, ST77XX_WHITE);

  // Paddle
  tft.fillRect(x - 20, y + 25, 40, 5, ST77XX_CYAN);
}

void startBrickBreakerGame() {
  bbState = BB_PLAYING;

  bbPaddleX = (tft.width() - bbPaddleW) / 2;
  bbOldPaddleX = bbPaddleX;
  bbPaddleY = tft.height() - 14;

  bbBallX = tft.width() / 2;
  bbBallY = bbPaddleY - 10;
  bbOldBallX = bbBallX;
  bbOldBallY = bbBallY;

  bbBallDX = 2;
  bbBallDY = -2;

  bbScore = 0;
  bbTotalBricks = BB_ROWS * BB_COLS;

  bbBrickStartX = (tft.width() - ((BB_COLS * bbBrickW) + ((BB_COLS - 1) * bbBrickGap))) / 2;
  bbBrickStartY = 22;

  for (int row = 0; row < BB_ROWS; row++) {
    for (int col = 0; col < BB_COLS; col++) {
      bbBricks[row][col] = true;
    }
  }

  bbLastLeftState = HIGH;
  bbLastRightState = HIGH;
  bbLastSelectState = HIGH;

  bbLastSelectPressTime = 0;
  bbLastPaddleMoveTime = 0;

  tft.fillScreen(ST77XX_BLACK);

  drawBrickBreakerBricks();
  drawBrickBreakerPaddle(bbPaddleX, bbPaddleY, ST77XX_CYAN);
  drawBrickBreakerBall(bbBallX, bbBallY, ST77XX_WHITE);

  bbLastFrameTime = millis();
}

void updateBrickBreakerGame() {
  bbOldBallX = bbBallX;
  bbOldBallY = bbBallY;

  bbBallX += bbBallDX;
  bbBallY += bbBallDY;

  if (bbBallX - bbBallRadius <= 0) {
    bbBallX = bbBallRadius;
    bbBallDX = -bbBallDX;
  }

  if (bbBallX + bbBallRadius >= tft.width()) {
    bbBallX = tft.width() - bbBallRadius;
    bbBallDX = -bbBallDX;
  }

  if (bbBallY - bbBallRadius <= 0) {
    bbBallY = bbBallRadius;
    bbBallDY = -bbBallDY;
  }

  bool ballOnPaddleX = bbBallX >= bbPaddleX && bbBallX <= bbPaddleX + bbPaddleW;
  bool ballOnPaddleY = bbBallY + bbBallRadius >= bbPaddleY && bbBallY - bbBallRadius <= bbPaddleY + bbPaddleH;

  if (ballOnPaddleX && ballOnPaddleY && bbBallDY > 0) {
    bbBallY = bbPaddleY - bbBallRadius - 1;
    bbBallDY = -bbBallDY;

    int paddleCenter = bbPaddleX + (bbPaddleW / 2);

    if (bbBallX < paddleCenter - 8) {
      bbBallDX = -3;
    } else if (bbBallX > paddleCenter + 8) {
      bbBallDX = 3;
    } else if (bbBallX < paddleCenter) {
      bbBallDX = -2;
    } else {
      bbBallDX = 2;
    }
  }

  checkBrickBreakerCollision();

  if (bbBallY - bbBallRadius > tft.height()) {
    gameOverBrickBreaker();
    return;
  }

  eraseBrickBreakerBall(bbOldBallX, bbOldBallY);
  drawBrickBreakerBall(bbBallX, bbBallY, ST77XX_WHITE);
}

void moveBrickBreakerPaddleLeft() {
  bbOldPaddleX = bbPaddleX;
  bbPaddleX -= bbPaddleSpeed;

  if (bbPaddleX < 2) {
    bbPaddleX = 2;
  }

  if (bbOldPaddleX != bbPaddleX) {
    eraseBrickBreakerPaddle(bbOldPaddleX, bbPaddleY);
    drawBrickBreakerPaddle(bbPaddleX, bbPaddleY, ST77XX_CYAN);
  }
}

void moveBrickBreakerPaddleRight() {
  bbOldPaddleX = bbPaddleX;
  bbPaddleX += bbPaddleSpeed;

  if (bbPaddleX + bbPaddleW > tft.width() - 2) {
    bbPaddleX = tft.width() - bbPaddleW - 2;
  }

  if (bbOldPaddleX != bbPaddleX) {
    eraseBrickBreakerPaddle(bbOldPaddleX, bbPaddleY);
    drawBrickBreakerPaddle(bbPaddleX, bbPaddleY, ST77XX_CYAN);
  }
}

void drawBrickBreakerPaddle(int x, int y, uint16_t color) {
  tft.fillRect(x, y, bbPaddleW, bbPaddleH, color);
}

void eraseBrickBreakerPaddle(int x, int y) {
  tft.fillRect(x, y, bbPaddleW, bbPaddleH, ST77XX_BLACK);
}

void drawBrickBreakerBall(int x, int y, uint16_t color) {
  tft.fillCircle(x, y, bbBallRadius, color);
}

void eraseBrickBreakerBall(int x, int y) {
  tft.fillRect(
    x - bbBallRadius - 1,
    y - bbBallRadius - 1,
    (bbBallRadius * 2) + 3,
    (bbBallRadius * 2) + 3,
    ST77XX_BLACK
  );
}

void drawBrickBreakerBricks() {
  for (int row = 0; row < BB_ROWS; row++) {
    for (int col = 0; col < BB_COLS; col++) {
      if (bbBricks[row][col]) {
        if (row == 0) {
          drawBrickBreakerBrick(row, col, ST77XX_RED);
        } else if (row == 1) {
          drawBrickBreakerBrick(row, col, ST77XX_YELLOW);
        } else if (row == 2) {
          drawBrickBreakerBrick(row, col, ST77XX_GREEN);
        } else {
          drawBrickBreakerBrick(row, col, ST77XX_BLUE);
        }
      }
    }
  }
}

void drawBrickBreakerBrick(int row, int col, uint16_t color) {
  int x = bbBrickStartX + col * (bbBrickW + bbBrickGap);
  int y = bbBrickStartY + row * (bbBrickH + bbBrickGap);

  tft.fillRect(x, y, bbBrickW, bbBrickH, color);
}

void eraseBrickBreakerBrick(int row, int col) {
  int x = bbBrickStartX + col * (bbBrickW + bbBrickGap);
  int y = bbBrickStartY + row * (bbBrickH + bbBrickGap);

  tft.fillRect(x, y, bbBrickW, bbBrickH, ST77XX_BLACK);
}

void checkBrickBreakerCollision() {
  for (int row = 0; row < BB_ROWS; row++) {
    for (int col = 0; col < BB_COLS; col++) {
      if (!bbBricks[row][col]) {
        continue;
      }

      int brickX = bbBrickStartX + col * (bbBrickW + bbBrickGap);
      int brickY = bbBrickStartY + row * (bbBrickH + bbBrickGap);

      bool hitX = bbBallX + bbBallRadius >= brickX &&
                  bbBallX - bbBallRadius <= brickX + bbBrickW;

      bool hitY = bbBallY + bbBallRadius >= brickY &&
                  bbBallY - bbBallRadius <= brickY + bbBrickH;

      if (hitX && hitY) {
        bbBricks[row][col] = false;
        eraseBrickBreakerBrick(row, col);

        bbScore++;
        bbTotalBricks--;

        bbBallDY = -bbBallDY;

        if (bbTotalBricks <= 0) {
          winBrickBreaker();
          return;
        }

        return;
      }
    }
  }
}

void gameOverBrickBreaker() {
  bbState = BB_GAME_OVER;

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
  tft.print(bbScore);

  delay(2000);

  showPlayAgainScreen();
}

void winBrickBreaker() {
  bbState = BB_GAME_OVER;

  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  tft.setCursor(25, 30);
  tft.println("YOU WIN");

  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
  tft.setCursor(15, 65);
  tft.print("SCORE");

  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
  tft.setCursor(95, 65);
  tft.print(bbScore);

  delay(2000);

  showPlayAgainScreen();
}