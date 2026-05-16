#include <Arduino.h>

#include "button.hpp"
#include "brickbreaker.hpp"
#include "connect4.hpp"
#include "dinorun.hpp"
#include "laneracer.hpp"
#include "reaction.hpp"
#include "display.hpp"
#include "app.hpp"
#include "snake.hpp"
#include "starship.hpp"
#include "pong.hpp"
#include "tower.hpp"
#include "tictactoe.hpp"

const int GAME_COUNT = 10;
int selectedGame = 0;
int selectedPlayAgainOption = 0;

void drawGameRow(int index, bool selected);
void drawHomeMenu();

void drawPlayAgain();
void drawPlayAgainOption(int option, bool selected);

void startGames(int selectedGame);
void showHomeMenuScreen();
void showPlayAgainScreen();

String games[GAME_COUNT] = {
  "Snake",
  "Brick Breaker",
  "Connect 4",
  "Dino Run",
  "Reaction Test",
  "Lane Racer",
  "Starship",
  "Pong",
  "Tower",
  "TicTacToe"
};

enum ScreenState {
  HOME_MENU,
  PLAY_AGAIN,
  IN_GAME
};

ScreenState currentScreen = HOME_MENU;

void drawHomeMenu() {
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
  tft.setCursor(6, 5);
  tft.println("PocketPlay");

  for (int i = 0; i < GAME_COUNT; i++) {
    drawGameRow(i, i == selectedGame);
  }
}

void drawGameRow(int index, bool selected) {
  int startY = 35;      // moved up
  int rowSpacing = 12;  // reduced spacing
  int y = startY + (rowSpacing * index);

  // Clear only this row
  tft.fillRect(0, y, tft.width(), 11, ST77XX_BLACK);

  tft.setCursor(8, y);
  tft.setTextSize(1);

  if (selected) {
    tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
    tft.print("> ");
  } else {
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.print("  ");
  }

  tft.println(games[index]);
}

void drawPlayAgain() {
  tft.fillScreen(ST77XX_BLACK);

  tft.setCursor(2, 10);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
  tft.println("PLAY AGAIN");

  drawPlayAgainOption(0, selectedPlayAgainOption == 0);
  drawPlayAgainOption(1, selectedPlayAgainOption == 1);
}

void drawPlayAgainOption(int option, bool selected) {
  int y;

  if (option == 0) {
    y = 60;
  } else {
    y = 90;
  }

  tft.fillRect(20, y, 120, 20, ST77XX_BLACK);

  tft.setCursor(30, y);
  tft.setTextSize(1);

  if (selected) {
    tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
    tft.print("> ");
  } else {
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.print("  ");
  }

  if (option == 0) {
    tft.print("YES");
  } else {
    tft.print("EXIT");
  }
}

void showHomeMenuScreen() {
  currentScreen = HOME_MENU;
  drawHomeMenu();
}

void showPlayAgainScreen() {
  selectedPlayAgainOption = 0;
  currentScreen = PLAY_AGAIN;
  drawPlayAgain();
}

void startGames(int selectedGame) {
  currentScreen = IN_GAME;

  switch (selectedGame) {
    case 0:
      initSnake();
      break;

    case 1:
      initBrickBreaker();
      break;

    case 2:
      initConnect4();
      break;

    case 3:
      initDinoRun();
      break;

    case 4:
      initReactionTime();
      break;

    case 5:
      initLaneRacer();
      break;
    case 6:
      initStarship();
      break;
    case 7:
      initPong();
      break;
    case 8:
      initTower();
      break;
    case 9:
      initTicTacToe();
      break;
    default:
      Serial.println("Invalid Game Selected");
      showHomeMenuScreen();
      break;
  }
}

void setup() {
  Serial.begin(9600);
  delay(500);

  randomSeed(esp_random());

  setupDisplay();
  setupButtons();
  tft.setRotation(4);
  showHomeMenuScreen();
}

void loop() {
  if (currentScreen == HOME_MENU) {
    if (digitalRead(DOWN_BUTTON_GPIO) == LOW) {
      int oldSelectedGame = selectedGame;

      selectedGame++;

      if (selectedGame >= GAME_COUNT) {
        selectedGame = 0;
      }

      drawGameRow(oldSelectedGame, false);
      drawGameRow(selectedGame, true);

      delay(200);
    }

    if (digitalRead(UP_BUTTON_GPIO) == LOW) {
      int oldSelectedGame = selectedGame;

      selectedGame--;

      if (selectedGame < 0) {
        selectedGame = GAME_COUNT - 1;
      }

      drawGameRow(oldSelectedGame, false);
      drawGameRow(selectedGame, true);

      delay(200);
    }

    if (digitalRead(SELECT_BUTTON_GPIO) == LOW) {
      Serial.println("SELECT GAME");

      startGames(selectedGame);

      delay(300);
    }
  }

  else if (currentScreen == PLAY_AGAIN) {
    if (digitalRead(DOWN_BUTTON_GPIO) == LOW || digitalRead(UP_BUTTON_GPIO) == LOW) {
      int oldSelectedOption = selectedPlayAgainOption;

      selectedPlayAgainOption = 1 - selectedPlayAgainOption;

      drawPlayAgainOption(oldSelectedOption, false);
      drawPlayAgainOption(selectedPlayAgainOption, true);

      delay(200);
    }

    if (digitalRead(SELECT_BUTTON_GPIO) == LOW) {
      if (selectedPlayAgainOption == 0) {
        Serial.println("YES selected");

        startGames(selectedGame);
      } 
      else if (selectedPlayAgainOption == 1) {
        Serial.println("EXIT selected");

        showHomeMenuScreen();
      }

      delay(300);
    }
  }

  else if (currentScreen == IN_GAME) {
    switch (selectedGame) {
      case 0:
        updateSnake();
        break;

      case 1:
        updateBrickBreaker();
        break;

      case 2:
        updateConnect4();
        break;

      case 3:
        updateDinoRun();
        break;

      case 4:
        updateReactionTime();
        break;

      case 5:
        updateLaneRacer();
        break;
      
      case 6:
        updateStarship();
        break;

      case 7:
        updatePong();
        break;

      case 8:
        updateTower();
        break;
      
      case 9:
        updateTicTacToe();
        break;

      default:
        showHomeMenuScreen();
        break;
    }
  }
}