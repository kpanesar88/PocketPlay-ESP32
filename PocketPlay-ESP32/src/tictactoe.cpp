#include <Arduino.h>

#include "button.hpp"
#include "tictactoe.hpp"
#include "display.hpp"
#include "app.hpp"

const int TTT_OPTION_COUNT = 2;
int selectedTTTOption = 0;

String tttOptions[TTT_OPTION_COUNT] = {
  "PLAY",
  "BACK"
};

enum TicTacToeState {
  TTT_MENU,
  TTT_PLAYING,
  TTT_GAME_OVER
};

TicTacToeState tttState = TTT_MENU;

// Button states
bool tttLastUpState = HIGH;
bool tttLastDownState = HIGH;
bool tttLastLeftState = HIGH;
bool tttLastRightState = HIGH;
bool tttLastSelectState = HIGH;

// Debounce
unsigned long tttLastUpPressTime = 0;
unsigned long tttLastDownPressTime = 0;
unsigned long tttLastLeftPressTime = 0;
unsigned long tttLastRightPressTime = 0;
unsigned long tttLastSelectPressTime = 0;

const int tttDebounceDelay = 150;

// Board
char tttBoard[3][3];

int tttSelectedRow = 0;
int tttSelectedCol = 0;

char tttCurrentPlayer = 'X';

// Centered board settings
int tttGridX = 0;
int tttGridY = 28;
const int tttCellSize = 30;

// Function declarations
void drawTicTacToeMenu();
void drawTicTacToeOption(int index, bool selected);
void drawTicTacToeIcon(int x, int y);

void startTicTacToeGame();
void drawTicTacToeBoard();
void drawTicTacToeCell(int row, int col, bool selected);

void moveTicTacToeCursor(int rowChange, int colChange);
void placeTicTacToePiece();

bool checkTicTacToeWin(char player);
bool checkTicTacToeDraw();

void showTicTacToeWinner(char winner);
void showTicTacToeDraw();

void initTicTacToe() {
  selectedTTTOption = 0;
  tttState = TTT_MENU;

  tttLastUpState = HIGH;
  tttLastDownState = HIGH;
  tttLastLeftState = HIGH;
  tttLastRightState = HIGH;
  tttLastSelectState = HIGH;

  tttLastUpPressTime = 0;
  tttLastDownPressTime = 0;
  tttLastLeftPressTime = 0;
  tttLastRightPressTime = 0;
  tttLastSelectPressTime = 0;

  drawTicTacToeMenu();
}

void updateTicTacToe() {
  bool currentUpState = digitalRead(UP_BUTTON_GPIO);
  bool currentDownState = digitalRead(DOWN_BUTTON_GPIO);
  bool currentLeftState = digitalRead(LEFT_BUTTON_GPIO);
  bool currentRightState = digitalRead(RIGHT_BUTTON_GPIO);
  bool currentSelectState = digitalRead(SELECT_BUTTON_GPIO);

  unsigned long currentTime = millis();

  if (tttState == TTT_MENU) {
    if (
      currentDownState == LOW &&
      tttLastDownState == HIGH &&
      currentTime - tttLastDownPressTime > tttDebounceDelay
    ) {
      tttLastDownPressTime = currentTime;

      int oldOption = selectedTTTOption;

      selectedTTTOption++;

      if (selectedTTTOption >= TTT_OPTION_COUNT) {
        selectedTTTOption = 0;
      }

      drawTicTacToeOption(oldOption, false);
      drawTicTacToeOption(selectedTTTOption, true);
    }

    if (
      currentUpState == LOW &&
      tttLastUpState == HIGH &&
      currentTime - tttLastUpPressTime > tttDebounceDelay
    ) {
      tttLastUpPressTime = currentTime;

      int oldOption = selectedTTTOption;

      selectedTTTOption--;

      if (selectedTTTOption < 0) {
        selectedTTTOption = TTT_OPTION_COUNT - 1;
      }

      drawTicTacToeOption(oldOption, false);
      drawTicTacToeOption(selectedTTTOption, true);
    }

    if (
      currentSelectState == LOW &&
      tttLastSelectState == HIGH &&
      currentTime - tttLastSelectPressTime > tttDebounceDelay
    ) {
      tttLastSelectPressTime = currentTime;

      if (selectedTTTOption == 0) {
        Serial.println("Tic Tac Toe started");
        startTicTacToeGame();
      } else {
        Serial.println("Back to home");
        showHomeMenuScreen();
      }
    }

    tttLastUpState = currentUpState;
    tttLastDownState = currentDownState;
    tttLastSelectState = currentSelectState;

    return;
  }

  if (tttState == TTT_PLAYING) {
    if (
      currentUpState == LOW &&
      tttLastUpState == HIGH &&
      currentTime - tttLastUpPressTime > tttDebounceDelay
    ) {
      tttLastUpPressTime = currentTime;
      moveTicTacToeCursor(-1, 0);
    }

    if (
      currentDownState == LOW &&
      tttLastDownState == HIGH &&
      currentTime - tttLastDownPressTime > tttDebounceDelay
    ) {
      tttLastDownPressTime = currentTime;
      moveTicTacToeCursor(1, 0);
    }

    if (
      currentLeftState == LOW &&
      tttLastLeftState == HIGH &&
      currentTime - tttLastLeftPressTime > tttDebounceDelay
    ) {
      tttLastLeftPressTime = currentTime;
      moveTicTacToeCursor(0, -1);
    }

    if (
      currentRightState == LOW &&
      tttLastRightState == HIGH &&
      currentTime - tttLastRightPressTime > tttDebounceDelay
    ) {
      tttLastRightPressTime = currentTime;
      moveTicTacToeCursor(0, 1);
    }

    if (
      currentSelectState == LOW &&
      tttLastSelectState == HIGH &&
      currentTime - tttLastSelectPressTime > tttDebounceDelay
    ) {
      tttLastSelectPressTime = currentTime;
      placeTicTacToePiece();
    }
  }

  tttLastUpState = currentUpState;
  tttLastDownState = currentDownState;
  tttLastLeftState = currentLeftState;
  tttLastRightState = currentRightState;
  tttLastSelectState = currentSelectState;
}

