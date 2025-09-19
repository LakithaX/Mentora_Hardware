// Core
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Display + Eyes
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
#include <FluxGarage_RoboEyes.h>

// Sensors (modular)
#include "sensors/BH1750Sensor.h"
#include "sensors/DHT22Sensor.h"
#include "sensors/MAX30102Sensor.h"
#include "sensors/TiltSwitch.h"
#include "TTP223Touch.h"
#include "SensorFusion.h"

// WiFi
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Endpoints
const char* postUrl = "http://YOUR_APP_ENDPOINT/api/mentora"; // replace

// Pins per requirements
// I2C: SDA=21, SCL=22 (OLED, BH1750, MAX30102)
const int TOUCH2_PIN = 25;  // reactions
const int TOUCH3_PIN = 26;  // study toggle
const int DHT22_PIN = 4;    // DHT22 data
const int TILT_PIN = 27;    // tilt switch
const int SERVO_TILT_PIN = 18;
const int SERVO_PAN_PIN  = 19;
const int LED_PIN = 2;

// Display config
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Eyes
roboEyes roboEyes;

// Servos
Servo tiltServo;
Servo panServo;

// Web server
WebServer server(80);

// Sensors
BH1750Sensor lightSensor;
DHT22Sensor climateSensor(DHT22_PIN);
MAX30102Sensor heartSensor;
TiltSwitch tiltSensor(TILT_PIN);
TTP223Touch touchSensor(TOUCH2_PIN, TOUCH3_PIN);
SensorFusion fusion;

// Emotions
String currentEmotion = "DEFAULT";
String baseEmotion = "DEFAULT";
bool hasReaction = false;
bool isYesNoAnimation = false;
bool isReactionAnimation = false;
bool animationActive = false;
unsigned long animationStartTime = 0;
unsigned long lastAnimationUpdate = 0;
int yesNoStep = 0;
int reactionStep = 0;
unsigned long yesNoStepDuration = 250;
unsigned long reactionStepDuration = 150;
const unsigned long reactionAnimationDuration = 3000;

unsigned long lastPost = 0;
const unsigned long POST_INTERVAL = 2000; // 2 seconds

// Forward declarations
void initializeDisplay();
void initializeRoboEyes();
void setupWebServer();
void displayEmotion();
String parseBaseEmotion(String emotion);
bool isReactionEmotion(String emotion);
void setEmotion(String emotion);
void startYesNoAnimation(String type);
void startReactionAnimation(String base);
void updateAnimations();
void updateYesNoAnimation();
void updateReactionAnimation();
void endYesNoAnimation();
void endReactionAnimation();
void performHappyReaction();
void performAngryReaction();
void performTiredReaction();
void performDefaultReaction();
void nodYes();
void shakeNo();

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // I2C
  Wire.begin(21, 22);

  // Display + eyes
  initializeDisplay();
  initializeRoboEyes();

  // Servos
  tiltServo.attach(SERVO_TILT_PIN);
  panServo.attach(SERVO_PAN_PIN);
  tiltServo.write(90);
  panServo.write(90);

  // Sensors
  lightSensor.begin();
  climateSensor.begin();
  heartSensor.begin();
  tiltSensor.begin();
  touchSensor.begin();
  fusion.attachSensors(&lightSensor, &touchSensor, &heartSensor, &tiltSensor, &climateSensor);
  fusion.begin();

  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println();
  Serial.print("WiFi connected: "); Serial.println(WiFi.localIP());
  digitalWrite(LED_PIN, HIGH);

  // Web server
  setupWebServer();

  // Initial emotion
  displayEmotion();
}

