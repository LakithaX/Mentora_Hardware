#ifndef MENTORA_BH1750_SENSOR_H
#define MENTORA_BH1750_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>

class BH1750Sensor {
private:
    BH1750 lightMeter;
    float currentLux;
    unsigned long lastReading;
    const unsigned long READING_INTERVAL = 1000;

public:
    BH1750Sensor();
    bool begin();
    bool updateReading();
    float getLux();
    String getLightLevel();
    bool isGoodForStudying();
    bool isDarkEnvironment();
    bool isBrightEnvironment();
    String getLightRecommendation();
};

#endif

