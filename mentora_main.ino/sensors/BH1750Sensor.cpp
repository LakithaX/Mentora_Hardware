#include "BH1750Sensor.h"

BH1750Sensor::BH1750Sensor() : currentLux(0.0f), lastReading(0) {}

bool BH1750Sensor::begin() {
    Wire.begin();
    if (lightMeter.begin()) {
        Serial.println("BH1750 initialized successfully");
        return true;
    }
    Serial.println("Error initializing BH1750");
    return false;
}

bool BH1750Sensor::updateReading() {
    unsigned long now = millis();
    if (now - lastReading >= READING_INTERVAL) {
        currentLux = lightMeter.readLightLevel();
        lastReading = now;
        return true;
    }
    return false;
}

float BH1750Sensor::getLux() { return currentLux; }

String BH1750Sensor::getLightLevel() {
    if (currentLux < 10) return "Very Dark";
    if (currentLux < 50) return "Dark";
    if (currentLux < 200) return "Dim";
    if (currentLux < 500) return "Good";
    if (currentLux < 1000) return "Bright";
    return "Very Bright";
}

bool BH1750Sensor::isGoodForStudying() { return currentLux >= 300 && currentLux <= 750; }
bool BH1750Sensor::isDarkEnvironment() { return currentLux < 100; }
bool BH1750Sensor::isBrightEnvironment() { return currentLux > 1000; }

String BH1750Sensor::getLightRecommendation() {
    if (isDarkEnvironment()) return "Too dark for studying! Turn on more lights.";
    if (isBrightEnvironment()) return "Very bright! Consider reducing glare.";
    if (isGoodForStudying()) return "Perfect lighting for studying!";
    return "Lighting is okay, but could be better.";
}

