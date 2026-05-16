#include <Arduino.h>

#include "button.hpp"
#include "reaction.hpp"
#include "display.hpp"
#include "app.hpp"

const int REACTION_OPTION_COUNT = 2;
int selectedReactionOption = 0;

String reactionOptions[REACTION_OPTION_COUNT] = {
  "PLAY",
  "BACK"
};

// Reaction menu button edge detection
bool reactionLastUpState = HIGH;
bool reactionLastDownState = HIGH;
bool reactionLastSelectState = HIGH;

void drawReactionMenu();
void drawReactionIcon(int x, int y);
void drawReactionOption(int index, bool selected);
int getRandomDelayMs();

bool isSelectPressed();
void waitForSelectReleased();
bool stableSelectPress();

void initReactionTime() {
  selectedReactionOption = 0;

  reactionLastUpState = HIGH;
  reactionLastDownState = HIGH;
  reactionLastSelectState = HIGH;

  drawReactionMenu();
}

void drawReactionMenu() {
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
  tft.setTextSize(2);

  // Center REACTION
  String title1 = "REACTION";
  int title1Width = title1.length() * 6 * 2;
  int title1X = (tft.width() - title1Width) / 2;

  tft.setCursor(title1X, 6);
  tft.println(title1);

  // Center TIME
  String title2 = "TIME";
  int title2Width = title2.length() * 6 * 2;
  int title2X = (tft.width() - title2Width) / 2;

  tft.setCursor(title2X, 28);
  tft.println(title2);

  // Center icon under title
  drawReactionIcon(tft.width() / 2, 64);

  // Center PLAY / BACK options under icon
  for (int i = 0; i < REACTION_OPTION_COUNT; i++) {
    drawReactionOption(i, i == selectedReactionOption);
  }
}void drawReactionIcon(int x, int y) {
  // Outer circle
  tft.drawCircle(x, y, 15, ST77XX_WHITE);
  tft.drawCircle(x, y, 14, ST77XX_WHITE);

  // Inner red center
  tft.fillCircle(x, y, 6, ST77XX_RED);

  // Lightning bolt
  tft.fillTriangle(x - 3, y - 13, x + 4, y - 13, x - 1, y - 2, ST77XX_YELLOW);
  tft.fillTriangle(x - 1, y - 2, x + 6, y - 2, x - 5, y + 13, ST77XX_YELLOW);

  // Small button base
  tft.fillRect(x - 18, y + 18, 36, 5, ST77XX_WHITE);
  tft.fillRect(x - 12, y + 23, 24, 4, ST77XX_WHITE);
}void drawReactionOption(int index, bool selected) {
  int y = 100 + (index * 16);

  tft.fillRect(0, y, tft.width(), 14, ST77XX_BLACK);

  String optionText;

  if (selected) {
    optionText = "> " + reactionOptions[index];
    tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  } else {
    optionText = "  " + reactionOptions[index];
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  }

  tft.setTextSize(1);

  int textWidth = optionText.length() * 6;
  int x = (tft.width() - textWidth) / 2;

  tft.setCursor(x, y);
  tft.print(optionText);
}

void updateReactionTime() {
  bool currentUpState = digitalRead(UP_BUTTON_GPIO);
  bool currentDownState = digitalRead(DOWN_BUTTON_GPIO);
  bool currentSelectState = digitalRead(SELECT_BUTTON_GPIO);

  // DOWN button: one move per click
  if (currentDownState == LOW && reactionLastDownState == HIGH) {
    int oldSelectedOption = selectedReactionOption;

    selectedReactionOption++;

    if (selectedReactionOption >= REACTION_OPTION_COUNT) {
      selectedReactionOption = 0;
    }

    drawReactionOption(oldSelectedOption, false);
    drawReactionOption(selectedReactionOption, true);
  }

  // UP button: one move per click
  if (currentUpState == LOW && reactionLastUpState == HIGH) {
    int oldSelectedOption = selectedReactionOption;

    selectedReactionOption--;

    if (selectedReactionOption < 0) {
      selectedReactionOption = REACTION_OPTION_COUNT - 1;
    }

    drawReactionOption(oldSelectedOption, false);
    drawReactionOption(selectedReactionOption, true);
  }

  // SELECT button: one select per click
  if (currentSelectState == LOW && reactionLastSelectState == HIGH) {
    if (selectedReactionOption == 0) {
      Serial.println("Reaction Test Started");

      int waitTimeMs = getRandomDelayMs();
      reactionGame(waitTimeMs);
    } 
    else if (selectedReactionOption == 1) {
      Serial.println("Back to home");

      showHomeMenuScreen();
    }
  }

  reactionLastUpState = currentUpState;
  reactionLastDownState = currentDownState;
  reactionLastSelectState = currentSelectState;
}

int getRandomDelayMs() {
  return random(1000, 5001); // 1 to 5 seconds
}

bool isSelectPressed() {
  return digitalRead(SELECT_BUTTON_GPIO) == LOW;
}

void waitForSelectReleased() {
  while (isSelectPressed()) {
    delay(1);
  }

  delay(20);
}

bool stableSelectPress() {
  if (isSelectPressed()) {
    delay(10);

    if (isSelectPressed()) {
      return true;
    }
  }

  return false;
}

void reactionGame(int waitTimeMs) {
  // Make sure SELECT from menu is released before starting
  waitForSelectReleased();

  // WAIT screen
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(35, 35);
  tft.println("WAIT");

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(28, 80);
  tft.println("Press when red");

  unsigned long waitStartTime = millis();

  // Random wait period
  while (millis() - waitStartTime < waitTimeMs) {
    if (stableSelectPress()) {
      tft.fillScreen(ST77XX_BLACK);

      tft.setTextSize(2);
      tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
      tft.setCursor(15, 45);
      tft.println("TOO EARLY");

      delay(1500);
      showPlayAgainScreen();
      return;
    }
  }

  // RED = press now
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
  tft.setCursor(30, 35);
  tft.println("PRESS");

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(38, 80);
  tft.println("Press SELECT");

  unsigned long reactionStartTime = micros();

  // Wait for SELECT press
  while (!isSelectPressed()) {
    delayMicroseconds(100);
  }

  unsigned long reactionTimeUs = micros() - reactionStartTime;
  float reactionTimeMs = reactionTimeUs / 1000.0;

  // Show result
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
  tft.setCursor(20, 25);
  tft.println("TIME:");

  tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  tft.setCursor(10, 60);
  tft.print(reactionTimeMs, 2);
  tft.println(" ms");

  delay(2000);

  showPlayAgainScreen();
}