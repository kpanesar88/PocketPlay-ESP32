#include <Arduino.h>

#include "button.hpp"
#include "pong.hpp"
#include "display.hpp"
#include "app.hpp"

const int PONG_OPTION_COUNT = 2;
int selectedPongOption = 0;

String pongOptions[PONG_OPTION_COUNT] = {
  "PLAY",
  "BACK"
};

enum PongState {
  PONG_MENU,
  PONG_PLAYING,
  PONG_GAME_OVER
};

PongState pongState = PONG_MENU;

// Button states
bool pongLastUpState = HIGH;
bool pongLastDownState = HIGH;
bool pongLastLeftState = HIGH;
bool pongLastRightState = HIGH;
bool pongLastSelectState = HIGH;

// Debounce for menu/select
unsigned long pongLastUpPressTime = 0;
unsigned long pongLastDownPressTime = 0;
unsigned long pongLastSelectPressTime = 0;

const int pongDebounceDelay = 150;

// Paddle movement timing
unsigned long pongLastPaddleMoveTime = 0;
const int pongPaddleMoveDelay = 14;
const int pongPaddleSpeed = 2;

// Game settings
const int pongPaddleW = 34;
const int pongPaddleH = 4;

int pongTopPaddleX = 63;
int pongTopPaddleY = 18;
int pongOldTopPaddleX = 63;

int pongBottomPaddleX = 63;
int pongBottomPaddleY = 118;
int pongOldBottomPaddleX = 63;

int pongBallX = 80;
int pongBallY = 64;
int pongOldBallX = 80;
int pongOldBallY = 64;

int pongBallDX = 2;
int pongBallDY = 2;
const int pongBallSize = 4;

int pongP1Score = 0; // top player
int pongP2Score = 0; // bottom player
const int pongWinningScore = 5;

// Timing
unsigned long pongLastFrameTime = 0;
const int pongFrameDelay = 24;

// Function declarations
void drawPongMenu();
void drawPongOption(int index, bool selected);
void drawPongIcon(int x, int y);

void startPongGame();
void updatePongGame();

void movePongPaddles();

void drawPongPaddle(int x, int y, uint16_t color);
void erasePongPaddle(int x, int y);

void drawPongBall(int x, int y, uint16_t color);
void erasePongBall(int x, int y);

void drawPongCenterLine();
void drawPongScore();
void resetPongBall(int direction);

void showPongWinner(int player);

void initPong() {
  selectedPongOption = 0;
  pongState = PONG_MENU;

  pongLastUpState = HIGH;
  pongLastDownState = HIGH;
  pongLastLeftState = HIGH;
  pongLastRightState = HIGH;
  pongLastSelectState = HIGH;

  pongLastUpPressTime = 0;
  pongLastDownPressTime = 0;
  pongLastSelectPressTime = 0;
  pongLastPaddleMoveTime = 0;

  drawPongMenu();
}

void updatePong() {
  bool currentUpState = digitalRead(UP_BUTTON_GPIO);
  bool currentDownState = digitalRead(DOWN_BUTTON_GPIO);
  bool currentLeftState = digitalRead(LEFT_BUTTON_GPIO);
  bool currentRightState = digitalRead(RIGHT_BUTTON_GPIO);
  bool currentSelectState = digitalRead(SELECT_BUTTON_GPIO);

  unsigned long currentTime = millis();

  if (pongState == PONG_MENU) {
    if (
      currentDownState == LOW &&
      pongLastDownState == HIGH &&
      currentTime - pongLastDownPressTime > pongDebounceDelay
    ) {
      pongLastDownPressTime = currentTime;

      int oldOption = selectedPongOption;

      selectedPongOption++;

      if (selectedPongOption >= PONG_OPTION_COUNT) {
        selectedPongOption = 0;
      }

      drawPongOption(oldOption, false);
      drawPongOption(selectedPongOption, true);
    }

    if (
      currentUpState == LOW &&
      pongLastUpState == HIGH &&
      currentTime - pongLastUpPressTime > pongDebounceDelay
    ) {
      pongLastUpPressTime = currentTime;

      int oldOption = selectedPongOption;

      selectedPongOption--;

      if (selectedPongOption < 0) {
        selectedPongOption = PONG_OPTION_COUNT - 1;
      }

      drawPongOption(oldOption, false);
      drawPongOption(selectedPongOption, true);
    }

    if (
      currentSelectState == LOW &&
      pongLastSelectState == HIGH &&
      currentTime - pongLastSelectPressTime > pongDebounceDelay
    ) {
      pongLastSelectPressTime = currentTime;

      if (selectedPongOption == 0) {
        Serial.println("Vertical Pong started");
        startPongGame();
      } else {
        Serial.println("Back to home");
        showHomeMenuScreen();
      }
    }

    pongLastUpState = currentUpState;
    pongLastDownState = currentDownState;
    pongLastSelectState = currentSelectState;

    return;
  }

  if (pongState == PONG_PLAYING) {
    if (currentTime - pongLastPaddleMoveTime >= pongPaddleMoveDelay) {
      movePongPaddles();
      pongLastPaddleMoveTime = currentTime;
    }

    if (currentTime - pongLastFrameTime >= pongFrameDelay) {
      pongLastFrameTime = currentTime;
      updatePongGame();
    }
  }

  pongLastUpState = currentUpState;
  pongLastDownState = currentDownState;
  pongLastLeftState = currentLeftState;
  pongLastRightState = currentRightState;
  pongLastSelectState = currentSelectState;
}

