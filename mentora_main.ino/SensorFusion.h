#ifndef MENTORA_SENSOR_FUSION_H
#define MENTORA_SENSOR_FUSION_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "sensors/BH1750Sensor.h"
#include "sensors/DHT22Sensor.h"
#include "TTP223Touch.h"
#include "sensors/MAX30102Sensor.h"
#include "sensors/TiltSwitch.h"

struct StudyMetrics {
    bool isActivelyStudying;
    String focusMode;
    int attentionLevel; // 0-100
    bool needsBreak;
    unsigned long totalStudyTime; // ms
    String recommendation;
};

class SensorFusion {
private:
    BH1750Sensor* light;
    DHT22Sensor* climate;
    TTP223Touch* touch;
    MAX30102Sensor* heart;
    TiltSwitch* tilt;
    String currentActivity;
    unsigned long studyStart;
    bool studyMode;

public:
    SensorFusion();
    void attachSensors(BH1750Sensor* l, TTP223Touch* t, MAX30102Sensor* h, TiltSwitch* ts, DHT22Sensor* c = nullptr);
    void begin();
    void update();
    String getJSONData();
    String getSmartRecommendation();
    String getEmotionalResponse();
    String analyzeStudyEnvironment();
    String detectInteractionPattern();
    void setCurrentActivity(const String& activity);
    String getCurrentActivity();
    StudyMetrics getStudyMetrics();
    int calculateFocusScore();
};

#endif

