#include <Arduino.h>

#include "button.hpp"
#include "dinorun.hpp"
#include "display.hpp"
#include "app.hpp"

const int DINO_OPTION_COUNT = 2;
int selectedDinoOption = 0;

String dinoOptions[DINO_OPTION_COUNT] = {
  "PLAY",
  "BACK"
};

enum DinoRunState {
  DINO_MENU,
  DINO_PLAYING
};

DinoRunState dinoRunState = DINO_MENU;

// Button edge detection
bool dinoLastUpState = HIGH;
bool dinoLastDownState = HIGH;
bool dinoLastSelectState = HIGH;
bool dinoLastJumpState = HIGH;

// Game variables
int dinoX = 25;
int dinoY = 86;
int oldDinoY = 86;

int groundY = 80;

int dinoVelocity = 0;
bool isJumping = false;

int cactusX = 160;
int oldCactusX = 160;

int cactusSpeed = 4;
int dinoScore = 0;

unsigned long dinoLastFrameTime = 0;
const int dinoFrameDelay = 25;

const int gravity = 1;
const int jumpPower = -8;

// Function declarations
void drawDinoMenu();
void drawDinoOption(int index, bool selected);
void drawDinoIcon(int x, int y);

void startDinoGame();
void drawGround();
void redrawGroundArea(int x, int w);

void drawDino(int x, int y, uint16_t color);
void eraseDino(int x, int y);

void drawCactus(int x, uint16_t color);
void eraseCactus(int x);

void resetCactus();
bool checkDinoCollision();
void gameOverDinoRun();

void initDinoRun() {
  selectedDinoOption = 0;
  dinoRunState = DINO_MENU;

  dinoLastUpState = HIGH;
  dinoLastDownState = HIGH;
  dinoLastSelectState = HIGH;
  dinoLastJumpState = HIGH;

  drawDinoMenu();
}

void updateDinoRun() {
  bool currentUpState = digitalRead(UP_BUTTON_GPIO);
  bool currentDownState = digitalRead(DOWN_BUTTON_GPIO);
  bool currentSelectState = digitalRead(SELECT_BUTTON_GPIO);

  if (dinoRunState == DINO_MENU) {
    if (currentDownState == LOW && dinoLastDownState == HIGH) {
      int oldSelectedOption = selectedDinoOption;

      selectedDinoOption++;

      if (selectedDinoOption >= DINO_OPTION_COUNT) {
        selectedDinoOption = 0;
      }

      drawDinoOption(oldSelectedOption, false);
      drawDinoOption(selectedDinoOption, true);
    }

    if (currentUpState == LOW && dinoLastUpState == HIGH) {
      int oldSelectedOption = selectedDinoOption;

      selectedDinoOption--;

      if (selectedDinoOption < 0) {
        selectedDinoOption = DINO_OPTION_COUNT - 1;
      }

      drawDinoOption(oldSelectedOption, false);
      drawDinoOption(selectedDinoOption, true);
    }

    if (currentSelectState == LOW && dinoLastSelectState == HIGH) {
      if (selectedDinoOption == 0) {
        Serial.println("Dino Run started");
        startDinoGame();
      } 
      else if (selectedDinoOption == 1) {
        Serial.println("Back to home");
        showHomeMenuScreen();
      }
    }

    dinoLastUpState = currentUpState;
    dinoLastDownState = currentDownState;
    dinoLastSelectState = currentSelectState;

    return;
  }

  if (dinoRunState == DINO_PLAYING) {
    // Jump once per button click
    if (currentUpState == LOW && dinoLastJumpState == HIGH && !isJumping) {
      isJumping = true;
      dinoVelocity = jumpPower;
    }

    dinoLastJumpState = currentUpState;

    if (millis() - dinoLastFrameTime >= dinoFrameDelay) {
      dinoLastFrameTime = millis();

      oldDinoY = dinoY;
      oldCactusX = cactusX;

      // Dino jump physics
      if (isJumping) {
        dinoY += dinoVelocity;
        dinoVelocity += gravity;

        if (dinoY >= groundY - 18) {
          dinoY = groundY - 18;
          dinoVelocity = 0;
          isJumping = false;
        }
      }

      // Move cactus
      cactusX -= cactusSpeed;

      eraseCactus(oldCactusX);

      // Only redraw dino if it moved
      if (oldDinoY != dinoY) {
        eraseDino(dinoX, oldDinoY);
        drawDino(dinoX, dinoY, ST77XX_GREEN);
      }

      drawCactus(cactusX, ST77XX_RED);

      if (checkDinoCollision()) {
        gameOverDinoRun();
        return;
      }

      if (cactusX < -25) {
        dinoScore++;

        if (dinoScore % 5 == 0) {
          cactusSpeed++;
        }

        resetCactus();
      }
    }
  }
}

