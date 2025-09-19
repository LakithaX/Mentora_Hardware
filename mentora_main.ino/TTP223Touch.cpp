#include "TTP223Touch.h"

TTP223Touch::TTP223Touch(int pin1, int pin2)
    : touchPin1(pin1), touchPin2(pin2), state1(false), state2(false), last1(false), last2(false),
      lastChange1(0), lastChange2(0), lastPattern(""), lastPatternTime(0) {}

void TTP223Touch::begin() {
    pinMode(touchPin1, INPUT);
    pinMode(touchPin2, INPUT);
    state1 = digitalRead(touchPin1);
    state2 = digitalRead(touchPin2);
    last1 = state1;
    last2 = state2;
}

void TTP223Touch::update() {
    unsigned long now = millis();
    bool r1 = digitalRead(touchPin1);
    bool r2 = digitalRead(touchPin2);
    if (r1 != last1 && (now - lastChange1) > DEBOUNCE_MS) {
        lastChange1 = now;
        last1 = state1;
        state1 = r1;
        if (state1) { lastPattern = "TOUCH1"; lastPatternTime = now; }
    }
    if (r2 != last2 && (now - lastChange2) > DEBOUNCE_MS) {
        lastChange2 = now;
        last2 = state2;
        state2 = r2;
        if (state2) { lastPattern = "TOUCH2"; lastPatternTime = now; }
    }
}

bool TTP223Touch::isTouch1() { return state1; }
bool TTP223Touch::isTouch2() { return state2; }

String TTP223Touch::getTouchPattern() { return lastPattern; }

String TTP223Touch::getTouchResponse() {
    if (lastPattern == "TOUCH1") return "Reaction: Play fun animation";
    if (lastPattern == "TOUCH2") return "Toggle Study Mode";
    return "";
}

