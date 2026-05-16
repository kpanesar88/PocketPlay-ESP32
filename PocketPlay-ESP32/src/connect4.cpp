#include <Arduino.h>

#include "button.hpp"
#include "connect4.hpp"
#include "display.hpp"
#include "app.hpp"

const int C4_OPTION_COUNT = 2;
int selectedC4Option = 0;

String c4Options[C4_OPTION_COUNT] = {
  "PLAY",
  "BACK"
};

enum Connect4State {
  C4_MENU,
  C4_PLAYING,
  C4_GAME_OVER
};

Connect4State c4State = C4_MENU;

// Button edge detection
bool c4LastUpState = HIGH;
bool c4LastDownState = HIGH;
bool c4LastLeftState = HIGH;
bool c4LastRightState = HIGH;
bool c4LastSelectState = HIGH;

// Button debounce
unsigned long c4LastUpPressTime = 0;
unsigned long c4LastDownPressTime = 0;
unsigned long c4LastLeftPressTime = 0;
unsigned long c4LastRightPressTime = 0;
unsigned long c4LastSelectPressTime = 0;

const int c4DebounceDelay = 220;

// Board settings
const int C4_ROWS = 6;
const int C4_COLS = 7;

int c4Board[C4_ROWS][C4_COLS];

// Smaller centered board
int c4CellSize = 14;
int c4BoardX = 0;
int c4BoardY = 28;

int c4SelectedCol = 3;
int c4CurrentPlayer = 1; // 1 = red, 2 = blue

// Function declarations
void drawConnect4Menu();
void drawConnect4Option(int index, bool selected);
void drawConnect4Icon(int x, int y);

void startConnect4Game();
void drawConnect4Board();
void drawC4Cursor(int col, bool selected);
void drawC4Piece(int row, int col);
void clearC4Cursor();

bool dropC4Piece(int col);
bool c4ColumnFull(int col);
bool checkC4Win(int player);
bool checkC4Draw();

void showC4Winner(int player);
void showC4DrawScreen();

void initConnect4() {
  selectedC4Option = 0;
  c4State = C4_MENU;

  c4LastUpState = HIGH;
  c4LastDownState = HIGH;
  c4LastLeftState = HIGH;
  c4LastRightState = HIGH;
  c4LastSelectState = HIGH;

  c4LastUpPressTime = 0;
  c4LastDownPressTime = 0;
  c4LastLeftPressTime = 0;
  c4LastRightPressTime = 0;
  c4LastSelectPressTime = 0;

  drawConnect4Menu();
}