void drawTicTacToeMenu() {
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);

  String title = "TIC TAC";
  int titleWidth = title.length() * 6 * 2;
  int titleX = (tft.width() - titleWidth) / 2;

  tft.setCursor(titleX, 8);
  tft.println(title);

  String title2 = "TOE";
  int title2Width = title2.length() * 6 * 2;
  int title2X = (tft.width() - title2Width) / 2;

  tft.setCursor(title2X, 32);
  tft.println(title2);

  drawTicTacToeIcon(tft.width() / 2, 68);

  for (int i = 0; i < TTT_OPTION_COUNT; i++) {
    drawTicTacToeOption(i, i == selectedTTTOption);
  }
}

void drawTicTacToeOption(int index, bool selected) {
  int y = 100 + (index * 16);

  tft.fillRect(0, y, tft.width(), 14, ST77XX_BLACK);

  String optionText;

  if (selected) {
    optionText = "> " + tttOptions[index];
    tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  } else {
    optionText = "  " + tttOptions[index];
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  }

  int textWidth = optionText.length() * 6;
  int x = (tft.width() - textWidth) / 2;

  tft.setCursor(x, y);
  tft.setTextSize(1);
  tft.print(optionText);
}

void drawTicTacToeIcon(int x, int y) {
  int size = 42;
  int cell = size / 3;
  int startX = x - size / 2;
  int startY = y - size / 2;

  tft.drawRect(startX, startY, size, size, ST77XX_WHITE);

  tft.drawFastVLine(startX + cell, startY, size, ST77XX_WHITE);
  tft.drawFastVLine(startX + cell * 2, startY, size, ST77XX_WHITE);

  tft.drawFastHLine(startX, startY + cell, size, ST77XX_WHITE);
  tft.drawFastHLine(startX, startY + cell * 2, size, ST77XX_WHITE);

  tft.setTextSize(1);

  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
  tft.setCursor(startX + 6, startY + 5);
  tft.print("X");

  tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
  tft.setCursor(startX + cell + 6, startY + cell + 5);
  tft.print("O");

  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
  tft.setCursor(startX + (cell * 2) + 6, startY + (cell * 2) + 5);
  tft.print("X");
}

void startTicTacToeGame() {
  tttState = TTT_PLAYING;

  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 3; col++) {
      tttBoard[row][col] = ' ';
    }
  }

  tttSelectedRow = 0;
  tttSelectedCol = 0;
  tttCurrentPlayer = 'X';

  // Center the 3x3 board on screen
  tttGridX = (tft.width() - (tttCellSize * 3)) / 2;
  tttGridY = 28;

  tft.fillScreen(ST77XX_BLACK);

  drawTicTacToeBoard();
}