void drawPongMenu() {
  tft.fillScreen(ST77XX_BLACK);

  // Centered title
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);

  String title = "PONG";
  int titleWidth = title.length() * 6 * 2;
  int titleX = (tft.width() - titleWidth) / 2;

  tft.setCursor(titleX, 10);
  tft.println(title);

  // Centered icon
  drawPongIcon(tft.width() / 2, 62);

  // Centered options
  for (int i = 0; i < PONG_OPTION_COUNT; i++) {
    drawPongOption(i, i == selectedPongOption);
  }
}

void drawPongOption(int index, bool selected) {
  int y = 100 + (index * 16);

  tft.fillRect(0, y, tft.width(), 14, ST77XX_BLACK);

  String optionText;

  if (selected) {
    optionText = "> " + pongOptions[index];
    tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  } else {
    optionText = "  " + pongOptions[index];
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  }

  int textWidth = optionText.length() * 6;
  int x = (tft.width() - textWidth) / 2;

  tft.setCursor(x, y);
  tft.setTextSize(1);
  tft.print(optionText);
}

void drawPongIcon(int x, int y) {
  // Mini vertical court
  tft.drawRect(x - 38, y - 28, 76, 56, ST77XX_WHITE);

  // Center line
  for (int dashX = x - 35; dashX < x + 36; dashX += 10) {
    tft.drawFastHLine(dashX, y, 5, ST77XX_WHITE);
  }

  // Top paddle
  tft.fillRect(x - 18, y - 22, 36, 4, ST77XX_CYAN);

  // Bottom paddle
  tft.fillRect(x - 18, y + 18, 36, 4, ST77XX_RED);

  // Ball
  tft.fillRect(x - 2, y - 2, 5, 5, ST77XX_YELLOW);
}

void startPongGame() {
  pongState = PONG_PLAYING;

  pongTopPaddleX = (tft.width() - pongPaddleW) / 2;
  pongBottomPaddleX = (tft.width() - pongPaddleW) / 2;

  pongTopPaddleY = 18;
  pongBottomPaddleY = tft.height() - 10;

  pongOldTopPaddleX = pongTopPaddleX;
  pongOldBottomPaddleX = pongBottomPaddleX;

  pongP1Score = 0;
  pongP2Score = 0;

  tft.fillScreen(ST77XX_BLACK);

  drawPongCenterLine();
  drawPongScore();

  drawPongPaddle(pongTopPaddleX, pongTopPaddleY, ST77XX_CYAN);
  drawPongPaddle(pongBottomPaddleX, pongBottomPaddleY, ST77XX_RED);

  resetPongBall(1);

  pongLastPaddleMoveTime = millis();
  pongLastFrameTime = millis();
}

void updatePongGame() {
  pongOldBallX = pongBallX;
  pongOldBallY = pongBallY;

  pongBallX += pongBallDX;
  pongBallY += pongBallDY;

  // Left and right wall bounce
  if (pongBallX <= 0) {
    pongBallX = 0;
    pongBallDX = -pongBallDX;
  }

  if (pongBallX + pongBallSize >= tft.width()) {
    pongBallX = tft.width() - pongBallSize;
    pongBallDX = -pongBallDX;
  }

  // Top paddle collision
  bool hitTopPaddle =
    pongBallY <= pongTopPaddleY + pongPaddleH &&
    pongBallY + pongBallSize >= pongTopPaddleY &&
    pongBallX + pongBallSize >= pongTopPaddleX &&
    pongBallX <= pongTopPaddleX + pongPaddleW;

  if (hitTopPaddle && pongBallDY < 0) {
    pongBallY = pongTopPaddleY + pongPaddleH + 1;
    pongBallDY = -pongBallDY;

    int paddleCenter = pongTopPaddleX + pongPaddleW / 2;

    if (pongBallX < paddleCenter - 8) {
      pongBallDX = -3;
    } else if (pongBallX > paddleCenter + 8) {
      pongBallDX = 3;
    } else if (pongBallX < paddleCenter) {
      pongBallDX = -2;
    } else {
      pongBallDX = 2;
    }
  }

  // Bottom paddle collision
  bool hitBottomPaddle =
    pongBallY + pongBallSize >= pongBottomPaddleY &&
    pongBallY <= pongBottomPaddleY + pongPaddleH &&
    pongBallX + pongBallSize >= pongBottomPaddleX &&
    pongBallX <= pongBottomPaddleX + pongPaddleW;

  if (hitBottomPaddle && pongBallDY > 0) {
    pongBallY = pongBottomPaddleY - pongBallSize - 1;
    pongBallDY = -pongBallDY;

    int paddleCenter = pongBottomPaddleX + pongPaddleW / 2;

    if (pongBallX < paddleCenter - 8) {
      pongBallDX = -3;
    } else if (pongBallX > paddleCenter + 8) {
      pongBallDX = 3;
    } else if (pongBallX < paddleCenter) {
      pongBallDX = -2;
    } else {
      pongBallDX = 2;
    }
  }

  // Bottom player scores if ball goes past top
  if (pongBallY < 0) {
    pongP2Score++;
    drawPongScore();

    if (pongP2Score >= pongWinningScore) {
      showPongWinner(2);
      return;
    }

    resetPongBall(1);
    return;
  }

  // Top player scores if ball goes past bottom
  if (pongBallY > tft.height()) {
    pongP1Score++;
    drawPongScore();

    if (pongP1Score >= pongWinningScore) {
      showPongWinner(1);
      return;
    }

    resetPongBall(-1);
    return;
  }

  erasePongBall(pongOldBallX, pongOldBallY);
  drawPongBall(pongBallX, pongBallY, ST77XX_YELLOW);
}