void drawDinoMenu() {
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);

  String title = "DINO RUN";
  int titleWidth = title.length() * 6 * 2;
  int titleX = (tft.width() - titleWidth) / 2;

  tft.setCursor(titleX, 10);
  tft.println(title);

  // Centered icon under title
  drawDinoIcon(tft.width() / 2, 58);

  // Centered play/back options
  for (int i = 0; i < DINO_OPTION_COUNT; i++) {
    drawDinoOption(i, i == selectedDinoOption);
  }
}

void drawDinoOption(int index, bool selected) {
  int y = 100 + (index * 16);

  tft.fillRect(0, y, tft.width(), 14, ST77XX_BLACK);

  String optionText;

  if (selected) {
    optionText = "> " + dinoOptions[index];
    tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  } else {
    optionText = "  " + dinoOptions[index];
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  }

  tft.setTextSize(1);

  int textWidth = optionText.length() * 6;
  int x = (tft.width() - textWidth) / 2;

  tft.setCursor(x, y);
  tft.print(optionText);
}
void drawDinoIcon(int x, int y) {
  // Total icon width is about 76px
  // This keeps the whole dino + cactus centered around x
  int startX = x - 38;
  int startY = y;

  // Dino body
  tft.fillRect(startX + 8, startY + 12, 16, 9, ST77XX_GREEN);

  // Dino head
  tft.fillRect(startX + 20, startY + 6, 10, 8, ST77XX_GREEN);

  // Snout
  tft.fillRect(startX + 28, startY + 9, 4, 3, ST77XX_GREEN);

  // Eye
  tft.fillRect(startX + 26, startY + 8, 2, 2, ST77XX_WHITE);

  // Tail
  tft.fillTriangle(
    startX + 8, startY + 15,
    startX, startY + 12,
    startX + 8, startY + 19,
    ST77XX_GREEN
  );

  // Legs
  tft.fillRect(startX + 11, startY + 21, 3, 5, ST77XX_GREEN);
  tft.fillRect(startX + 19, startY + 21, 3, 5, ST77XX_GREEN);

  // Feet
  tft.fillRect(startX + 10, startY + 25, 5, 2, ST77XX_GREEN);
  tft.fillRect(startX + 18, startY + 25, 5, 2, ST77XX_GREEN);

  // Red cactus
  tft.fillRect(startX + 58, startY + 16, 5, 13, ST77XX_RED);
  tft.fillRect(startX + 54, startY + 21, 4, 3, ST77XX_RED);
  tft.fillRect(startX + 63, startY + 19, 4, 3, ST77XX_RED);

  // Small ground line
  tft.drawFastHLine(startX - 2, startY + 30, 76, ST77XX_WHITE);
}
void startDinoGame() {
  dinoRunState = DINO_PLAYING;

  // Ground near center
  groundY = tft.height() / 2 + 15;

  dinoX = 25;
  dinoY = groundY - 18;
  oldDinoY = dinoY;

  dinoVelocity = 0;
  isJumping = false;

  cactusX = tft.width() + 30;
  oldCactusX = cactusX;

  cactusSpeed = 4;
  dinoScore = 0;

  dinoLastJumpState = HIGH;

  tft.fillScreen(ST77XX_BLACK);

  drawGround();
  drawDino(dinoX, dinoY, ST77XX_GREEN);
  drawCactus(cactusX, ST77XX_RED);

  dinoLastFrameTime = millis();
}

