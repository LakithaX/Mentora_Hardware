#ifndef MENTORA_TILT_SWITCH_H
#define MENTORA_TILT_SWITCH_H

#include <Arduino.h>

class TiltSwitch {
private:
    int pin;
    bool lastState;
    bool currentState;
    bool lifted;
    bool tilted;
    unsigned long lastStateChange;
    unsigned long tiltStartTime;
    unsigned long liftStartTime;
    int humorResponseIndex;
    bool hasResponded;
    const unsigned long DEBOUNCE_DELAY = 100;
    const unsigned long MIN_TILT_TIME = 500;

public:
    explicit TiltSwitch(int tiltPin);
    void begin();
    void update();
    bool isCurrentlyTilted();
    bool isCurrentlyLifted();
    bool justTilted();
    bool justPutDown();
    String getHumorResponse();
    String getContextualResponse(String currentActivity);
    void resetHumorIndex();
};

#endif