void loop() {
  // Web requests
  server.handleClient();

  // Update sensors
  lightSensor.updateReading();
  climateSensor.updateReading();
  heartSensor.update();
  tiltSensor.update();
  touchSensor.update();
  fusion.update();

  // Sensor-driven emotions
  if (heartSensor.isUserStressed()) {
    if (currentEmotion != "TIRED" && !isYesNoAnimation && !isReactionAnimation) setEmotion("TIRED");
  } else if (lightSensor.isGoodForStudying()) {
    if (currentEmotion != "HAPPY" && !isYesNoAnimation && !isReactionAnimation) setEmotion("HAPPY");
  }

  // Eyes/animations
  if (!isYesNoAnimation && !isReactionAnimation) {
    roboEyes.update();
  }
  updateAnimations();

  // Prepare JSON
  String payload = fusion.getJSONData();

  // POST every 2s
  if (WiFi.status() == WL_CONNECTED && millis() - lastPost >= POST_INTERVAL) {
    lastPost = millis();
    HTTPClient http;
    http.begin(postUrl);
    http.addHeader("Content-Type", "application/json");
    int code = http.POST(payload);
    if (code > 0) {
      String resp = http.getString();
      Serial.print("POST "); Serial.print(code); Serial.print(" -> "); Serial.println(resp);
    } else {
      Serial.print("HTTP POST failed: "); Serial.println(code);
    }
    http.end();
  }

  delay(10);
}

// ===== Display & Eyes =====
void initializeDisplay() {
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("SSD1306 allocation failed");
    while(1) { delay(1000); }
  }
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 20);
  display.println("Mentora");
  display.display();
  delay(800);
}

void initializeRoboEyes() {
  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 60);
  roboEyes.setAutoblinker(ON, 3, 2);
  roboEyes.setIdleMode(ON, 2, 2);
}

// ===== Web Server =====
void setupWebServer() {
  server.enableCORS(true);

  server.on("/status", HTTP_GET, [](){
    DynamicJsonDocument doc(512);
    doc["emotion"] = currentEmotion;
    doc["base_emotion"] = baseEmotion;
    doc["has_reaction"] = hasReaction;
    doc["ip"] = WiFi.localIP().toString();
    doc["uptime"] = millis();
    String res; serializeJson(doc, res);
    server.send(200, "application/json", res);
  });

  server.on("/emotion", HTTP_POST, [](){
    if (!server.hasArg("plain")) { server.send(400, "application/json", "{\"error\":\"No JSON\"}"); return; }
    DynamicJsonDocument doc(256);
    if (deserializeJson(doc, server.arg("plain"))) { server.send(400, "application/json", "{\"error\":\"Bad JSON\"}"); return; }
    String e = doc["emotion"] | "DEFAULT"; e.toUpperCase();
    if (e == "YES" || e == "NO" || e.endsWith("_REACTION") || e == "HAPPY" || e == "ANGRY" || e == "TIRED" || e == "DEFAULT") {
      setEmotion(e);
      server.send(200, "application/json", "{\"ok\":true}");
    } else {
      server.send(400, "application/json", "{\"error\":\"Invalid emotion\"}");
    }
  });

  server.on("/move", HTTP_POST, [](){
    if (!server.hasArg("plain")) { server.send(400, "application/json", "{\"error\":\"No JSON\"}"); return; }
    DynamicJsonDocument doc(256);
    if (deserializeJson(doc, server.arg("plain"))) { server.send(400, "application/json", "{\"error\":\"Bad JSON\"}"); return; }
    int tilt = doc["tilt"] | 90;
    int pan  = doc["pan"]  | 90;
    tilt = constrain(tilt, 0, 180);
    pan  = constrain(pan , 0, 180);
    tiltServo.write(tilt);
    panServo.write(pan);
    server.send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/message", HTTP_POST, [](){
    // Accepts {"text":"..."} to show brief message on OLED
    if (!server.hasArg("plain")) { server.send(400, "application/json", "{\"error\":\"No JSON\"}"); return; }
    DynamicJsonDocument doc(256);
    if (deserializeJson(doc, server.arg("plain"))) { server.send(400, "application/json", "{\"error\":\"Bad JSON\"}"); return; }
    String text = doc["text"] | "";
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0,0);
    display.print(text);
    display.display();
    server.send(200, "application/json", "{\"ok\":true}");
  });

  server.onNotFound([](){ server.send(404, "application/json", "{\"error\":\"not found\"}"); });

  server.begin();
  Serial.println("Web server started");
}

// ===== Emotions =====
String parseBaseEmotion(String emotion) {
  if (emotion.endsWith("_REACTION")) return emotion.substring(0, emotion.indexOf("_REACTION"));
  return emotion;
}

bool isReactionEmotion(String emotion) { return emotion.endsWith("_REACTION"); }

