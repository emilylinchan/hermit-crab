#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <ESP32Servo.h>
#include "movement-sequences.h"
#include "captive-portal.h"

// --- Access Point Configuration ---
#define AP_SSID  "Sesame-Controller"
#define AP_PASS  "12345678"

// --- Station Mode Configuration (Optional) ---
#define NETWORK_SSID ""
#define NETWORK_PASS ""
#define ENABLE_NETWORK_MODE false

// DNS Server for Captive Portal
DNSServer dnsServer;
const byte DNS_PORT = 53;

WebServer server(80);

// Global state
String currentCommand = "";

// WiFi Info / input tracking
unsigned long lastInputTime = 0;
bool firstInputReceived = false;

// Network Mode
bool networkConnected = false;
IPAddress networkIP;
String deviceHostname = "sesame-robot";

// ======================================================================
// Servo pin mapping
// ======================================================================
Servo servos[8];

// ESP32 GPIO pins
const int servoPins[8] = {13, 14, 15, 16, 17, 18, 19, 21};

// Subtrim values for each servo (offset in degrees)
int8_t servoSubtrim[8] = {0, 0, 0, 0, 0, 0, 0, 0};

// Full range: 500–2500 µs = 2000 µs span for 270°
//
// To use only the center 180°:
// remove 45° from each end
//
// 45° / 270° * 2000 µs ≈ 333 µs
//
// Result:
// 833–2167 µs usable range
const int SERVO_MIN_US = 833;   // 500  + 333
const int SERVO_MAX_US = 2167;  // 2500 - 333

// Animation / movement constants
int frameDelay  = 100;
int walkCycles  = 10;
int motorCurrentDelay = 5;

// Stub face/idle functions required by movement-sequences.h
// (no display connected — these do nothing)
String currentFaceName = "none";
FaceAnimMode currentFaceMode = FACE_ANIM_LOOP;
bool faceAnimFinished = false;
bool idleActive = false;

// Prototypes
void setServoAngle(uint8_t channel, int angle);
void setFace(const String& faceName);
void setFaceMode(FaceAnimMode mode);
void setFaceWithMode(const String& faceName, FaceAnimMode mode);
void delayWithFace(unsigned long ms);
void enterIdle();
void exitIdle();
bool pressingCheck(String cmd, int ms);
void handleGetSettings();
void handleSetSettings();
void handleGetStatus();
void handleApiCommand();
void recordInput();

// ======================================================================
// Face stubs — movement-sequences.h calls these; with no OLED they
// are no-ops so the rest of the code compiles and runs unchanged.
// ======================================================================
void setFace(const String& faceName)                          { currentFaceName = faceName; }
void setFaceMode(FaceAnimMode mode)                           { currentFaceMode = mode; faceAnimFinished = false; }
void setFaceWithMode(const String& faceName, FaceAnimMode m)  { setFaceMode(m); setFace(faceName); }

void enterIdle() {
  idleActive = true;
  setFaceWithMode("idle", FACE_ANIM_BOOMERANG);
}

void exitIdle() {
  idleActive = false;
}

// delayWithFace — keeps web server alive during waits
void delayWithFace(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    server.handleClient();
    dnsServer.processNextRequest();
    delay(5);
  }
}

// ======================================================================
// Web server handlers
// ======================================================================
void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleCommandWeb() {
  if (server.hasArg("pose")) {
    currentCommand = server.arg("pose");
    recordInput();
    exitIdle();
    server.send(200, "text/plain", "OK");
  }
  else if (server.hasArg("go")) {
    currentCommand = server.arg("go");
    recordInput();
    exitIdle();
    server.send(200, "text/plain", "OK");
  }
  else if (server.hasArg("stop")) {
    currentCommand = "";
    recordInput();
    server.send(200, "text/plain", "OK");
  }
  else if (server.hasArg("motor") && server.hasArg("value")) {
    int motorNum  = server.arg("motor").toInt();
    int servoIdx  = servoNameToIndex(server.arg("motor"));
    int angle     = server.arg("value").toInt();
    if (motorNum >= 1 && motorNum <= 8 && angle >= 0 && angle <= 180) {
      setServoAngle(motorNum - 1, angle);
      recordInput();
      server.send(200, "text/plain", "OK");
    } else if (servoIdx != -1 && angle >= 0 && angle <= 180) {
      setServoAngle(servoIdx, angle);
      recordInput();
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Invalid motor or angle");
    }
  }
  else {
    server.send(400, "text/plain", "Bad Args");
  }
}

