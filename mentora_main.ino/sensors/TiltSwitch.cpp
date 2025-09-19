#include "TiltSwitch.h"

TiltSwitch::TiltSwitch(int tiltPin)
    : pin(tiltPin), lastState(false), currentState(false), lifted(false), tilted(false),
      lastStateChange(0), tiltStartTime(0), liftStartTime(0), humorResponseIndex(0), hasResponded(false) {}

void TiltSwitch::begin() {
    pinMode(pin, INPUT_PULLUP);
    currentState = digitalRead(pin);
    lastState = currentState;
}

void TiltSwitch::update() {
    unsigned long now = millis();
    bool reading = digitalRead(pin);
    if (reading != lastState && (now - lastStateChange) > DEBOUNCE_DELAY) {
        lastStateChange = now;
        lastState = currentState;
        currentState = reading;

        if (currentState != lastState) {
            if (currentState == HIGH) {
                if (!tilted && !lifted) {
                    tiltStartTime = now;
                    liftStartTime = now;
                }
                tilted = true;
                lifted = true;
                hasResponded = false;
            } else {
                tilted = false;
                lifted = false;
                hasResponded = false;
            }
        }
    }
    if (tilted && (now - tiltStartTime) > MIN_TILT_TIME) {
        // sustained tilt marker if needed
    }
}

bool TiltSwitch::isCurrentlyTilted() { return tilted; }
bool TiltSwitch::isCurrentlyLifted() { return lifted; }
bool TiltSwitch::justTilted() { return (currentState == HIGH) && (lastState == LOW); }
bool TiltSwitch::justPutDown() { return (currentState == LOW) && (lastState == HIGH); }

String TiltSwitch::getHumorResponse() {
    if (hasResponded) return "";
    static const char* liftedResponses[] = {
        "Whoa! I'm getting dizzy up here!",
        "Hey! I'm not a toy, I'm your study buddy!",
        "The world looks funny upside down!",
        "I prefer staying grounded, literally!",
        "Is this how birds feel? Put me back down!",
        "I'm getting a different perspective on things!",
        "Everything's topsy-turvy! This is making me dizzy!",
        "I think I left my stomach down there!",
        "Houston, we have a problem - I'm floating!",
        "I'm not built for space travel, put me down!"
    };
    static const char* tiltedResponses[] = {
        "I think I need to recalibrate my balance!",
        "Everything's sideways! Are we doing geometry now?",
        "I'm getting a tilted view of the world!",
        "Is this part of the physics lesson?",
        "I feel like I'm on a roller coaster!",
        "This is making me lean into learning!",
        "I'm at an angle! Quick, calculate my degrees!",
        "Gravity is doing interesting things to me!"
    };

    String response = "";
    if (lifted) {
        response = liftedResponses[humorResponseIndex % 10];
        humorResponseIndex++;
        hasResponded = true;
    } else if (tilted) {
        response = tiltedResponses[humorResponseIndex % 8];
        humorResponseIndex++;
        hasResponded = true;
    }
    return response;
}

String TiltSwitch::getContextualResponse(String currentActivity) {
    if (hasResponded) return "";
    String response = "";
    if (lifted) {
        if (currentActivity == "studying") response = "Hey! We're learning! Put me back down!";
        else if (currentActivity == "chatting") response = "I can't chat properly when I'm floating!";
        else if (currentActivity == "idle") response = "Don't put me away yet!";
        else response = getHumorResponse();
        hasResponded = true;
    } else if (justPutDown()) {
        response = "Ahh, much better! Thanks for putting me back down.";
        hasResponded = true;
    }
    return response;
}

void TiltSwitch::resetHumorIndex() { humorResponseIndex = 0; }

