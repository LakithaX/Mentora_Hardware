//***********************************************************************************************
//  ESP32 Robo Eyes with Web Server Control - Enhanced with Emotion Reactions + YES/NO
//  Controls robotic eyes emotions + emotion reactions + YES/NO responses via HTTP requests
//  Now supports separate basic emotions and emotion reactions for Flutter app integration
//
//  Hardware: ESP32 dev board, I2C OLED display (SSD1306/SSD1309), Servo motors
//  ESP32 I2C Pins: SDA = GPIO 21, SCL = GPIO 22 (default)
//  Servo Pins: Define TILT_PIN and PAN_PIN below
//
//  API Endpoints:
//  GET /status - Returns current emotion and connection status
//  POST /emotion - Set emotion (body: {"emotion": "EMOTION_NAME"})
//  GET / - Enhanced web interface for testing
//
//  Supported Emotions:
//  Basic: HAPPY, ANGRY, TIRED, DEFAULT, YES, NO
//  With Reactions: HAPPY_REACTION, ANGRY_REACTION, TIRED_REACTION, DEFAULT_REACTION
//
//  Note: Make sure FluxGarage_RoboEyes library is properly installed
//
//***********************************************************************************************

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>

// WiFi credentials - Change these to your network
const char* ssid = "kkk";
const char* password = "lakitha4";

// Servo pin definitions - ADD YOUR ACTUAL PIN NUMBERS HERE
#define TILT_PIN 18  // Change to your tilt servo pin
#define PAN_PIN 19   // Change to your pan servo pin

// Define servo objects
Servo tiltServo;
Servo panServo;

// OLED display configuration
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// Create display object - must be global for FluxGarage library
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Include FluxGarage library after display declaration
#include <FluxGarage_RoboEyes.h>

// Create RoboEyes instance
roboEyes roboEyes;

// Web server on port 80
WebServer server(80);

// Current emotion state
String currentEmotion = "DEFAULT";
String baseEmotion = "DEFAULT"; // The underlying emotion without reaction
bool hasReaction = false;
unsigned long lastCommandTime = 0;
bool wifiConnected = false;
unsigned long lastEyeUpdate = 0;
int eyeState = 0;
bool eyeOpen = true;

// Animation variables
unsigned long lastAnimationUpdate = 0;
const unsigned long animationInterval = 100;
int animationStep = 0;
bool animationActive = false;
unsigned long animationStartTime = 0;
const unsigned long animationDuration = 3000; // Duration for reaction animations

// YES/NO specific animation variables
bool isYesNoAnimation = false;
int yesNoStep = 0;
const int yesSteps = 6; // Number of nod cycles
const int noSteps = 8; // Number of shake cycles
unsigned long yesNoStepDuration = 300; // Duration per step in ms

// Reaction animation variables
bool isReactionAnimation = false;
int reactionStep = 0;
const unsigned long reactionAnimationDuration = 3000; // 3 seconds for reactions
unsigned long reactionStepDuration = 150; // Duration per reaction step

// Function declarations
void initializeDisplay();
void initializeRoboEyes();
void connectToWiFi();
void checkWiFiConnection();
void setupWebServer();
void setEmotion(String emotion);
void displayEmotion();
void startYesNoAnimation(String type);
void startReactionAnimation(String emotionType);
void startTransitionAnimation();
void updateAnimations();
void updateYesNoAnimation();
void updateReactionAnimation();
void endYesNoAnimation();
void endReactionAnimation();
void displayConnectionLost();
void nodYes();
void shakeNo();
void performHappyReaction();
void performAngryReaction();
void performTiredReaction();
void performDefaultReaction();
String parseBaseEmotion(String emotion);
bool isReactionEmotion(String emotion);

