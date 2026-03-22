#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include "mdns.h"
#include "esp_netif.h"
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

// Servo timing
int motorCurrentDelay = 20;

// Prototypes
void setServoAngle(uint8_t channel, int angle);
void serviceDelay(unsigned long ms);
void handleGetSettings();
void handleSetSettings();
void handleGetStatus();
void handleApiCommand();
void recordInput();
void scheduleServoDetach(uint8_t channel);
void scheduleAllServoDetach();
void checkServoDetach();
void handleControl();
void handleSensors();

void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleCommandWeb() {
  if (server.hasArg("servoBtn") && server.hasArg("angle")) {
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

void handleControl() {
  if (!server.hasArg("leg") || !server.hasArg("direction") || !server.hasArg("angle")) {
    server.send(400, "application/json", "{\"error\":\"Missing leg, direction, or angle\"}");
    return;
  }

  String leg = server.arg("leg");
  String direction = server.arg("direction");
  int angle = server.arg("angle").toInt();

  if (angle < 0 || angle > 90) {
    server.send(400, "application/json", "{\"error\":\"Angle must be 0-90\"}");
    return;
  }

  // Map leg + direction to servo index
  // fwd/back servos: L1=left_front, L2=left_back, R1=right_front, R2=right_back
  // up/down servos:  L3=left_front, L4=left_back, R3=right_front, R4=right_back
  int servoIdx = -1;
  if (leg == "left_front") {
    servoIdx = (direction == "fwd" || direction == "back") ? L1 : L3;
  } else if (leg == "left_back") {
    servoIdx = (direction == "fwd" || direction == "back") ? L2 : L4;
  } else if (leg == "right_front") {
    servoIdx = (direction == "fwd" || direction == "back") ? R1 : R3;
  } else if (leg == "right_back") {
    servoIdx = (direction == "fwd" || direction == "back") ? R2 : R4;
  }

  if (servoIdx == -1) {
    server.send(400, "application/json", "{\"error\":\"Invalid leg\"}");
    return;
  }

  // Convert direction + angle to servo angle (90 = neutral)
  // fwd/back: left side fwd=90+angle, right side fwd=90-angle (mirrored)
  // up/down:  both sides same: up=90+angle, down=90-angle
  bool isRight = (leg == "right_front" || leg == "right_back");
  int servoAngle;
  if (direction == "fwd") {
    servoAngle = isRight ? (90 + angle) : (90 - angle);
  } else if (direction == "back") {
    servoAngle = isRight ? (90 - angle) : (90 + angle);
  } else if (direction == "up") {
    bool upPositive = (leg == "left_front" || leg == "right_back");
    servoAngle = upPositive ? (90 + angle) : (90 - angle);
  } else if (direction == "down") {
    bool upPositive = (leg == "left_front" || leg == "right_back");
    servoAngle = upPositive ? (90 - angle) : (90 + angle);
  } else {
    server.send(400, "application/json", "{\"error\":\"Invalid direction (fwd/back/up/down)\"}");
    return;
  }

  runServoToAngle(servoIdx, servoAngle);
  Serial.print(F("CTRL: ")); Serial.print(leg); Serial.print(F(" "));
  Serial.print(direction); Serial.print(F(" ")); Serial.println(angle);
  server.send(200, "application/json", "{\"status\":\"ok\"}");
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
  json += "\"motorCurrentDelay\":" + String(motorCurrentDelay);
  json += "}";
  server.send(200, "application/json", json);
}

void handleSetSettings() {
  if (server.hasArg("motorCurrentDelay")) motorCurrentDelay = server.arg("motorCurrentDelay").toInt();
  server.send(200, "text/plain", "OK");
}

void handleGetStatus() {
  String json = "{";
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

  // Parse servo commands (e.g. "L1-90")
  int dashIdx = command.indexOf('-');
  if (dashIdx > 0) {
    String servoName = command.substring(0, dashIdx);
    int angle = command.substring(dashIdx + 1).toInt();
    int idx = servoNameToIndex(servoName);
    if (idx != -1 && angle >= 0 && angle <= 180) {
      runServoToAngle(idx, angle);
      recordInput();
      server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Servo command executed\"}");
      return;
    }
  }
  server.send(400, "application/json", "{\"error\":\"Unknown command\"}");
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
      WiFi.enableIPv6();
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
  WiFi.softAPenableIPv6();
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

    // Enable IPv6 mDNS responses. Without this, clients (macOS/iOS) query for
    // both A and AAAA records but only get an A response. The AAAA query times
    // out after ~5 seconds before falling back to IPv4, causing huge latency.
    esp_netif_t *sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (sta_netif) {
      mdns_netif_action(sta_netif, MDNS_EVENT_ENABLE_IP6);
      mdns_netif_action(sta_netif, MDNS_EVENT_ANNOUNCE_IP6);
    }
    esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (ap_netif) {
      mdns_netif_action(ap_netif, MDNS_EVENT_ENABLE_IP6);
      mdns_netif_action(ap_netif, MDNS_EVENT_ANNOUNCE_IP6);
    }
  } else {
    Serial.println("Error setting up mDNS responder!");
  }

  // Start DNS Server for Captive Portal (only in AP-only mode to avoid
  // interfering with normal DNS resolution on the external network)
  if (!networkConnected) {
    dnsServer.start(DNS_PORT, "*", myIP);
  }

  // Web Server Routes
  server.on("/", handleRoot);
  server.on("/cmd", handleCommandWeb);
  server.on("/control", handleControl);
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

  // Startup sequence: set all servos to 90 degrees (0 offset) one at a time
  // Order: L1, R1, L2, R2, L3, R3, L4, R4
  const uint8_t startupOrder[] = {L1, R1, L2, R2, L3, R3, L4, R4};
  for (int i = 0; i < 8; i++) {
    setServoAngle(startupOrder[i], 90);
    Serial.print(ServoNames[startupOrder[i]]); Serial.println(F("-90"));
    delay(500);
  }
  scheduleAllServoDetach();
  Serial.println(F("Startup sequence complete."));
}

void loop() {
  if (!networkConnected) {
    dnsServer.processNextRequest();
  }
  server.handleClient();
  checkServoDetach();

  // Read ADC every second - SHARP 2Y0A02 distance sensor
  if (millis() - lastAdcReadMs >= 1000) {
    lastAdcReadMs = millis();
    int adcValue = adc1_get_raw(ADC1_CHANNEL_2);
    float voltage = adcValue * 3.3 / 8191.0;
    float distCm = (voltage > 0.1) ? (60.0 / voltage) : 0;

    // Push to SSE stream if connected
    if (sensorStreamActive) {
      if (sensorClient.connected()) {
        sensorClient.print("{ \"distance\": ");
        sensorClient.print(distCm, 1);
        sensorClient.println(" }");
      } else {
        sensorStreamActive = false;
        Serial.println(F("Sensor stream disconnected"));
      }
    }
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
        if (strcmp(command_buffer, "subtrim") == 0 || strcmp(command_buffer, "st") == 0) {
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


void recordInput() {
  // Placeholder for input tracking (no display to update)
}
