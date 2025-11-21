#include "button.h"

ButtonHandler::ButtonHandler(uint8_t buttonPin, bool capacitiveTouch)
    : pin(buttonPin), lastState(false), pressStartTime(0),
      lastDebounceTime(0), isPressed(false), touchBaseline(0),
      touchThreshold(0), useCapacitiveTouch(capacitiveTouch) {
}

void ButtonHandler::init() {
    if (useCapacitiveTouch) {
        // External capacitive touch module (e.g., TTP223)
        // These modules output HIGH when touched, LOW when not touched
        pinMode(pin, INPUT);
        Serial.print("Initializing capacitive touch module on GPIO");
        Serial.println(pin);

        // Calibrate to detect initial state
        calibrateTouch();

        Serial.print("Touch sensor ready (active ");
        Serial.print(touchBaseline > 512 ? "HIGH" : "LOW");
        Serial.println(")");
    } else {
        // Regular button with pull-up (LOW when pressed)
        pinMode(pin, INPUT_PULLUP);
        lastState = digitalRead(pin) == LOW;
        Serial.print("Regular button initialized on GPIO");
        Serial.println(pin);
    }
}

void ButtonHandler::calibrateTouch() {
    // For external capacitive touch modules (ESP32-C3 doesn't have built-in touch)
    // Read the initial state to determine active HIGH or LOW
    delay(100); // Let sensor stabilize

    uint32_t sum = 0;
    const int samples = 10;

    for (int i = 0; i < samples; i++) {
        sum += analogRead(pin);
        delay(10);
    }

    touchBaseline = sum / samples;

    // Most capacitive touch modules output HIGH (3.3V) when touched
    // Set threshold at midpoint (1.65V ~ 2048 for 12-bit ADC)
    touchThreshold = 2048;
}

bool ButtonHandler::isTouched() {
    if (!useCapacitiveTouch) {
        // Regular button: LOW when pressed (pull-up)
        return digitalRead(pin) == LOW;
    }

    // External capacitive touch module
    // Use analog reading since digitalRead doesn't work reliably on this pin
    // Not touched: ~6-20
    // Touched: ~4095
    // Threshold at 2000 (midpoint)
    int analogValue = analogRead(pin);

    return analogValue > 2000;
}

ButtonEvent ButtonHandler::check() {
    bool currentState = isTouched();
    unsigned long now = millis();

    // Debounce
    if (currentState != lastState) {
        lastDebounceTime = now;
    }

    if ((now - lastDebounceTime) > DEBOUNCE_DELAY) {
        // Button pressed
        if (currentState && !isPressed) {
            isPressed = true;
            pressStartTime = now;
            Serial.println("Button touched");
        }

        // Button released
        if (!currentState && isPressed) {
            isPressed = false;
            unsigned long pressDuration = now - pressStartTime;

            Serial.print("Button released after ");
            Serial.print(pressDuration);
            Serial.println(" ms");

            // Long press: 4.2+ seconds
            if (pressDuration >= LONG_PRESS_MIN) {
                Serial.println("Long press triggered");
                return LONG_PRESS;
            }

            // Short press: < 1 second
            if (pressDuration >= DEBOUNCE_DELAY && pressDuration < SHORT_PRESS_MAX) {
                Serial.println("Short press triggered");
                return SHORT_PRESS;
            }
        }
    }

    lastState = currentState;
    return NONE;
}

bool ButtonHandler::isCurrentlyPressed() {
    return isPressed;
}

unsigned long ButtonHandler::getCurrentPressDuration() {
    if (!isPressed) return 0;
    return millis() - pressStartTime;
}
