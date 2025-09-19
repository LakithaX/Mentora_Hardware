#ifndef MENTORA_TTP223_TOUCH_H
#define MENTORA_TTP223_TOUCH_H

#include <Arduino.h>

class TTP223Touch {
private:
    int touchPin1;
    int touchPin2;
    bool state1;
    bool state2;
    bool last1;
    bool last2;
    unsigned long lastChange1;
    unsigned long lastChange2;
    const unsigned long DEBOUNCE_MS = 40;
    String lastPattern;
    unsigned long lastPatternTime;

public:
    TTP223Touch(int pin1, int pin2);
    void begin();
    void update();
    bool isTouch1();
    bool isTouch2();
    String getTouchPattern();
    String getTouchResponse();
};

#endif

