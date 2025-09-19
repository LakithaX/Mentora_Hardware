#ifndef MENTORA_MAX30102_SENSOR_H
#define MENTORA_MAX30102_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"

class MAX30102Sensor {
private:
    MAX30105 particleSensor;
    bool isFingerDetected;
    float heartRate;
    int beatsPerMinute;
    long irValue;
    unsigned long lastBeat;
    int rateArray[4];
    int rateSpot;
    long delta;
    bool validReading;
    unsigned long lastReading;
    const unsigned long READING_INTERVAL = 100;

    float avgHeartRate;
    int stressLevel;
    bool isStressed;

    void updateStressLevel();

public:
    MAX30102Sensor();
    bool begin();
    void update();
    bool isFingerOnSensor();
    float getHeartRate();
    int getBPM();
    bool hasValidReading();
    int getStressLevel();
    bool isUserStressed();
    String getStressDescription();
    String getWellnessRecommendation();
    long getIRValue();
};

#endif