void drawGround() {
  tft.drawLine(0, groundY, tft.width(), groundY, ST77XX_WHITE);
}

void redrawGroundArea(int x, int w) {
  if (x < 0) {
    x = 0;
  }

  if (x + w > tft.width()) {
    w = tft.width() - x;
  }

  if (w > 0) {
    tft.drawLine(x, groundY, x + w, groundY, ST77XX_WHITE);
  }
}

void drawDino(int x, int y, uint16_t color) {
  // Smaller playable dino

  // Body
  tft.fillRect(x, y + 7, 14, 8, color);

  // Head
  tft.fillRect(x + 10, y + 2, 9, 8, color);

  // Snout
  tft.fillRect(x + 18, y + 5, 4, 3, color);

  // Eye
  tft.fillRect(x + 16, y + 4, 2, 2, ST77XX_WHITE);

  // Tail
  tft.fillTriangle(x, y + 11, x - 6, y + 8, x, y + 15, color);

  // Legs
  tft.fillRect(x + 3, y + 15, 3, 4, color);
  tft.fillRect(x + 10, y + 15, 3, 4, color);

  // Feet
  tft.fillRect(x + 2, y + 18, 5, 2, color);
  tft.fillRect(x + 9, y + 18, 5, 2, color);
}

void eraseDino(int x, int y) {
  tft.fillRect(x - 7, y, 32, 23, ST77XX_BLACK);
  redrawGroundArea(x - 7, 32);
}

void drawCactus(int x, uint16_t color) {
  if (x < -25 || x > tft.width() + 25) {
    return;
  }

  int cactusY = groundY - 12;

  // Smaller but noticeable red cactus

  // Main body
  tft.fillRect(x, cactusY, 6, 12, color);

  // Top cap
  tft.fillRect(x + 1, cactusY - 2, 4, 3, color);

  // Left arm
  tft.fillRect(x - 5, cactusY + 5, 5, 3, color);
  tft.fillRect(x - 5, cactusY + 2, 3, 6, color);

  // Right arm
  tft.fillRect(x + 6, cactusY + 6, 5, 3, color);
  tft.fillRect(x + 8, cactusY + 3, 3, 6, color);

  // White outline so it stands out
  tft.drawRect(x - 1, cactusY - 3, 8, 15, ST77XX_WHITE);
}

void eraseCactus(int x) {
  int cactusY = groundY - 16;

  tft.fillRect(x - 7, cactusY, 22, 19, ST77XX_BLACK);
  redrawGroundArea(x - 7, 22);
}

void resetCactus() {
  cactusX = tft.width() + random(35, 95);
  oldCactusX = cactusX;
}

bool checkDinoCollision() {
  // Smaller and fairer hitboxes

  int dinoLeft = dinoX + 2;
  int dinoRight = dinoX + 21;
  int dinoTop = dinoY + 3;
  int dinoBottom = dinoY + 19;

  int cactusLeft = cactusX - 5;
  int cactusRight = cactusX + 11;
  int cactusTop = groundY - 12;
  int cactusBottom = groundY;

  bool xOverlap = dinoRight >= cactusLeft && dinoLeft <= cactusRight;
  bool yOverlap = dinoBottom >= cactusTop && dinoTop <= cactusBottom;

  return xOverlap && yOverlap;
}

void gameOverDinoRun() {
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
  tft.print(dinoScore);

  delay(2000);

  showPlayAgainScreen();
}