void handleGetSettings() {
  String json = "{";
  json += "\"frameDelay\":"        + String(frameDelay)        + ",";
  json += "\"walkCycles\":"        + String(walkCycles)        + ",";
  json += "\"motorCurrentDelay\":" + String(motorCurrentDelay);
  json += "}";
  server.send(200, "application/json", json);
}

void handleSetSettings() {
  if (server.hasArg("frameDelay"))        frameDelay        = server.arg("frameDelay").toInt();
  if (server.hasArg("walkCycles"))        walkCycles        = server.arg("walkCycles").toInt();
  if (server.hasArg("motorCurrentDelay")) motorCurrentDelay = server.arg("motorCurrentDelay").toInt();
  server.send(200, "text/plain", "OK");
}

void handleGetStatus() {
  String json = "{";
  json += "\"currentCommand\":\"" + currentCommand + "\",";
  json += "\"networkConnected\":"  + String(networkConnected ? "true" : "false") + ",";
  json += "\"apIP\":\""            + WiFi.softAPIP().toString() + "\"";
  if (networkConnected) {
    json += ",\"networkIP\":\"" + networkIP.toString() + "\"";
  }
  json += "}";
  server.send(200, "application/json", json);
}

void handleApiCommand() {
  if (server.method() != HTTP_POST) {
    server.send(405, "application/json", "{\"error\":\"Method not allowed\"}");
    return;
  }

  String body = server.arg("plain");
  Serial.println("API Command received:");
  Serial.println(body);

  // Check for face-only command
  int faceOnlyStart = body.indexOf("\"face\":\"");
  if (faceOnlyStart == -1) faceOnlyStart = body.indexOf("\"face\": \"");
  bool faceOnly = (faceOnlyStart > 0 &&
                   body.indexOf("\"command\":") == -1 &&
                   body.indexOf("\"command\": ") == -1);

  String command = "";
  String face    = "";

  if (faceOnlyStart > 0) {
    faceOnlyStart = body.indexOf("\"", faceOnlyStart + 6) + 1;
    int faceEnd   = body.indexOf("\"", faceOnlyStart);
    if (faceEnd > faceOnlyStart) face = body.substring(faceOnlyStart, faceEnd);
  }

  if (!faceOnly) {
    int cmdStart = body.indexOf("\"command\":\"");
    if (cmdStart == -1) cmdStart = body.indexOf("\"command\": \"");
    if (cmdStart == -1) {
      server.send(400, "application/json", "{\"error\":\"Missing command field\"}");
      return;
    }
    cmdStart   = body.indexOf("\"", cmdStart + 10) + 1;
    int cmdEnd = body.indexOf("\"", cmdStart);
    if (cmdEnd <= cmdStart) {
      server.send(400, "application/json", "{\"error\":\"Invalid command format\"}");
      return;
    }
    command = body.substring(cmdStart, cmdEnd);
  }

  if (face.length() > 0) setFace(face);

  if (faceOnly) {
    recordInput();
    server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Face updated\"}");
    return;
  }

  if (command == "stop") {
    currentCommand = "";
  } else {
    currentCommand = command;
    exitIdle();
  }
  recordInput();
  server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Command executed\"}");
}