void setEmotion(String emotion) {
  currentEmotion = emotion;
  baseEmotion = parseBaseEmotion(emotion);
  hasReaction = isReactionEmotion(emotion);

  if (emotion == "YES" || emotion == "NO") {
    startYesNoAnimation(emotion);
    if (emotion == "YES") nodYes(); else shakeNo();
  } else if (hasReaction) {
    startReactionAnimation(baseEmotion);
  } else {
    displayEmotion();
  }
}

void displayEmotion() {
  if (isYesNoAnimation || isReactionAnimation) return;
  roboEyes.setCyclops(ON);
  if (baseEmotion == "HAPPY") {
    roboEyes.setMood(HAPPY);
    roboEyes.setCuriosity(ON);
    roboEyes.setAutoblinker(ON, 3, 2);
    roboEyes.setIdleMode(ON, 2, 2);
  } else if (baseEmotion == "ANGRY") {
    roboEyes.setMood(ANGRY);
    roboEyes.setCuriosity(OFF);
    roboEyes.setHFlicker(ON, 1);
    roboEyes.setIdleMode(OFF);
  } else if (baseEmotion == "TIRED") {
    roboEyes.setMood(TIRED);
    roboEyes.setCuriosity(OFF);
    roboEyes.setVFlicker(ON, 1);
    roboEyes.setAutoblinker(ON, 2, 1);
    roboEyes.setIdleMode(ON, 4, 2);
  } else {
    roboEyes.setMood(DEFAULT);
    roboEyes.setCuriosity(ON);
    roboEyes.setAutoblinker(ON, 3, 2);
    roboEyes.setIdleMode(ON, 2, 2);
  }
}

void startYesNoAnimation(String type) {
  roboEyes.setAutoblinker(OFF, 0, 0);
  roboEyes.setIdleMode(OFF, 0, 0);
  roboEyes.setHFlicker(OFF, 0);
  roboEyes.setVFlicker(OFF, 0);
  roboEyes.setMood(DEFAULT);
  roboEyes.setPosition(DEFAULT);
  isYesNoAnimation = true; animationActive = true; animationStartTime = millis(); yesNoStep = 0;
  yesNoStepDuration = (type == "YES") ? 300 : 200;
  lastAnimationUpdate = millis();
}

void startReactionAnimation(String emotionType) {
  roboEyes.setAutoblinker(OFF, 0, 0);
  roboEyes.setIdleMode(OFF, 0, 0);
  roboEyes.setHFlicker(OFF, 0);
  roboEyes.setVFlicker(OFF, 0);
  if (emotionType == "HAPPY") roboEyes.setMood(HAPPY);
  else if (emotionType == "ANGRY") roboEyes.setMood(ANGRY);
  else if (emotionType == "TIRED") roboEyes.setMood(TIRED);
  else roboEyes.setMood(DEFAULT);
  roboEyes.setPosition(DEFAULT);
  isReactionAnimation = true; animationActive = true; animationStartTime = millis(); reactionStep = 0;
  lastAnimationUpdate = millis();
}

void updateAnimations() {
  unsigned long now = millis();
  if (!animationActive) return;
  if (isYesNoAnimation) { updateYesNoAnimation(); return; }
  if (isReactionAnimation) { updateReactionAnimation(); return; }
}

void updateYesNoAnimation() {
  unsigned long now = millis();
  if (now - lastAnimationUpdate < yesNoStepDuration) return;
  lastAnimationUpdate = now;

  if (currentEmotion == "YES") {
    switch (yesNoStep % 4) {
      case 0: roboEyes.setPosition(DEFAULT); break;
      case 1: roboEyes.setPosition(N); break;
      case 2: roboEyes.setPosition(DEFAULT); break;
      case 3: roboEyes.setPosition(S); break;
    }
    yesNoStep++;
    if (yesNoStep >= 12) endYesNoAnimation();
  } else {
    switch (yesNoStep % 4) {
      case 0: roboEyes.setPosition(DEFAULT); break;
      case 1: roboEyes.setPosition(W); break;
      case 2: roboEyes.setPosition(DEFAULT); break;
      case 3: roboEyes.setPosition(E); break;
    }
    yesNoStep++;
    if (yesNoStep >= 16) endYesNoAnimation();
  }
  roboEyes.drawEyes();
}