void setup() {
  Serial.begin(115200);
  Serial.println("Starting ESP32 Robo Eyes Web Server with Emotion Reactions...");
  
  // Initialize servos first
  tiltServo.attach(TILT_PIN);
  panServo.attach(PAN_PIN);
  
  // Set servos to center position
  tiltServo.write(90);
  panServo.write(90);
  delay(500);
  
  // Initialize I2C and OLED
  initializeDisplay();
  
  // Initialize RoboEyes library
  initializeRoboEyes();
  
  // Connect to WiFi
  connectToWiFi();
  
  // Setup web server routes
  setupWebServer();
  
  Serial.println("Setup complete!");
  displayEmotion();
}

void loop() {
  // Handle web server requests
  server.handleClient();
  
  // Update RoboEyes animations (only when not doing special animations)
  if (!isYesNoAnimation && !isReactionAnimation) {
    roboEyes.update();
  }
  
  // Update custom animations
  updateAnimations();
  
  // Check WiFi connection
  checkWiFiConnection();
  
  delay(10); // Small delay for stability
}

void initializeDisplay() {
  Wire.begin(21, 22); // ESP32 I2C pins (SDA, SCL)
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("SSD1306 allocation failed");
    while(1); // Stop execution
  }
  
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 20);
  display.println("ROBO");
  display.setCursor(20, 40);
  display.println("EYES");
  display.display();
  delay(2000);
  
  Serial.println("OLED display initialized");
}

void initializeRoboEyes() {
  // Initialize robo eyes - must be called after display is initialized
  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 60); // screen-width, screen-height, max framerate
  
  // Configure automated behaviors (will be controlled manually for special animations)
  roboEyes.setAutoblinker(ON, 3, 2); // Start auto blinker animation cycle
  roboEyes.setIdleMode(ON, 2, 2); // Start idle animation cycle
  
  Serial.println("FluxGarage RoboEyes library initialized");
}

void connectToWiFi() {
  // Show connecting message on display
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Connecting to WiFi...");
  display.display();
  
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(1000);
    Serial.print(".");
    attempts++;
    
    // Show progress dots
    display.print(".");
    display.display();
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println();
    Serial.print("Connected! IP address: ");
    Serial.println(WiFi.localIP());
    
    // Show IP address
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("WiFi Connected!");
    display.setCursor(0, 15);
    display.println("IP Address:");
    display.setCursor(0, 25);
    display.println(WiFi.localIP().toString());
    display.setCursor(0, 45);
    display.println("Ready for commands!");
    display.display();
    delay(3000); // Show IP for 3 seconds
    
  } else {
    wifiConnected = false;
    Serial.println();
    Serial.println("WiFi connection failed!");
    
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("WiFi Failed!");
    display.setCursor(0, 15);
    display.println("Check credentials");
    display.setCursor(0, 30);
    display.println("in code and");
    display.setCursor(0, 40);
    display.println("restart ESP32");
    display.display();
    delay(5000);
  }
}

void checkWiFiConnection() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 10000) { // Check every 10 seconds
    lastCheck = millis();
    
    if (WiFi.status() != WL_CONNECTED && wifiConnected) {
      wifiConnected = false;
      Serial.println("WiFi connection lost!");
      displayConnectionLost();
    } else if (WiFi.status() == WL_CONNECTED && !wifiConnected) {
      wifiConnected = true;
      Serial.println("WiFi reconnected!");
    }
  }
}

