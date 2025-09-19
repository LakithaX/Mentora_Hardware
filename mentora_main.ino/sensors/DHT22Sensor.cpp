#include "DHT22Sensor.h"

DHT22Sensor::DHT22Sensor(int dataPin) : dht(dataPin, DHT22), pin(dataPin), temperature(0), humidity(0), heatIndex(0), lastReading(0), validReading(false) {}

bool DHT22Sensor::begin() {
    dht.begin();
    delay(2000);
    return updateReading();
}

bool DHT22Sensor::updateReading() {
    unsigned long now = millis();
    if (now - lastReading < READING_INTERVAL) return false;
    lastReading = now;

    float newHumidity = dht.readHumidity();
    float newTemperature = dht.readTemperature();
    if (isnan(newHumidity) || isnan(newTemperature)) {
        validReading = false;
        return false;
    }
    humidity = newHumidity;
    temperature = newTemperature;
    heatIndex = dht.computeHeatIndex(temperature, humidity, false);
    validReading = true;
    return true;
}

float DHT22Sensor::getTemperature() { return temperature; }
float DHT22Sensor::getHumidity() { return humidity; }
float DHT22Sensor::getHeatIndex() { return heatIndex; }
bool DHT22Sensor::hasValidReading() { return validReading; }

bool DHT22Sensor::isTemperatureComfortable() { return temperature >= TEMP_MIN_COMFORT && temperature <= TEMP_MAX_COMFORT; }
bool DHT22Sensor::isHumidityComfortable() { return humidity >= HUMIDITY_MIN_COMFORT && humidity <= HUMIDITY_MAX_COMFORT; }
bool DHT22Sensor::isEnvironmentComfortable() { return isTemperatureComfortable() && isHumidityComfortable(); }

String DHT22Sensor::getTemperatureStatus() {
    if (temperature < 18) return "Very Cold";
    if (temperature < 20) return "Cold";
    if (temperature < 22) return "Cool";
    if (temperature <= 26) return "Comfortable";
    if (temperature < 28) return "Warm";
    if (temperature < 30) return "Hot";
    return "Very Hot";
}

String DHT22Sensor::getHumidityStatus() {
    if (humidity < 20) return "Very Dry";
    if (humidity < 30) return "Dry";
    if (humidity <= 60) return "Comfortable";
    if (humidity < 70) return "Humid";
    if (humidity < 80) return "Very Humid";
    return "Extremely Humid";
}

String DHT22Sensor::getComfortRecommendation() {
    if (isTooHotToFocus()) return "Too hot for optimal studying! Try cooling the room.";
    if (isTooColdToFocus()) return "Too cold! Consider warming up the room.";
    if (isTooHumid()) return "Very humid! Improve ventilation.";
    if (isTooDry()) return "Air is too dry. A humidifier could help.";
    if (isEnvironmentComfortable()) return "Perfect temperature and humidity for studying!";
    return "Environment is okay but could be optimized.";
}

String DHT22Sensor::getStudyEnvironmentAnalysis() {
    String analysis = "Climate Analysis:\n";
    analysis += "Temperature: " + String(temperature) + "°C (" + getTemperatureStatus() + ")\n";
    analysis += "Humidity: " + String(humidity) + "% (" + getHumidityStatus() + ")\n";
    analysis += "Heat Index: " + String(heatIndex) + "°C\n";
    analysis += "Study Comfort: " + String(isEnvironmentComfortable() ? "Optimal" : "Needs Adjustment");
    return analysis;
}

bool DHT22Sensor::isTooHotToFocus() { return temperature > 28.0; }
bool DHT22Sensor::isTooColdToFocus() { return temperature < 18.0; }
bool DHT22Sensor::isTooHumid() { return humidity > 70.0; }
bool DHT22Sensor::isTooDry() { return humidity < 25.0; }