void updateReactionAnimation() {
  unsigned long now = millis();
  if (now - animationStartTime >= reactionAnimationDuration) { endReactionAnimation(); return; }
  if (now - lastAnimationUpdate < reactionStepDuration) return;
  lastAnimationUpdate = now;
  reactionStep++;
  if (baseEmotion == "HAPPY") performHappyReaction();
  else if (baseEmotion == "ANGRY") performAngryReaction();
  else if (baseEmotion == "TIRED") performTiredReaction();
  else performDefaultReaction();
  roboEyes.drawEyes();
}

void endYesNoAnimation() {
  isYesNoAnimation = false; animationActive = false; yesNoStep = 0;
  roboEyes.setPosition(DEFAULT);
  currentEmotion = "DEFAULT"; baseEmotion = "DEFAULT"; hasReaction = false;
  displayEmotion();
}

void endReactionAnimation() {
  isReactionAnimation = false; animationActive = false; reactionStep = 0;
  tiltServo.write(90); panServo.write(90);
  roboEyes.setPosition(DEFAULT);
  currentEmotion = baseEmotion; hasReaction = false;
  displayEmotion();
}

// Reactions (servo + eyes)
void performHappyReaction() {
  switch (reactionStep % 6) {
    case 0: roboEyes.setPosition(NE); tiltServo.write(120); panServo.write(60); break;
    case 1: roboEyes.setPosition(NW); tiltServo.write(60);  panServo.write(120); break;
    case 2: roboEyes.setPosition(SE); tiltServo.write(110); panServo.write(70); break;
    case 3: roboEyes.setPosition(SW); tiltServo.write(70);  panServo.write(110); break;
    case 4: roboEyes.setPosition(N);  roboEyes.blink(); tiltServo.write(100); panServo.write(90); break;
    case 5: roboEyes.setPosition(DEFAULT); roboEyes.anim_laugh(); tiltServo.write(90); panServo.write(90); break;
  }
}

void performAngryReaction() {
  switch (reactionStep % 5) {
    case 0: roboEyes.setPosition(W); tiltServo.write(70);  panServo.write(50);  break;
    case 1: roboEyes.setPosition(E); tiltServo.write(70);  panServo.write(130); break;
    case 2: roboEyes.setPosition(N); tiltServo.write(120); panServo.write(90);  break;
    case 3: roboEyes.setPosition(DEFAULT); roboEyes.anim_confused(); tiltServo.write(80); panServo.write(90); break;
    case 4: roboEyes.blink(); tiltServo.write(90); panServo.write(90); break;
  }
}

void performTiredReaction() {
  switch (reactionStep % 4) {
    case 0: roboEyes.setPosition(S);  tiltServo.write(60); panServo.write(90);  break;
    case 1: roboEyes.setPosition(SW); tiltServo.write(60); panServo.write(110); break;
    case 2: roboEyes.setPosition(SE); tiltServo.write(60); panServo.write(70);  break;
    case 3: roboEyes.setPosition(DEFAULT); roboEyes.blink(); tiltServo.write(90); panServo.write(90); break;
  }
}

void performDefaultReaction() {
  switch (reactionStep % 8) {
    case 0: roboEyes.setPosition(N);  tiltServo.write(120); panServo.write(80);  break;
    case 1: roboEyes.setPosition(NE); tiltServo.write(130); panServo.write(90);  break;
    case 2: roboEyes.setPosition(E);  tiltServo.write(110); panServo.write(60);  break;
    case 3: roboEyes.setPosition(SE); tiltServo.write(100); panServo.write(70);  break;
    case 4: roboEyes.setPosition(S);  tiltServo.write(140); panServo.write(85);  break;
    case 5: roboEyes.setPosition(SW); tiltServo.write(120); panServo.write(100); break;
    case 6: roboEyes.setPosition(W);  tiltServo.write(100); panServo.write(120); break;
    case 7: roboEyes.setPosition(DEFAULT); roboEyes.blink(); tiltServo.write(90); panServo.write(90); break;
  }
}

void nodYes() {
  for (int i = 0; i < 3; i++) { tiltServo.write(120); delay(120); tiltServo.write(60); delay(120); }
  tiltServo.write(90);
}

void shakeNo() {
  for (int i = 0; i < 4; i++) { panServo.write(60); delay(120); panServo.write(120); delay(120); }
  panServo.write(90);
}