// ======================================================================
// Setup
// ======================================================================
void setup() {
  Serial.begin(115200);
  randomSeed(micros());

  Serial.println(F("Sesame Robot starting..."));

  // --- WiFi ---
  if (ENABLE_NETWORK_MODE && String(NETWORK_SSID).length() > 0) {
    WiFi.mode(WIFI_AP_STA);
    WiFi.setHostname(deviceHostname.c_str());
    WiFi.begin(NETWORK_SSID, NETWORK_PASS);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500); Serial.print("."); attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      networkConnected = true;
      networkIP = WiFi.localIP();
      Serial.println();
      Serial.print("Network IP: "); Serial.println(networkIP);
    } else {
      Serial.println(); Serial.println("Network failed, AP-only mode.");
      WiFi.mode(WIFI_AP);
    }
  } else {
    WiFi.mode(WIFI_AP);
  }

  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

  if (MDNS.begin(deviceHostname.c_str())) {
    MDNS.addService("http", "tcp", 80);
    Serial.print("mDNS: http://"); Serial.print(deviceHostname); Serial.println(".local");
  }

  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  server.on("/",            handleRoot);
  server.on("/cmd",         handleCommandWeb);
  server.on("/getSettings", handleGetSettings);
  server.on("/setSettings", handleSetSettings);
  server.on("/api/status",  handleGetStatus);
  server.on("/api/command", handleApiCommand);
  server.onNotFound(handleRoot);
  server.begin();

  // --- Servo init with 270° calibration ---
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  for (int i = 0; i < 8; i++) {
    servos[i].setPeriodHertz(50);
    servos[i].attach(servoPins[i], SERVO_MIN_US, SERVO_MAX_US);
  }
  delay(10);

  lastInputTime      = millis();
  firstInputReceived = false;

  Serial.println(F("Ready. Commands: rn st, rn rs, rn wf, rn wb, rn tl, rn tr,"));
  Serial.println(F("                rn wv, rn dn, rn sw, rn pt, rn pu, rn bw,"));
  Serial.println(F("                rn ct, rn fk, rn wm, rn sk, rn sg, rn dd, rn cb"));
  Serial.println(F("                0 90 (motor angle)   all 90 (all motors)"));
}

