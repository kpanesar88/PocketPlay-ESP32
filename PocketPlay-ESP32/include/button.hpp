#ifndef BUTTON_HPP
#define BUTTON_HPP

#include <Arduino.h>

// Button pins
#define UP_BUTTON_GPIO 32
#define DOWN_BUTTON_GPIO 33
#define RIGHT_BUTTON_GPIO 26
#define LEFT_BUTTON_GPIO 25
#define SELECT_BUTTON_GPIO 27

struct Button {
  int pin;
  const char* name;
  bool lastStableState;
  bool lastReading;
  unsigned long lastDebounceTime;
};

void setupButtons();
bool isButtonPressed(Button &button);
void checkButtons();

#endif