void setupWebServer() {
  // Enable CORS for all responses
  server.enableCORS(true);
  
  // GET /status - Return current status
  server.on("/status", HTTP_GET, []() {
    JsonDocument doc;
    doc["emotion"] = currentEmotion;
    doc["base_emotion"] = baseEmotion;
    doc["has_reaction"] = hasReaction;
    doc["wifi_connected"] = wifiConnected;
    doc["ip_address"] = WiFi.localIP().toString();
    doc["uptime"] = millis();
    doc["last_command"] = lastCommandTime;
    doc["signal_strength"] = WiFi.RSSI();
    doc["animation_active"] = animationActive;
    doc["yes_no_active"] = isYesNoAnimation;
    doc["reaction_active"] = isReactionAnimation;
    
    String response;
    serializeJson(doc, response);
    
    server.sendHeader("Content-Type", "application/json");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", response);
    
    Serial.println("Status requested - Current emotion: " + currentEmotion);
  });
  
  // POST /emotion - Set new emotion (now includes reactions)
  server.on("/emotion", HTTP_POST, []() {
    if (!server.hasArg("plain")) {
      server.send(400, "application/json", "{\"error\":\"No JSON body provided\"}");
      return;
    }
    
    String body = server.arg("plain");
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
      server.send(400, "application/json", "{\"error\":\"Invalid JSON format\"}");
      Serial.println("Invalid JSON received: " + body);
      return;
    }
    
    if (!doc.containsKey("emotion")) {
      server.send(400, "application/json", "{\"error\":\"Missing 'emotion' field in JSON\"}");
      return;
    }
    
    String newEmotion = doc["emotion"];
    newEmotion.toUpperCase();
    
    // Enhanced validation to include reaction emotions
    if (newEmotion == "HAPPY" || newEmotion == "ANGRY" || 
        newEmotion == "TIRED" || newEmotion == "DEFAULT" ||
        newEmotion == "HAPPY_REACTION" || newEmotion == "ANGRY_REACTION" || 
        newEmotion == "TIRED_REACTION" || newEmotion == "DEFAULT_REACTION" ||
        newEmotion == "YES" || newEmotion == "NO") {
      
      setEmotion(newEmotion);
      lastCommandTime = millis();
      
      JsonDocument response;
      response["success"] = true;
      response["emotion"] = currentEmotion;
      response["base_emotion"] = baseEmotion;
      response["has_reaction"] = hasReaction;
      response["message"] = "Emotion set successfully";
      response["timestamp"] = millis();
      
      String responseStr;
      serializeJson(response, responseStr);
      
      server.sendHeader("Content-Type", "application/json");
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "application/json", responseStr);
      
      Serial.println("Emotion changed to: " + currentEmotion);
      
    } else {
      server.send(400, "application/json", 
                 "{\"error\":\"Invalid emotion. Use: HAPPY, ANGRY, TIRED, DEFAULT, HAPPY_REACTION, ANGRY_REACTION, TIRED_REACTION, DEFAULT_REACTION, YES, or NO\"}");
      Serial.println("Invalid emotion received: " + newEmotion);
    }
  });
  
  // Handle preflight requests for CORS
  server.on("/emotion", HTTP_OPTIONS, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    server.send(200);
  });
  
  // GET / - Enhanced web interface with reaction buttons
  server.on("/", HTTP_GET, []() {
    String html = "<!DOCTYPE html><html><head><title>Robo Eyes Control</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>body{font-family:Arial;margin:20px;text-align:center;background:#f5f5f5;}";
    html += "button{padding:12px 20px;margin:8px;font-size:16px;border:none;border-radius:8px;cursor:pointer;transition:all 0.2s;}";
    html += "button:hover{transform:scale(1.05);box-shadow:0 4px 8px rgba(0,0,0,0.2);} button:active{transform:scale(0.95);}";
    html += ".happy{background-color:#4CAF50;color:white;} .angry{background-color:#f44336;color:white;}";
    html += ".tired{background-color:#ff9800;color:white;} .default{background-color:#2196F3;color:white;}";
    html += ".reaction{background:linear-gradient(45deg, #FFD700, #FFA500);color:#333;font-weight:bold;border:2px solid #FFD700;}";
    html += ".yes{background-color:#00C853;color:white;font-weight:bold;} .no{background-color:#D50000;color:white;font-weight:bold;}";
    html += ".status{background-color:white;padding:15px;margin:15px 0;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1);}";
    html += ".section{margin:25px 0;} .section h3{color:#333;margin-bottom:10px;}";
    html += ".response{background-color:#e8f5e8;padding:10px;border-radius:5px;margin:10px 0;display:none;}";
    html += ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:10px;max-width:800px;margin:0 auto;}";
    html += "</style></head><body>";
    html += "<h1>🤖 Advanced Robo Eyes Controller</h1>";
    html += "<div class='status'>";
    html += "<p><strong>Current State:</strong> " + currentEmotion + "</p>";
    html += "<p><strong>Base Emotion:</strong> " + baseEmotion + "</p>";
    html += "<p><strong>Has Reaction:</strong> " + String(hasReaction ? "Yes" : "No") + "</p>";
    html += "<p><strong>WiFi Status:</strong> " + String(wifiConnected ? "Connected" : "Disconnected") + "</p>";
    html += "<p><strong>IP Address:</strong> " + WiFi.localIP().toString() + "</p>";
    html += "<p><strong>Animation Active:</strong> " + String(animationActive ? "Yes" : "No") + "</p>";
    html += "</div>";
    
    html += "<div class='section'>";
    html += "<h3>🎭 Basic Emotions</h3>";
    html += "<div class='grid'>";
    html += "<button class='happy' onclick=\"setEmotion('HAPPY')\">😊 HAPPY</button>";
    html += "<button class='angry' onclick=\"setEmotion('ANGRY')\">😠 ANGRY</button>";
    html += "<button class='tired' onclick=\"setEmotion('TIRED')\">😴 TIRED</button>";
    html += "<button class='default' onclick=\"setEmotion('DEFAULT')\">😐 DEFAULT</button>";
    html += "</div></div>";
    
    html += "<div class='section'>";
    html += "<h3>✨ Emotions with Reactions</h3>";
    html += "<div class='grid'>";
    html += "<button class='happy reaction' onclick=\"setEmotion('HAPPY_REACTION')\">🎉 HAPPY + REACTION</button>";
    html += "<button class='angry reaction' onclick=\"setEmotion('ANGRY_REACTION')\">💢 ANGRY + REACTION</button>";
    html += "<button class='tired reaction' onclick=\"setEmotion('TIRED_REACTION')\">😪 TIRED + REACTION</button>";
    html += "<button class='default reaction' onclick=\"setEmotion('DEFAULT_REACTION')\">🔄 DEFAULT + REACTION</button>";
    html += "</div></div>";
    
    html += "<div class='section'>";
    html += "<h3>✅ YES/NO Responses</h3>";
    html += "<div class='grid'>";
    html += "<button class='yes' onclick=\"setEmotion('YES')\">✅ YES (Nod)</button>";
    html += "<button class='no' onclick=\"setEmotion('NO')\">❌ NO (Shake)</button>";
    html += "</div></div>";
    
    html += "<div id='response' class='response'></div>";
    
    html += "<script>";
    html += "function setEmotion(emotion) {";
    html += "  const responseDiv = document.getElementById('response');";
    html += "  responseDiv.style.display = 'block';";
    html += "  responseDiv.innerHTML = 'Sending ' + emotion + ' command...';";
    html += "  responseDiv.style.backgroundColor = '#fff3cd';";
    html += "  ";
    html += "  fetch('/emotion', {";
    html += "    method: 'POST',";
    html += "    headers: {'Content-Type': 'application/json'},";
    html += "    body: JSON.stringify({emotion: emotion})";
    html += "  }).then(response => response.json())";
    html += "  .then(data => {";
    html += "    if(data.success) {";
    html += "      responseDiv.innerHTML = '✅ ' + emotion + ' command sent successfully!';";
    html += "      responseDiv.style.backgroundColor = '#e8f5e8';";
    html += "      if(emotion.includes('REACTION') || emotion === 'YES' || emotion === 'NO') {";
    html += "        responseDiv.innerHTML += '<br>Animation will play for 3 seconds';";
    html += "        setTimeout(() => location.reload(), 3500);";
    html += "      } else {";
    html += "        setTimeout(() => location.reload(), 1000);";
    html += "      }";
    html += "    } else {";
    html += "      responseDiv.innerHTML = '❌ Error: ' + (data.error || 'Unknown error');";
    html += "      responseDiv.style.backgroundColor = '#f8d7da';";
    html += "    }";
    html += "  }).catch(err => {";
    html += "    responseDiv.innerHTML = '❌ Connection error: ' + err;";
    html += "    responseDiv.style.backgroundColor = '#f8d7da';";
    html += "  });";
    html += "}";
    html += "setInterval(() => {";
    html += "  if(!document.getElementById('response').innerHTML.includes('Sending')) {";
    html += "    location.reload();";
    html += "  }";
    html += "}, 20000);"; // Auto-refresh every 20 seconds when not animating
    html += "</script></body></html>";
    
    server.send(200, "text/html", html);
  });
  
  // 404 handler
  server.onNotFound([]() {
    server.send(404, "application/json", "{\"error\":\"Endpoint not found\"}");
  });
  
  server.begin();
  Serial.println("Web server started on port 80");
  Serial.println("Access web interface at: http://" + WiFi.localIP().toString());
}

