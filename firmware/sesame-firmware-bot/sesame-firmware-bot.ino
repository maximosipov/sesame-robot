#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <ESP32Servo.h>
#include "driver/adc.h"
#include "movement-bot.h"
#include "captive-portal-bot.h"

// --- Access Point Configuration ---
#define AP_SSID  "Sesame-Controller-BETA"
#define AP_PASS  "12345678"

// --- Station Mode Configuration (Optional) ---
#define NETWORK_SSID "VodafoneD52E4E"
#define NETWORK_PASS "EbNAdJysfxNrYmxb"
#define ENABLE_NETWORK_MODE true

// DNS Server for Captive Portal
DNSServer dnsServer;
const byte DNS_PORT = 53;

WebServer server(80);

// Global state
String currentCommand = "";

// Network Mode
bool networkConnected = false;
IPAddress networkIP;
String deviceHostname = "sesame-robot";

// Servo Pins
Servo servos[8];
// Lolin S2 Mini Pinout
const int servoPins[8] = {1, 2, 4, 6, 8, 10, 13, 14};

// Subtrim values for each servo (offset in degrees)
int8_t servoSubtrim[8] = {0, 0, 0, 0, 0, 0, 0, 0};

// Servo auto-detach after manual control
unsigned long servoDetachTime[8] = {0,0,0,0,0,0,0,0};
bool servoDetachPending[8] = {false,false,false,false,false,false,false,false};
#define SERVO_DETACH_DELAY 2000

// ADC Input
#define ADC_PIN 3
unsigned long lastAdcReadMs = 0;

// Sensor streaming (SSE)
WiFiClient sensorClient;
bool sensorStreamActive = false;

// Movement constants
int frameDelay = 100;
int walkCycles = 10;
int motorCurrentDelay = 20;

// Prototypes
void setServoAngle(uint8_t channel, int angle);
void serviceDelay(unsigned long ms);
bool pressingCheck(String cmd, int ms);
void handleGetSettings();
void handleSetSettings();
void handleGetStatus();
void handleApiCommand();
void recordInput();
void scheduleServoDetach(uint8_t channel);
void scheduleAllServoDetach();
void checkServoDetach();
void handleSensors();