void updateConnect4() {
  bool currentUpState = digitalRead(UP_BUTTON_GPIO);
  bool currentDownState = digitalRead(DOWN_BUTTON_GPIO);
  bool currentLeftState = digitalRead(LEFT_BUTTON_GPIO);
  bool currentRightState = digitalRead(RIGHT_BUTTON_GPIO);
  bool currentSelectState = digitalRead(SELECT_BUTTON_GPIO);

  unsigned long currentTime = millis();

  if (c4State == C4_MENU) {
    if (
      currentDownState == LOW &&
      c4LastDownState == HIGH &&
      currentTime - c4LastDownPressTime > c4DebounceDelay
    ) {
      c4LastDownPressTime = currentTime;

      int oldSelectedOption = selectedC4Option;

      selectedC4Option++;

      if (selectedC4Option >= C4_OPTION_COUNT) {
        selectedC4Option = 0;
      }

      drawConnect4Option(oldSelectedOption, false);
      drawConnect4Option(selectedC4Option, true);
    }

    if (
      currentUpState == LOW &&
      c4LastUpState == HIGH &&
      currentTime - c4LastUpPressTime > c4DebounceDelay
    ) {
      c4LastUpPressTime = currentTime;

      int oldSelectedOption = selectedC4Option;

      selectedC4Option--;

      if (selectedC4Option < 0) {
        selectedC4Option = C4_OPTION_COUNT - 1;
      }

      drawConnect4Option(oldSelectedOption, false);
      drawConnect4Option(selectedC4Option, true);
    }

    if (
      currentSelectState == LOW &&
      c4LastSelectState == HIGH &&
      currentTime - c4LastSelectPressTime > c4DebounceDelay
    ) {
      c4LastSelectPressTime = currentTime;

      if (selectedC4Option == 0) {
        Serial.println("Connect 4 started");
        startConnect4Game();
      } 
      else if (selectedC4Option == 1) {
        Serial.println("Back to home");
        showHomeMenuScreen();
      }
    }

    c4LastUpState = currentUpState;
    c4LastDownState = currentDownState;
    c4LastSelectState = currentSelectState;

    return;
  }

  if (c4State == C4_PLAYING) {
    if (
      currentLeftState == LOW &&
      c4LastLeftState == HIGH &&
      currentTime - c4LastLeftPressTime > c4DebounceDelay
    ) {
      c4LastLeftPressTime = currentTime;

      c4SelectedCol--;

      if (c4SelectedCol < 0) {
        c4SelectedCol = C4_COLS - 1;
      }

      clearC4Cursor();
      drawC4Cursor(c4SelectedCol, true);
    }

    if (
      currentRightState == LOW &&
      c4LastRightState == HIGH &&
      currentTime - c4LastRightPressTime > c4DebounceDelay
    ) {
      c4LastRightPressTime = currentTime;

      c4SelectedCol++;

      if (c4SelectedCol >= C4_COLS) {
        c4SelectedCol = 0;
      }

      clearC4Cursor();
      drawC4Cursor(c4SelectedCol, true);
    }

    if (
      currentSelectState == LOW &&
      c4LastSelectState == HIGH &&
      currentTime - c4LastSelectPressTime > c4DebounceDelay
    ) {
      c4LastSelectPressTime = currentTime;

      if (!c4ColumnFull(c4SelectedCol)) {
        bool placed = dropC4Piece(c4SelectedCol);

        if (placed) {
          if (checkC4Win(c4CurrentPlayer)) {
            showC4Winner(c4CurrentPlayer);
            return;
          }

          if (checkC4Draw()) {
            showC4DrawScreen();
            return;
          }

          if (c4CurrentPlayer == 1) {
            c4CurrentPlayer = 2;
          } else {
            c4CurrentPlayer = 1;
          }

          drawC4Cursor(c4SelectedCol, true);
        }
      }
    }
  }

  c4LastUpState = currentUpState;
  c4LastDownState = currentDownState;
  c4LastLeftState = currentLeftState;
  c4LastRightState = currentRightState;
  c4LastSelectState = currentSelectState;
}

void drawConnect4Menu() {
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);

  String title = "CONNECT 4";
  int titleWidth = title.length() * 6 * 2;
  int titleX = (tft.width() - titleWidth) / 2;

  tft.setCursor(titleX, 10);
  tft.println(title);

  drawConnect4Icon(tft.width() / 2, 62);

  for (int i = 0; i < C4_OPTION_COUNT; i++) {
    drawConnect4Option(i, i == selectedC4Option);
  }
}
void drawConnect4Option(int index, bool selected) {
  int y = 100 + (index * 16);

  tft.fillRect(0, y, tft.width(), 14, ST77XX_BLACK);

  String optionText;

  if (selected) {
    optionText = "> " + c4Options[index];
    tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  } else {
    optionText = "  " + c4Options[index];
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  }

  tft.setTextSize(1);

  int textWidth = optionText.length() * 6;
  int x = (tft.width() - textWidth) / 2;

  tft.setCursor(x, y);
  tft.print(optionText);
}
void drawConnect4Icon(int x, int y) {
  int iconW = 64;
  int iconH = 42;

  int startX = x - (iconW / 2);
  int startY = y - (iconH / 2);

  tft.fillRect(startX, startY, iconW, iconH, ST77XX_BLUE);

  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 4; col++) {
      int cx = startX + 8 + (col * 16);
      int cy = startY + 8 + (row * 12);

      if ((row + col) % 2 == 0) {
        tft.fillCircle(cx, cy, 4, ST77XX_RED);
      } else {
        tft.fillCircle(cx, cy, 4, ST77XX_CYAN);
      }
    }
  }
}
void startConnect4Game() {
  c4State = C4_PLAYING;

  c4SelectedCol = 3;
  c4CurrentPlayer = 1;

  // Center board automatically
  c4BoardX = (tft.width() - (C4_COLS * c4CellSize)) / 2;
  c4BoardY = 28;

  for (int row = 0; row < C4_ROWS; row++) {
    for (int col = 0; col < C4_COLS; col++) {
      c4Board[row][col] = 0;
    }
  }

  c4LastUpState = HIGH;
  c4LastDownState = HIGH;
  c4LastLeftState = HIGH;
  c4LastRightState = HIGH;
  c4LastSelectState = HIGH;

  c4LastUpPressTime = 0;
  c4LastDownPressTime = 0;
  c4LastLeftPressTime = 0;
  c4LastRightPressTime = 0;
  c4LastSelectPressTime = 0;

  tft.fillScreen(ST77XX_BLACK);

  drawConnect4Board();
  drawC4Cursor(c4SelectedCol, true);
}