String parseBaseEmotion(String emotion) {
  if (emotion.endsWith("_REACTION")) {
    return emotion.substring(0, emotion.indexOf("_REACTION"));
  }
  return emotion;
}

bool isReactionEmotion(String emotion) {
  return emotion.endsWith("_REACTION");
}

void setEmotion(String emotion) {
  currentEmotion = emotion;
  baseEmotion = parseBaseEmotion(emotion);
  hasReaction = isReactionEmotion(emotion);
  
  Serial.println("Setting emotion to: " + emotion);
  Serial.println("Base emotion: " + baseEmotion + ", Has reaction: " + String(hasReaction));
  
  // Check if this is a YES/NO animation
  if (emotion == "YES" || emotion == "NO") {
    startYesNoAnimation(emotion);
    // Trigger physical servo movement
    if (emotion == "YES") {
      nodYes();
    } else {
      shakeNo();
    }
  } else if (hasReaction) {
    // This is an emotion with reaction
    startReactionAnimation(baseEmotion);
  } else {
    // Regular emotion - start transition animation
    startTransitionAnimation();
  }
  
  displayEmotion();
}

void displayEmotion() {
  // Don't set library states during special animations
  if (isYesNoAnimation || isReactionAnimation) {
    return;
  }
  
  roboEyes.setCyclops(ON);
  
  // Set emotion using RoboEyes library based on base emotion
  if (baseEmotion == "HAPPY") {
    roboEyes.setMood(HAPPY);
    roboEyes.setCuriosity(ON);
    roboEyes.setHFlicker(OFF, 0);
    roboEyes.setVFlicker(OFF, 0);
    roboEyes.setAutoblinker(ON, 3, 2);
    roboEyes.setIdleMode(ON, 2, 2);
    
  } else if (baseEmotion == "ANGRY") {
    roboEyes.setMood(ANGRY);
    roboEyes.setCuriosity(OFF);
    roboEyes.setHFlicker(ON, 1);
    roboEyes.setVFlicker(OFF, 0);
    roboEyes.setAutoblinker(ON, 4, 1);
    roboEyes.setIdleMode(OFF);
    
  } else if (baseEmotion == "TIRED") {
    roboEyes.setMood(TIRED);
    roboEyes.setCuriosity(OFF);
    roboEyes.setHFlicker(OFF, 0);
    roboEyes.setVFlicker(ON, 1);
    roboEyes.setAutoblinker(ON, 2, 1);
    roboEyes.setIdleMode(ON, 4, 2);
    
  } else { // DEFAULT (and after special animations)
    roboEyes.setMood(DEFAULT);
    roboEyes.setCuriosity(ON);
    roboEyes.setHFlicker(OFF, 0);
    roboEyes.setVFlicker(OFF, 0);
    roboEyes.setAutoblinker(ON, 3, 2);
    roboEyes.setIdleMode(ON, 2, 2);
  }
}