// ======================================================================
// Loop
// ======================================================================
void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  if (currentCommand != "") {
    String cmd = currentCommand;
    if      (cmd == "forward")  runWalkPose();
    else if (cmd == "backward") runWalkBackward();
    else if (cmd == "left")     runTurnLeft();
    else if (cmd == "right")    runTurnRight();
    else if (cmd == "rest")     { runRestPose();    if (currentCommand == "rest")  currentCommand = ""; }
    else if (cmd == "stand")    { runStandPose(1);  if (currentCommand == "stand") currentCommand = ""; }
    else if (cmd == "wave")     runWavePose();
    else if (cmd == "dance")    runDancePose();
    else if (cmd == "swim")     runSwimPose();
    else if (cmd == "point")    runPointPose();
    else if (cmd == "pushup")   runPushupPose();
    else if (cmd == "bow")      runBowPose();
    else if (cmd == "cute")     runCutePose();
    else if (cmd == "freaky")   runFreakyPose();
    else if (cmd == "worm")     runWormPose();
    else if (cmd == "shake")    runShakePose();
    else if (cmd == "shrug")    runShrugPose();
    else if (cmd == "dead")     runDeadPose();
    else if (cmd == "crab")     runCrabPose();
  }

  // --- Serial CLI ---
  if (Serial.available()) {
    static char command_buffer[32];
    static byte buffer_pos = 0;
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (buffer_pos > 0) {
        command_buffer[buffer_pos] = '\0';
        int motorNum, angle;
        recordInput();

        if      (strcmp(command_buffer, "run walk") == 0 || strcmp(command_buffer, "rn wf") == 0)
          { currentCommand = "forward";  runWalkPose();      currentCommand = ""; }
        else if (strcmp(command_buffer, "rn wb") == 0)
          { currentCommand = "backward"; runWalkBackward();  currentCommand = ""; }
        else if (strcmp(command_buffer, "rn tl") == 0)
          { currentCommand = "left";     runTurnLeft();      currentCommand = ""; }
        else if (strcmp(command_buffer, "rn tr") == 0)
          { currentCommand = "right";    runTurnRight();     currentCommand = ""; }
        else if (strcmp(command_buffer, "run rest")  == 0 || strcmp(command_buffer, "rn rs") == 0) runRestPose();
        else if (strcmp(command_buffer, "run stand") == 0 || strcmp(command_buffer, "rn st") == 0) runStandPose(1);
        else if (strcmp(command_buffer, "rn wv") == 0) { currentCommand = "wave";   runWavePose(); }
        else if (strcmp(command_buffer, "rn dn") == 0) { currentCommand = "dance";  runDancePose(); }
        else if (strcmp(command_buffer, "rn sw") == 0) { currentCommand = "swim";   runSwimPose(); }
        else if (strcmp(command_buffer, "rn pt") == 0) { currentCommand = "point";  runPointPose(); }
        else if (strcmp(command_buffer, "rn pu") == 0) { currentCommand = "pushup"; runPushupPose(); }
        else if (strcmp(command_buffer, "rn bw") == 0) { currentCommand = "bow";    runBowPose(); }
        else if (strcmp(command_buffer, "rn ct") == 0) { currentCommand = "cute";   runCutePose(); }
        else if (strcmp(command_buffer, "rn fk") == 0) { currentCommand = "freaky"; runFreakyPose(); }
        else if (strcmp(command_buffer, "rn wm") == 0) { currentCommand = "worm";   runWormPose(); }
        else if (strcmp(command_buffer, "rn sk") == 0) { currentCommand = "shake";  runShakePose(); }
        else if (strcmp(command_buffer, "rn sg") == 0) { currentCommand = "shrug";  runShrugPose(); }
        else if (strcmp(command_buffer, "rn dd") == 0) { currentCommand = "dead";   runDeadPose(); }
        else if (strcmp(command_buffer, "rn cb") == 0) { currentCommand = "crab";   runCrabPose(); }

        // Subtrim commands
        else if (strcmp(command_buffer, "subtrim") == 0 || strcmp(command_buffer, "st") == 0) {
          Serial.println("Subtrim values:");
          for (int i = 0; i < 8; i++) {
            Serial.print("Motor "); Serial.print(i); Serial.print(": ");
            if (servoSubtrim[i] >= 0) Serial.print("+");
            Serial.println(servoSubtrim[i]);
          }
        }
        else if (strcmp(command_buffer, "subtrim save") == 0 || strcmp(command_buffer, "st save") == 0) {
          Serial.println("Copy this into your code:");
          Serial.print("int8_t servoSubtrim[8] = {");
          for (int i = 0; i < 8; i++) {
            Serial.print(servoSubtrim[i]);
            if (i < 7) Serial.print(", ");
          }
          Serial.println("};");
        }
        else if (strncmp(command_buffer, "subtrim reset", 13) == 0 || strncmp(command_buffer, "st reset", 8) == 0) {
          for (int i = 0; i < 8; i++) servoSubtrim[i] = 0;
          Serial.println("All subtrim values reset to 0");
        }
        else if (strncmp(command_buffer, "subtrim ", 8) == 0 || strncmp(command_buffer, "st ", 3) == 0) {
          const char* params = (command_buffer[1] == 't') ? command_buffer + 3 : command_buffer + 8;
          int trimMotor, trimValue;
          if (sscanf(params, "%d %d", &trimMotor, &trimValue) == 2) {
            if (trimMotor >= 0 && trimMotor < 8) {
              if (trimValue >= -90 && trimValue <= 90) {
                servoSubtrim[trimMotor] = (int8_t)trimValue;
                Serial.print("Motor "); Serial.print(trimMotor);
                Serial.print(" subtrim set to ");
                if (trimValue >= 0) Serial.print("+");
                Serial.println(trimValue);
              } else {
                Serial.println("Subtrim must be -90 to +90");
              }
            } else {
              Serial.println("Invalid motor number (0-7)");
            }
          }
        }

        // Direct motor control
        else if (strncmp(command_buffer, "all ", 4) == 0) {
          if (sscanf(command_buffer + 4, "%d", &angle) == 1) {
            for (int i = 0; i < 8; i++) setServoAngle(i, angle);
            Serial.print("All servos -> "); Serial.println(angle);
          }
        }
        else if (sscanf(command_buffer, "%d %d", &motorNum, &angle) == 2) {
          if (motorNum >= 0 && motorNum < 8) {
            setServoAngle(motorNum, angle);
            Serial.print("Servo "); Serial.print(motorNum);
            Serial.print(" -> "); Serial.println(angle);
          } else {
            Serial.println("Invalid motor number (0-7)");
          }
        }

        buffer_pos = 0;
      }
    } else if (buffer_pos < sizeof(command_buffer) - 1) {
      command_buffer[buffer_pos++] = c;
    }
  }
}

// ======================================================================
// Helpers
// ======================================================================
void setServoAngle(uint8_t channel, int angle) {
  if (channel < 8) {
    int adjusted = constrain(angle + servoSubtrim[channel], 0, 180);
    int pulseUs  = map(adjusted, 0, 180, SERVO_MIN_US, SERVO_MAX_US);
    servos[channel].writeMicroseconds(pulseUs);
    delayWithFace(motorCurrentDelay);
  }
}

bool pressingCheck(String cmd, int ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    server.handleClient();
    dnsServer.processNextRequest();
    if (currentCommand != cmd) {
      runStandPose(1);
      return false;
    }
    yield();
  }
  return true;
}

void recordInput() {
  lastInputTime = millis();
  firstInputReceived = true;
}
