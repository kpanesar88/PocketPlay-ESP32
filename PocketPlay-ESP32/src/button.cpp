#include "button.hpp"

const unsigned long debounceDelay = 50;

Button buttons[] = {
  {UP_BUTTON_GPIO, "UP", HIGH, HIGH, 0},
  {DOWN_BUTTON_GPIO, "DOWN", HIGH, HIGH, 0},
  {LEFT_BUTTON_GPIO, "LEFT", HIGH, HIGH, 0},
  {RIGHT_BUTTON_GPIO, "RIGHT", HIGH, HIGH, 0},
  {SELECT_BUTTON_GPIO, "SELECT", HIGH, HIGH, 0}
};

const int buttonCount = sizeof(buttons) / sizeof(buttons[0]);

void setupButtons() {
  for (int i = 0; i < buttonCount; i++) {
    pinMode(buttons[i].pin, INPUT_PULLUP);
  }
}

bool isButtonPressed(Button &button) {
  bool currentReading = digitalRead(button.pin);

  if (currentReading != button.lastReading) {
    button.lastDebounceTime = millis();
  }

  if ((millis() - button.lastDebounceTime) > debounceDelay) {
    if (currentReading != button.lastStableState) {
      button.lastStableState = currentReading;

      // INPUT_PULLUP means LOW = pressed
      if (button.lastStableState == LOW) {
        return true;
      }
    }
  }

  button.lastReading = currentReading;
  return false;
}

void checkButtons() {
  for (int i = 0; i < buttonCount; i++) {
    if (isButtonPressed(buttons[i])) {
      Serial.print(buttons[i].name);
      Serial.println(" pressed");
    }
  }
}