void startYesNoAnimation(String type) {
  Serial.println("Starting " + type + " animation");
  
  // Disable library auto-animations during YES/NO
  roboEyes.setAutoblinker(OFF, 0, 0);
  roboEyes.setIdleMode(OFF, 0, 0);
  roboEyes.setHFlicker(OFF, 0);
  roboEyes.setVFlicker(OFF, 0);
  
  // Set default mood and position
  roboEyes.setMood(DEFAULT);
  roboEyes.setPosition(DEFAULT);
  
  // Initialize YES/NO animation variables
  isYesNoAnimation = true;
  animationActive = true;
  animationStartTime = millis();
  yesNoStep = 0;
  
  // Set step duration based on type
  if (type == "YES") {
    yesNoStepDuration = 300; // Slower nods
  } else {
    yesNoStepDuration = 200; // Faster shakes
  }
  
  lastAnimationUpdate = millis();
}

void startReactionAnimation(String emotionType) {
  Serial.println("Starting " + emotionType + " reaction animation");
  
  // Disable library auto-animations during reaction
  roboEyes.setAutoblinker(OFF, 0, 0);
  roboEyes.setIdleMode(OFF, 0, 0);
  roboEyes.setHFlicker(OFF, 0);
  roboEyes.setVFlicker(OFF, 0);
  
  // Set the base emotion first
  if (emotionType == "HAPPY") {
    roboEyes.setMood(HAPPY);
  } else if (emotionType == "ANGRY") {
    roboEyes.setMood(ANGRY);
  } else if (emotionType == "TIRED") {
    roboEyes.setMood(TIRED);
  } else {
    roboEyes.setMood(DEFAULT);
  }
  
  roboEyes.setPosition(DEFAULT);
  
  // Initialize reaction animation variables
  isReactionAnimation = true;
  animationActive = true;
  animationStartTime = millis();
  reactionStep = 0;
  lastAnimationUpdate = millis();
}