void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleCommandWeb() {
  if (server.hasArg("go")) {
    currentCommand = server.arg("go");
    recordInput();
    server.send(200, "text/plain", "OK");
  }
  else if (server.hasArg("stop")) {
    currentCommand = "";
    recordInput();
    server.send(200, "text/plain", "OK");
  }
  else if (server.hasArg("servoBtn") && server.hasArg("angle")) {
    String servoName = server.arg("servoBtn");
    int angle = server.arg("angle").toInt();
    int idx = servoNameToIndex(servoName);
    if (idx != -1 && angle >= 0 && angle <= 180) {
      runServoToAngle(idx, angle);
      recordInput();
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Invalid servo or angle");
    }
  }
  else if (server.hasArg("motor") && server.hasArg("value")) {
    int motorNum = server.arg("motor").toInt();
    int servoIdx = servoNameToIndex(server.arg("motor"));
    int angle = server.arg("value").toInt();
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

void handleSensors() {
  sensorClient = server.client();
  sensorClient.println("HTTP/1.1 200 OK");
  sensorClient.println("Content-Type: text/event-stream");
  sensorClient.println("Cache-Control: no-cache");
  sensorClient.println("Connection: keep-alive");
  sensorClient.println("Access-Control-Allow-Origin: *");
  sensorClient.println();
  sensorStreamActive = true;
  Serial.println(F("Sensor stream connected"));
}

void handleGetSettings() {
  String json = "{";
  json += "\"frameDelay\":" + String(frameDelay) + ",";
  json += "\"walkCycles\":" + String(walkCycles) + ",";
  json += "\"motorCurrentDelay\":" + String(motorCurrentDelay);
  json += "}";
  server.send(200, "application/json", json);
}

void handleSetSettings() {
  if (server.hasArg("frameDelay")) frameDelay = server.arg("frameDelay").toInt();
  if (server.hasArg("walkCycles")) walkCycles = server.arg("walkCycles").toInt();
  if (server.hasArg("motorCurrentDelay")) motorCurrentDelay = server.arg("motorCurrentDelay").toInt();
  server.send(200, "text/plain", "OK");
}

void handleGetStatus() {
  String json = "{";
  json += "\"currentCommand\":\"" + currentCommand + "\",";
  json += "\"networkConnected\":" + String(networkConnected ? "true" : "false") + ",";
  json += "\"apIP\":\"" + WiFi.softAPIP().toString() + "\"";
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

  String command = "";

  int cmdStart = body.indexOf("\"command\":\"");
  if (cmdStart == -1) {
    cmdStart = body.indexOf("\"command\": \"");
  }

  if (cmdStart == -1) {
    Serial.println("Error: command field not found");
    server.send(400, "application/json", "{\"error\":\"Missing command field\"}");
    return;
  }

  cmdStart = body.indexOf("\"", cmdStart + 10) + 1;
  int cmdEnd = body.indexOf("\"", cmdStart);

  if (cmdEnd <= cmdStart) {
    Serial.println("Error: invalid command format");
    server.send(400, "application/json", "{\"error\":\"Invalid command format\"}");
    return;
  }

  command = body.substring(cmdStart, cmdEnd);
  Serial.print("Parsed command: ");
  Serial.println(command);

  if (command == "stop") {
    currentCommand = "";
    recordInput();
    server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Command stopped\"}");
  } else {
    currentCommand = command;
    recordInput();
    server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Command executed\"}");
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println(F("Sesame Bot starting..."));

  // --- WIFI CONFIGURATION ---
  if (ENABLE_NETWORK_MODE && String(NETWORK_SSID).length() > 0) {
    Serial.println("Attempting to connect to network: " + String(NETWORK_SSID));
    WiFi.mode(WIFI_AP_STA);
    WiFi.setHostname(deviceHostname.c_str());
    WiFi.begin(NETWORK_SSID, NETWORK_PASS);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      networkConnected = true;
      networkIP = WiFi.localIP();
      Serial.println();
      Serial.print("Connected to network! IP: ");
      Serial.println(networkIP);
    } else {
      Serial.println();
      Serial.println("Failed to connect to network. Running in AP-only mode.");
      WiFi.mode(WIFI_AP);
    }
  } else {
    WiFi.mode(WIFI_AP);
    Serial.println("Network mode disabled. Running in AP-only mode.");
  }

  // --- ACCESS POINT CONFIGURATION ---
  WiFi.softAP(AP_SSID, AP_PASS);
  IPAddress myIP = WiFi.softAPIP();

  Serial.print("AP Created. IP: ");
  Serial.println(myIP);

  // Start mDNS
  if (MDNS.begin(deviceHostname.c_str())) {
    Serial.println("mDNS responder started");
    Serial.print("Access controller at: http://");
    Serial.print(deviceHostname);
    Serial.println(".local");
    MDNS.addService("http", "tcp", 80);
  } else {
    Serial.println("Error setting up mDNS responder!");
  }

  // Start DNS Server for Captive Portal
  dnsServer.start(DNS_PORT, "*", myIP);

  // Web Server Routes
  server.on("/", handleRoot);
  server.on("/cmd", handleCommandWeb);
  server.on("/sensors", handleSensors);
  server.on("/getSettings", handleGetSettings);
  server.on("/setSettings", handleSetSettings);

  // API endpoints for network communication
  server.on("/api/status", handleGetStatus);
  server.on("/api/command", handleApiCommand);

  // Catch-all route for captive portal
  server.onNotFound(handleRoot);

  server.begin();

  // PWM Init
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  for (int i = 0; i < 8; i++) {
    servos[i].setPeriodHertz(50);
    servos[i].attach(servoPins[i], 732, 2929);
  }
  delay(10);

  // ADC Init - GPIO3 = ADC1_CH2 on ESP32-S2, 11dB attenuation for 0-3.3V range
  adc1_config_width(ADC_WIDTH_BIT_13);
  adc1_config_channel_atten(ADC1_CHANNEL_2, ADC_ATTEN_DB_11);
  gpio_pullup_dis((gpio_num_t)ADC_PIN);
  gpio_pulldown_dis((gpio_num_t)ADC_PIN);

  Serial.println(F("HTTP server & Captive Portal started. (Bot mode - no face/poses)"));
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  checkServoDetach();

  // Read ADC every second - SHARP 2Y0A02 distance sensor
  if (millis() - lastAdcReadMs >= 1000) {
    lastAdcReadMs = millis();
    int adcValue = adc1_get_raw(ADC1_CHANNEL_2);
    float voltage = adcValue * 3.3 / 8191.0;
    float distCm = (voltage > 0.1) ? (60.0 / voltage) : 0;
    Serial.print(F("DIST: ")); Serial.print(distCm, 1); Serial.println(F("cm"));

    // Push to SSE stream if connected
    if (sensorStreamActive) {
      if (sensorClient.connected()) {
        sensorClient.print("data: DIST: ");
        sensorClient.print(distCm, 1);
        sensorClient.println("cm");
        sensorClient.println();
      } else {
        sensorStreamActive = false;
        Serial.println(F("Sensor stream disconnected"));
      }
    }
  }

  // Movement commands only
  if (currentCommand != "") {
    String cmd = currentCommand;
    if (cmd == "forward") runWalkPose();
    else if (cmd == "backward") runWalkBackward();
    else if (cmd == "left") runTurnLeft();
    else if (cmd == "right") runTurnRight();
    else if (cmd == "stand") { runStandPose(); if (currentCommand == "stand") currentCommand = ""; }
  }

  // Serial CLI
  if (Serial.available()) {
    static char command_buffer[32];
    static byte buffer_pos = 0;
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (buffer_pos > 0) {
        command_buffer[buffer_pos] = '\0';
        int motorNum, angle;
        recordInput();
        if(strcmp(command_buffer, "run walk") == 0 || strcmp(command_buffer, "rn wf") == 0) { currentCommand = "forward"; runWalkPose(); currentCommand = ""; }
        else if(strcmp(command_buffer, "rn wb") == 0) { currentCommand = "backward"; runWalkBackward(); currentCommand = ""; }
        else if(strcmp(command_buffer, "rn tl") == 0) { currentCommand = "left"; runTurnLeft(); currentCommand = ""; }
        else if(strcmp(command_buffer, "rn tr") == 0) { currentCommand = "right"; runTurnRight(); currentCommand = ""; }
        else if(strcmp(command_buffer, "run stand") == 0 || strcmp(command_buffer, "rn st") == 0) runStandPose();
        else if (strcmp(command_buffer, "subtrim") == 0 || strcmp(command_buffer, "st") == 0) {
          Serial.println("Subtrim values:");
          for (int i = 0; i < 8; i++) {
            Serial.print("Motor "); Serial.print(i); Serial.print(": ");
            if (servoSubtrim[i] >= 0) Serial.print("+");
            Serial.println(servoSubtrim[i]);
          }
        }
        else if (strcmp(command_buffer, "subtrim save") == 0 || strcmp(command_buffer, "st save") == 0) {
          Serial.println("Copy and paste this into your code:");
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
                servoSubtrim[trimMotor] = trimValue;
                Serial.print("Motor "); Serial.print(trimMotor); Serial.print(" subtrim set to ");
                if (trimValue >= 0) Serial.print("+");
                Serial.println(trimValue);
              } else {
                Serial.println("Subtrim value must be between -90 and +90");
              }
            } else {
              Serial.println("Invalid motor number (0-7)");
            }
          }
        }
        else if (strchr(command_buffer, '-') != NULL) {
          char* dashPos = strchr(command_buffer, '-');
          *dashPos = '\0';
          String servoName = String(command_buffer);
          int angle = atoi(dashPos + 1);
          int idx = servoNameToIndex(servoName);
          if (idx != -1 && angle >= 0 && angle <= 180) {
            runServoToAngle(idx, angle);
          } else {
            Serial.println("Invalid servo name or angle (0-180)");
          }
          *dashPos = '-';
        }
        else if (strncmp(command_buffer, "all ", 4) == 0) {
             if (sscanf(command_buffer + 4, "%d", &angle) == 1) {
                 for (int i = 0; i < 8; i++) setServoAngle(i, angle);
                 Serial.print("All servos set to "); Serial.println(angle);
             }
        }
        else if (sscanf(command_buffer, "%d %d", &motorNum, &angle) == 2) {
             if (motorNum >= 0 && motorNum < 8) {
                 setServoAngle(motorNum, angle);
                 Serial.print("Servo "); Serial.print(motorNum); Serial.print(" set to "); Serial.println(angle);
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

// ====== HELPERS ======
void setServoAngle(uint8_t channel, int angle) {
  if (channel < 8) {
    if (!servos[channel].attached()) {
      servos[channel].attach(servoPins[channel], 732, 2929);
    }
    int adjustedAngle = constrain(angle + servoSubtrim[channel], 10, 170);
    servos[channel].write(adjustedAngle);
    serviceDelay(motorCurrentDelay);
  }
}

void scheduleServoDetach(uint8_t channel) {
  if (channel < 8) {
    servoDetachTime[channel] = millis() + SERVO_DETACH_DELAY;
    servoDetachPending[channel] = true;
  }
}

void scheduleAllServoDetach() {
  for (uint8_t i = 0; i < 8; i++) {
    scheduleServoDetach(i);
  }
}

void checkServoDetach() {
  unsigned long now = millis();
  for (uint8_t i = 0; i < 8; i++) {
    if (servoDetachPending[i] && now >= servoDetachTime[i]) {
      servos[i].detach();
      servoDetachPending[i] = false;
    }
  }
}

// Non-blocking delay that keeps servicing web requests
void serviceDelay(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    server.handleClient();
    dnsServer.processNextRequest();
    delay(5);
  }
}

bool pressingCheck(String cmd, int ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    server.handleClient();
    dnsServer.processNextRequest();
    if (currentCommand != cmd) {
      runStandPose();
      return false;
    }
    yield();
  }
  return true;
}

void recordInput() {
  // Placeholder for input tracking (no display to update)
}