void drawTicTacToeBoard() {
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
  tft.setCursor(4, 4);
  tft.print("TURN: ");
  tft.print(tttCurrentPlayer);

  // Grid outline
  tft.drawRect(tttGridX, tttGridY, tttCellSize * 3, tttCellSize * 3, ST77XX_WHITE);

  // Vertical lines
  tft.drawFastVLine(tttGridX + tttCellSize, tttGridY, tttCellSize * 3, ST77XX_WHITE);
  tft.drawFastVLine(tttGridX + tttCellSize * 2, tttGridY, tttCellSize * 3, ST77XX_WHITE);

  // Horizontal lines
  tft.drawFastHLine(tttGridX, tttGridY + tttCellSize, tttCellSize * 3, ST77XX_WHITE);
  tft.drawFastHLine(tttGridX, tttGridY + tttCellSize * 2, tttCellSize * 3, ST77XX_WHITE);

  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 3; col++) {
      drawTicTacToeCell(row, col, row == tttSelectedRow && col == tttSelectedCol);
    }
  }
}

void drawTicTacToeCell(int row, int col, bool selected) {
  int x = tttGridX + (col * tttCellSize);
  int y = tttGridY + (row * tttCellSize);

  // Clear inside cell only
  tft.fillRect(x + 2, y + 2, tttCellSize - 4, tttCellSize - 4, ST77XX_BLACK);

  if (selected) {
    tft.drawRect(x + 2, y + 2, tttCellSize - 4, tttCellSize - 4, ST77XX_GREEN);
  }

  char piece = tttBoard[row][col];

  if (piece == 'X') {
    tft.setTextSize(3);
    tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
    tft.setCursor(x + 7, y + 4);
    tft.print("X");
  } else if (piece == 'O') {
    tft.setTextSize(3);
    tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
    tft.setCursor(x + 7, y + 4);
    tft.print("O");
  }
}

void moveTicTacToeCursor(int rowChange, int colChange) {
  int oldRow = tttSelectedRow;
  int oldCol = tttSelectedCol;

  tttSelectedRow += rowChange;
  tttSelectedCol += colChange;

  if (tttSelectedRow < 0) {
    tttSelectedRow = 2;
  }

  if (tttSelectedRow > 2) {
    tttSelectedRow = 0;
  }

  if (tttSelectedCol < 0) {
    tttSelectedCol = 2;
  }

  if (tttSelectedCol > 2) {
    tttSelectedCol = 0;
  }

  drawTicTacToeCell(oldRow, oldCol, false);
  drawTicTacToeCell(tttSelectedRow, tttSelectedCol, true);
}

void placeTicTacToePiece() {
  if (tttBoard[tttSelectedRow][tttSelectedCol] != ' ') {
    return;
  }

  tttBoard[tttSelectedRow][tttSelectedCol] = tttCurrentPlayer;

  drawTicTacToeCell(tttSelectedRow, tttSelectedCol, true);

  if (checkTicTacToeWin(tttCurrentPlayer)) {
    showTicTacToeWinner(tttCurrentPlayer);
    return;
  }

  if (checkTicTacToeDraw()) {
    showTicTacToeDraw();
    return;
  }

  if (tttCurrentPlayer == 'X') {
    tttCurrentPlayer = 'O';
  } else {
    tttCurrentPlayer = 'X';
  }

  tft.fillRect(0, 0, tft.width(), 14, ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
  tft.setCursor(4, 4);
  tft.print("TURN: ");
  tft.print(tttCurrentPlayer);
}

bool checkTicTacToeWin(char player) {
  for (int row = 0; row < 3; row++) {
    if (
      tttBoard[row][0] == player &&
      tttBoard[row][1] == player &&
      tttBoard[row][2] == player
    ) {
      return true;
    }
  }

  for (int col = 0; col < 3; col++) {
    if (
      tttBoard[0][col] == player &&
      tttBoard[1][col] == player &&
      tttBoard[2][col] == player
    ) {
      return true;
    }
  }

  if (
    tttBoard[0][0] == player &&
    tttBoard[1][1] == player &&
    tttBoard[2][2] == player
  ) {
    return true;
  }

  if (
    tttBoard[0][2] == player &&
    tttBoard[1][1] == player &&
    tttBoard[2][0] == player
  ) {
    return true;
  }

  return false;
}

bool checkTicTacToeDraw() {
  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 3; col++) {
      if (tttBoard[row][col] == ' ') {
        return false;
      }
    }
  }

  return true;
}

void showTicTacToeWinner(char winner) {
  tttState = TTT_GAME_OVER;

  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  tft.setCursor(20, 25);
  tft.print(winner);
  tft.println(" WINS");

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(35, 70);
  tft.println("Nice move!");

  delay(2000);

  showPlayAgainScreen();
}

void showTicTacToeDraw() {
  tttState = TTT_GAME_OVER;

  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
  tft.setCursor(35, 35);
  tft.println("DRAW");

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(30, 70);
  tft.println("No winner");

  delay(2000);

  showPlayAgainScreen();
}