void startTransitionAnimation() {
  isYesNoAnimation = false;
  isReactionAnimation = false;
  animationActive = true;
  animationStartTime = millis();
  animationStep = 0;
  
  // Play appropriate animation based on new emotion
  if (baseEmotion == "HAPPY") {
    roboEyes.anim_laugh();
  } else if (baseEmotion == "ANGRY") {
    roboEyes.anim_confused();
  } else if (baseEmotion == "TIRED") {
    // Custom slow blink
    roboEyes.blink();
  } else { // DEFAULT
    // Look around briefly
  }
}

void updateAnimations() {
  unsigned long currentTime = millis();
  
  if (!animationActive) return;
  
  // Handle YES/NO animations
  if (isYesNoAnimation) {
    updateYesNoAnimation();
    return;
  }
  
  // Handle reaction animations
  if (isReactionAnimation) {
    updateReactionAnimation();
    return;
  }
  
  // Check if regular animation duration is over
  if (currentTime - animationStartTime >= animationDuration) {
    animationActive = false;
    roboEyes.setPosition(DEFAULT);
    return;
  }
  
  // Update regular animation based on current emotion
  if (currentTime - lastAnimationUpdate >= animationInterval) {
    lastAnimationUpdate = currentTime;
    animationStep++;
    
    if (baseEmotion == "TIRED") {
      // Slow eye movement for tired
      if (animationStep % 10 == 0) {
        roboEyes.setPosition(S);
      } else if (animationStep % 5 == 0) {
        roboEyes.setPosition(DEFAULT);
      }
    } else if (baseEmotion == "DEFAULT") {
      // Look around animation for default
      switch(animationStep % 8) {
        case 0: roboEyes.setPosition(N); break;
        case 1: roboEyes.setPosition(NE); break;
        case 2: roboEyes.setPosition(E); break;
        case 3: roboEyes.setPosition(SE); break;
        case 4: roboEyes.setPosition(S); break;
        case 5: roboEyes.setPosition(SW); break;
        case 6: roboEyes.setPosition(W); break;
        case 7: roboEyes.setPosition(NW); break;
      }
    }
  }
}