void drawConnect4Board() {
  int boardWidth = C4_COLS * c4CellSize;
  int boardHeight = C4_ROWS * c4CellSize;

  tft.fillRect(c4BoardX - 2, c4BoardY - 2, boardWidth + 4, boardHeight + 4, ST77XX_BLUE);

  for (int row = 0; row < C4_ROWS; row++) {
    for (int col = 0; col < C4_COLS; col++) {
      drawC4Piece(row, col);
    }
  }
}

void drawC4Cursor(int col, bool selected) {
  int x = c4BoardX + (col * c4CellSize) + (c4CellSize / 2);
  int y = 12;

  clearC4Cursor();

  if (c4CurrentPlayer == 1) {
    tft.fillTriangle(x, y + 8, x - 5, y, x + 5, y, ST77XX_RED);
  } else {
    tft.fillTriangle(x, y + 8, x - 5, y, x + 5, y, ST77XX_CYAN);
  }
}

void clearC4Cursor() {
  tft.fillRect(0, 5, tft.width(), 18, ST77XX_BLACK);
}

void drawC4Piece(int row, int col) {
  int x = c4BoardX + (col * c4CellSize) + (c4CellSize / 2);
  int y = c4BoardY + (row * c4CellSize) + (c4CellSize / 2);

  int radius = 5;

  if (c4Board[row][col] == 0) {
    tft.fillCircle(x, y, radius, ST77XX_BLACK);
  } 
  else if (c4Board[row][col] == 1) {
    tft.fillCircle(x, y, radius, ST77XX_RED);
  } 
  else if (c4Board[row][col] == 2) {
    tft.fillCircle(x, y, radius, ST77XX_CYAN);
  }
}

bool dropC4Piece(int col) {
  for (int row = C4_ROWS - 1; row >= 0; row--) {
    if (c4Board[row][col] == 0) {
      c4Board[row][col] = c4CurrentPlayer;
      drawC4Piece(row, col);
      return true;
    }
  }

  return false;
}

bool c4ColumnFull(int col) {
  return c4Board[0][col] != 0;
}

bool checkC4Win(int player) {
  // Horizontal
  for (int row = 0; row < C4_ROWS; row++) {
    for (int col = 0; col <= C4_COLS - 4; col++) {
      if (c4Board[row][col] == player &&
          c4Board[row][col + 1] == player &&
          c4Board[row][col + 2] == player &&
          c4Board[row][col + 3] == player) {
        return true;
      }
    }
  }

  // Vertical
  for (int col = 0; col < C4_COLS; col++) {
    for (int row = 0; row <= C4_ROWS - 4; row++) {
      if (c4Board[row][col] == player &&
          c4Board[row + 1][col] == player &&
          c4Board[row + 2][col] == player &&
          c4Board[row + 3][col] == player) {
        return true;
      }
    }
  }

  // Diagonal down-right
  for (int row = 0; row <= C4_ROWS - 4; row++) {
    for (int col = 0; col <= C4_COLS - 4; col++) {
      if (c4Board[row][col] == player &&
          c4Board[row + 1][col + 1] == player &&
          c4Board[row + 2][col + 2] == player &&
          c4Board[row + 3][col + 3] == player) {
        return true;
      }
    }
  }

  // Diagonal up-right
  for (int row = 3; row < C4_ROWS; row++) {
    for (int col = 0; col <= C4_COLS - 4; col++) {
      if (c4Board[row][col] == player &&
          c4Board[row - 1][col + 1] == player &&
          c4Board[row - 2][col + 2] == player &&
          c4Board[row - 3][col + 3] == player) {
        return true;
      }
    }
  }

  return false;
}

bool checkC4Draw() {
  for (int col = 0; col < C4_COLS; col++) {
    if (c4Board[0][col] == 0) {
      return false;
    }
  }

  return true;
}

void showC4Winner(int player) {
  c4State = C4_GAME_OVER;

  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(2);

  if (player == 1) {
    tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
    tft.setCursor(35, 35);
    tft.println("RED");
  } else {
    tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
    tft.setCursor(30, 35);
    tft.println("BLUE");
  }

  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(35, 65);
  tft.println("WINS");

  delay(2000);

  showPlayAgainScreen();
}

void showC4DrawScreen() {
  c4State = C4_GAME_OVER;

  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
  tft.setCursor(35, 45);
  tft.println("DRAW");

  delay(2000);

  showPlayAgainScreen();
}