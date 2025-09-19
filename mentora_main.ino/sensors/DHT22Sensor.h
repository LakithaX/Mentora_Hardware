#ifndef MENTORA_DHT22_SENSOR_H
#define MENTORA_DHT22_SENSOR_H

#include <Arduino.h>
#include <DHT.h>

class DHT22Sensor {
private:
    DHT dht;
    int pin;
    float temperature;
    float humidity;
    float heatIndex;
    unsigned long lastReading;
    const unsigned long READING_INTERVAL = 2000;

    const float TEMP_MIN_COMFORT = 20.0f;
    const float TEMP_MAX_COMFORT = 26.0f;
    const float HUMIDITY_MIN_COMFORT = 30.0f;
    const float HUMIDITY_MAX_COMFORT = 60.0f;

    bool validReading;

public:
    explicit DHT22Sensor(int dataPin);
    bool begin();
    bool updateReading();
    float getTemperature();
    float getHumidity();
    float getHeatIndex();
    bool hasValidReading();

    bool isTemperatureComfortable();
    bool isHumidityComfortable();
    bool isEnvironmentComfortable();
    String getTemperatureStatus();
    String getHumidityStatus();
    String getComfortRecommendation();

    String getStudyEnvironmentAnalysis();
    bool isTooHotToFocus();
    bool isTooColdToFocus();
    bool isTooHumid();
    bool isTooDry();
};

#endif