void updateYesNoAnimation() {
  unsigned long currentTime = millis();
  
  // Check if it's time to update the animation step
  if (currentTime - lastAnimationUpdate >= yesNoStepDuration) {
    lastAnimationUpdate = currentTime;
    
    if (currentEmotion == "YES") {
      // YES = Vertical nod (up-down motion)
      switch(yesNoStep % 4) {
        case 0:
          roboEyes.setPosition(DEFAULT);
          break;
        case 1:
          roboEyes.setPosition(N);
          break;
        case 2:
          roboEyes.setPosition(DEFAULT);
          break;
        case 3:
          roboEyes.setPosition(S);
          break;
      }
      
      yesNoStep++;
      
      // Complete after 3 full nod cycles (12 steps)
      if (yesNoStep >= (yesSteps * 2)) {
        endYesNoAnimation();
      }
      
    } else if (currentEmotion == "NO") {
      // NO = Horizontal shake (left-right motion)
      switch(yesNoStep % 4) {
        case 0:
          roboEyes.setPosition(DEFAULT);
          break;
        case 1:
          roboEyes.setPosition(W);
          break;
        case 2:
          roboEyes.setPosition(DEFAULT);
          break;
        case 3:
          roboEyes.setPosition(E);
          break;
      }
      
      yesNoStep++;
      
      // Complete after 4 full shake cycles (16 steps)
      if (yesNoStep >= (noSteps * 2)) {
        endYesNoAnimation();
      }
    }
    
    // Force draw update during YES/NO animations
    roboEyes.drawEyes();
  }
}

void updateReactionAnimation() {
  unsigned long currentTime = millis();
  
  // Check if reaction animation duration is over
  if (currentTime - animationStartTime >= reactionAnimationDuration) {
    endReactionAnimation();
    return;
  }
  
  // Check if it's time to update the reaction step
  if (currentTime - lastAnimationUpdate >= reactionStepDuration) {
    lastAnimationUpdate = currentTime;
    reactionStep++;
    
    // Perform emotion-specific reaction
    if (baseEmotion == "HAPPY") {
      performHappyReaction();
    } else if (baseEmotion == "ANGRY") {
      performAngryReaction();
    } else if (baseEmotion == "TIRED") {
      performTiredReaction();
    } else { // DEFAULT
      performDefaultReaction();
    }
    
    // Force draw update during reaction animations
    roboEyes.drawEyes();
  }
}

void performHappyReaction() {
  // Happy: playful, bouncing movements
  switch (reactionStep % 6) {
    case 0:
      roboEyes.setPosition(NE);
      tiltServo.write(120);
      panServo.write(60);
      break;
    case 1:
      roboEyes.setPosition(NW);
      tiltServo.write(60);
      panServo.write(120);
      break;
    case 2:
      roboEyes.setPosition(SE);
      tiltServo.write(110);
      panServo.write(70);
      break;
    case 3:
      roboEyes.setPosition(SW);
      tiltServo.write(70);
      panServo.write(110);
      break;
    case 4:
      roboEyes.setPosition(N);
      roboEyes.blink();
      tiltServo.write(100);
      panServo.write(90);
      break;
    case 5:
      roboEyes.setPosition(DEFAULT);
      roboEyes.anim_laugh();
      tiltServo.write(90);
      panServo.write(90);
      break;
  }
}

void performAngryReaction() {
  // Angry: sharp side-to-side motions + aggressive tilt
  switch (reactionStep % 5) {
    case 0:
      roboEyes.setPosition(W);
      tiltServo.write(70);
      panServo.write(50);
      break;
    case 1:
      roboEyes.setPosition(E);
      tiltServo.write(70);
      panServo.write(130);
      break;
    case 2:
      roboEyes.setPosition(N);
      tiltServo.write(120);
      panServo.write(90);
      break;
    case 3:
      roboEyes.setPosition(DEFAULT);
      roboEyes.anim_confused();
      tiltServo.write(80);
      panServo.write(90);
      break;
    case 4:
      roboEyes.blink();
      tiltServo.write(90);
      panServo.write(90);
      break;
  }
}

void performTiredReaction() {
  // Tired: slow, droopy downward motions
  switch (reactionStep % 4) {
    case 0:
      roboEyes.setPosition(S);
      tiltServo.write(60);
      panServo.write(90);
      break;
    case 1:
      roboEyes.setPosition(SW);
      tiltServo.write(60);
      panServo.write(110);
      break;
    case 2:
      roboEyes.setPosition(SE);
      tiltServo.write(60);
      panServo.write(70);
      break;
    case 3:
      roboEyes.setPosition(DEFAULT);
      roboEyes.blink();
      tiltServo.write(90);
      panServo.write(90);
      break;
  }
}

