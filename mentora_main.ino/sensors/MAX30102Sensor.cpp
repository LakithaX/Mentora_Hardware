#include "MAX30102Sensor.h"

MAX30102Sensor::MAX30102Sensor()
    : isFingerDetected(false), heartRate(0), beatsPerMinute(0), irValue(0),
      lastBeat(0), rateSpot(0), delta(0), validReading(false), lastReading(0),
      avgHeartRate(0), stressLevel(0), isStressed(false) {
    for (int i = 0; i < 4; i++) rateArray[i] = 0;
}

bool MAX30102Sensor::begin() {
    if (!particleSensor.begin()) {
        Serial.println("MAX30102 not found. Check wiring.");
        return false;
    }
    particleSensor.setup();
    particleSensor.setPulseAmplitudeRed(0x0A);
    particleSensor.setPulseAmplitudeGreen(0);
    return true;
}

void MAX30102Sensor::update() {
    unsigned long now = millis();
    if (now - lastReading < READING_INTERVAL) return;
    lastReading = now;

    irValue = particleSensor.getIR();
    if (checkForBeat(irValue)) {
        isFingerDetected = true;
        delta = now - lastBeat;
        lastBeat = now;
        beatsPerMinute = 60 / (delta / 1000.0);
        if (beatsPerMinute < 255 && beatsPerMinute > 20) {
            rateArray[rateSpot++] = (long)beatsPerMinute;
            rateSpot %= 4;
            long total = 0;
            for (int i = 0; i < 4; i++) total += rateArray[i];
            heartRate = total / 4.0;
            validReading = true;
            updateStressLevel();
        }
    } else {
        isFingerDetected = (irValue > 50000);
        if (!isFingerDetected) validReading = false;
    }
}

void MAX30102Sensor::updateStressLevel() {
    if (heartRate > 100) stressLevel = min(5, stressLevel + 1);
    else if (heartRate < 60 || (heartRate >= 60 && heartRate <= 80)) stressLevel = max(0, stressLevel - 1);
    isStressed = (stressLevel >= 3);
    avgHeartRate = (avgHeartRate * 0.9f) + (heartRate * 0.1f);
}

bool MAX30102Sensor::isFingerOnSensor() { return isFingerDetected; }
float MAX30102Sensor::getHeartRate() { return heartRate; }
int MAX30102Sensor::getBPM() { return (int)heartRate; }
bool MAX30102Sensor::hasValidReading() { return validReading && isFingerDetected; }
int MAX30102Sensor::getStressLevel() { return stressLevel; }
bool MAX30102Sensor::isUserStressed() { return isStressed; }
String MAX30102Sensor::getStressDescription() {
    if (stressLevel == 0) return "Very Relaxed";
    if (stressLevel == 1) return "Relaxed";
    if (stressLevel == 2) return "Normal";
    if (stressLevel == 3) return "Slightly Stressed";
    if (stressLevel == 4) return "Stressed";
    return "Very Stressed";
}
String MAX30102Sensor::getWellnessRecommendation() {
    if (isStressed) return "You seem stressed! Try deep breathing for 1 minute.";
    if (heartRate > 90) return "Heart rate elevated. Consider a short break.";
    if (heartRate < 50) return "Very relaxed!";
    return "Heart rate normal.";
}
long MAX30102Sensor::getIRValue() { return irValue; }