void movePongPaddles() {
  pongOldTopPaddleX = pongTopPaddleX;
  pongOldBottomPaddleX = pongBottomPaddleX;

  // Top player:
  // LEFT = move left
  // RIGHT = move right
  if (digitalRead(LEFT_BUTTON_GPIO) == LOW) {
    pongTopPaddleX -= pongPaddleSpeed;
  }

  if (digitalRead(RIGHT_BUTTON_GPIO) == LOW) {
    pongTopPaddleX += pongPaddleSpeed;
  }

  // Bottom player:
  // UP = move left
  // DOWN = move right
  if (digitalRead(UP_BUTTON_GPIO) == LOW) {
    pongBottomPaddleX -= pongPaddleSpeed;
  }

  if (digitalRead(DOWN_BUTTON_GPIO) == LOW) {
    pongBottomPaddleX += pongPaddleSpeed;
  }

  if (pongTopPaddleX < 0) {
    pongTopPaddleX = 0;
  }

  if (pongTopPaddleX + pongPaddleW > tft.width()) {
    pongTopPaddleX = tft.width() - pongPaddleW;
  }

  if (pongBottomPaddleX < 0) {
    pongBottomPaddleX = 0;
  }

  if (pongBottomPaddleX + pongPaddleW > tft.width()) {
    pongBottomPaddleX = tft.width() - pongPaddleW;
  }

  if (pongOldTopPaddleX != pongTopPaddleX) {
    erasePongPaddle(pongOldTopPaddleX, pongTopPaddleY);
    drawPongPaddle(pongTopPaddleX, pongTopPaddleY, ST77XX_CYAN);
  }

  if (pongOldBottomPaddleX != pongBottomPaddleX) {
    erasePongPaddle(pongOldBottomPaddleX, pongBottomPaddleY);
    drawPongPaddle(pongBottomPaddleX, pongBottomPaddleY, ST77XX_RED);
  }
}

void drawPongPaddle(int x, int y, uint16_t color) {
  tft.fillRect(x, y, pongPaddleW, pongPaddleH, color);
}

void erasePongPaddle(int x, int y) {
  tft.fillRect(x, y, pongPaddleW, pongPaddleH, ST77XX_BLACK);
  drawPongCenterLine();
}

void drawPongBall(int x, int y, uint16_t color) {
  tft.fillRect(x, y, pongBallSize, pongBallSize, color);
}

void erasePongBall(int x, int y) {
  tft.fillRect(x - 1, y - 1, pongBallSize + 2, pongBallSize + 2, ST77XX_BLACK);
  drawPongCenterLine();
}

void drawPongCenterLine() {
  for (int x = 0; x < tft.width(); x += 12) {
    tft.drawFastHLine(x, tft.height() / 2, 6, ST77XX_YELLOW);
  }
}

void drawPongScore() {
  tft.fillRect(0, 0, tft.width(), 12, ST77XX_BLACK);

  tft.setTextSize(1);

  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
  tft.setCursor(35, 2);
  tft.print(pongP1Score);

  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(75, 2);
  tft.print("-");

  tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
  tft.setCursor(115, 2);
  tft.print(pongP2Score);
}

void resetPongBall(int direction) {
  erasePongBall(pongBallX, pongBallY);

  pongBallX = tft.width() / 2;
  pongBallY = tft.height() / 2;

  pongOldBallX = pongBallX;
  pongOldBallY = pongBallY;

  if (random(0, 2) == 0) {
    pongBallDX = -2;
  } else {
    pongBallDX = 2;
  }

  pongBallDY = 2 * direction;

  drawPongBall(pongBallX, pongBallY, ST77XX_YELLOW);

  delay(500);
}

void showPongWinner(int player) {
  pongState = PONG_GAME_OVER;

  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(2);

  if (player == 1) {
    tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
    tft.setCursor(20, 30);
    tft.println("TOP WINS");
  } else {
    tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
    tft.setCursor(10, 30);
    tft.println("BOT WINS");
  }

  delay(2000);

  showPlayAgainScreen();
}