void performDefaultReaction() {
  // Default reaction: Curious looking around + gentle servo movements
  switch (reactionStep % 10) {
    case 0:
      roboEyes.setPosition(N);
      tiltServo.write(120);
      panServo.write(80);
      break;
    case 1:
      roboEyes.setPosition(NE);
      tiltServo.write(130);
      panServo.write(90);
      break;
    case 2:
      roboEyes.setPosition(E);
      tiltServo.write(110);
      panServo.write(60);   // fixed syntax, was 50))
      break;
    case 3:
      roboEyes.setPosition(SE);
      tiltServo.write(100);
      panServo.write(70);
      break;
    case 4:
      roboEyes.setPosition(S);
      tiltServo.write(140);
      panServo.write(85);
      break;
    case 5:
      roboEyes.setPosition(SW);
      tiltServo.write(120);
      panServo.write(100);
      break;
    case 6:
      roboEyes.setPosition(W);
      tiltServo.write(100);
      panServo.write(120);
      break;
    case 7:
      roboEyes.setPosition(NW);
      tiltServo.write(130);
      panServo.write(95);
      break;
    case 8:
      roboEyes.setPosition(DEFAULT);
      roboEyes.blink();
      tiltServo.write(90);
      panServo.write(90);
      break;
    case 9:
      roboEyes.setPosition(N);
      tiltServo.write(110);
      panServo.write(100);
      break;
  }
}

void endYesNoAnimation() {
  Serial.println("Ending YES/NO animation");
  
  // Reset animation state
  isYesNoAnimation = false;
  animationActive = false;
  yesNoStep = 0;
  
  // Return to center position
  roboEyes.setPosition(DEFAULT);
  
  // Return to DEFAULT emotion after YES/NO
  currentEmotion = "DEFAULT";
  baseEmotion = "DEFAULT";
  hasReaction = false;
  displayEmotion(); // This will re-enable auto-animations
  
  Serial.println("Returned to DEFAULT emotion");
}

void endReactionAnimation() {
  Serial.println("Ending reaction animation for: " + baseEmotion);
  
  // Reset animation state
  isReactionAnimation = false;
  animationActive = false;
  reactionStep = 0;
  
  // Return servos to center position
  tiltServo.write(90);
  panServo.write(90);
  
  // Return eyes to center position
  roboEyes.setPosition(DEFAULT);
  
  // Set the final emotion (base emotion without reaction)
  currentEmotion = baseEmotion;
  hasReaction = false;
  displayEmotion(); // This will re-enable auto-animations with the base emotion
  
  Serial.println("Finished reaction, now displaying: " + baseEmotion);
}

void displayConnectionLost() {
  // Use the display directly for connection lost message
  // Temporarily disable RoboEyes to show message
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Connection Lost!");
  display.setCursor(0, 15);
  display.println("Attempting to");
  display.setCursor(0, 25);
  display.println("reconnect...");
  
  // Draw simple sad eyes manually
  display.fillCircle(35, 40, 8, SSD1306_WHITE);
  display.fillCircle(35, 40, 4, SSD1306_BLACK);
  display.fillCircle(85, 40, 8, SSD1306_WHITE);
  display.fillCircle(85, 40, 4, SSD1306_BLACK);
  
  display.display();
}

void nodYes() {
  Serial.println("Executing physical YES nod");
  for(int i = 0; i < 3; i++) {
    tiltServo.write(120); // Look up
    delay(100);
    tiltServo.write(60);  // Look down
    delay(100);
  }
  tiltServo.write(90);    // Return to center
}

void shakeNo() {
  Serial.println("Executing physical NO shake");
  for(int i = 0; i < 4; i++) {
    panServo.write(60);   // Turn left
    delay(100);
    panServo.write(120);  // Turn right
    delay(100);
  }
  panServo.write(90);     // Return to center
}