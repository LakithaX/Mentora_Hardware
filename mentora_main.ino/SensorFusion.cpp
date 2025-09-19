#include "SensorFusion.h"

SensorFusion::SensorFusion() : light(nullptr), climate(nullptr), touch(nullptr), heart(nullptr), tilt(nullptr), currentActivity("idle"), studyStart(0), studyMode(false) {}

void SensorFusion::attachSensors(BH1750Sensor* l, TTP223Touch* t, MAX30102Sensor* h, TiltSwitch* ts, DHT22Sensor* c) {
    light = l; touch = t; heart = h; tilt = ts; climate = c;
}

void SensorFusion::begin() {
    studyStart = millis();
}

void SensorFusion::update() {
    // Update study mode from touch pattern
    if (touch) {
        String pattern = touch->getTouchPattern();
        if (pattern == "TOUCH2") {
            studyMode = !studyMode;
            currentActivity = studyMode ? "studying" : "idle";
        }
    }
}

String SensorFusion::getJSONData() {
    DynamicJsonDocument doc(1024);
    if (light) {
        doc["light"]["lux"] = light->getLux();
        doc["light"]["level"] = light->getLightLevel();
        doc["light"]["goodForStudy"] = light->isGoodForStudying();
    }
    if (climate) {
        doc["climate"]["tempC"] = climate->getTemperature();
        doc["climate"]["humidity"] = climate->getHumidity();
        doc["climate"]["heatIndexC"] = climate->getHeatIndex();
        doc["climate"]["comfortable"] = climate->isEnvironmentComfortable();
        doc["climate"]["recommendation"] = climate->getComfortRecommendation();
    }
    if (touch) {
        doc["touch"]["pattern"] = touch->getTouchPattern();
        doc["touch"]["response"] = touch->getTouchResponse();
    }
    if (heart) {
        doc["heart"]["bpm"] = heart->getBPM();
        doc["heart"]["valid"] = heart->hasValidReading();
        doc["heart"]["stressLevel"] = heart->getStressLevel();
        doc["heart"]["stressed"] = heart->isUserStressed();
    }
    if (tilt) {
        doc["tilt"]["tilted"] = tilt->isCurrentlyTilted();
        doc["tilt"]["lifted"] = tilt->isCurrentlyLifted();
        doc["tilt"]["humor"] = tilt->getContextualResponse(currentActivity);
    }
    doc["activity"] = currentActivity;
    doc["recommendation"] = getSmartRecommendation();
    String out;
    serializeJson(doc, out);
    return out;
}

String SensorFusion::getSmartRecommendation() {
    String rec = "";
    if (light && !light->isGoodForStudying()) rec += light->getLightRecommendation() + " ";
    if (climate) rec += climate->getComfortRecommendation() + " ";
    if (heart) rec += heart->getWellnessRecommendation() + " ";
    return rec;
}

String SensorFusion::getEmotionalResponse() {
    if (heart && heart->isUserStressed()) return "Concerned";
    if (light && light->isGoodForStudying()) return "Encouraging";
    return "Neutral";
}

String SensorFusion::analyzeStudyEnvironment() {
    String env = "";
    if (light) env += light->getLightRecommendation();
    return env;
}

String SensorFusion::detectInteractionPattern() {
    if (touch) return touch->getTouchPattern();
    return "";
}

void SensorFusion::setCurrentActivity(const String& activity) { currentActivity = activity; }
String SensorFusion::getCurrentActivity() { return currentActivity; }

StudyMetrics SensorFusion::getStudyMetrics() {
    StudyMetrics m{};
    m.isActivelyStudying = (currentActivity == "studying");
    m.focusMode = m.isActivelyStudying ? "Deep" : "Idle";
    m.attentionLevel = calculateFocusScore();
    m.needsBreak = (heart && heart->isUserStressed());
    m.totalStudyTime = studyMode ? (millis() - studyStart) : 0;
    m.recommendation = getSmartRecommendation();
    return m;
}

int SensorFusion::calculateFocusScore() {
    int score = 50;
    if (light && light->isGoodForStudying()) score += 20;
    if (heart && !heart->isUserStressed()) score += 20;
    if (currentActivity == "studying") score += 10;
    return constrain(score, 0, 100);
}

