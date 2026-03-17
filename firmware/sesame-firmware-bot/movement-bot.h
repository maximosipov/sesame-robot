#pragma once

#include <Arduino.h>

enum ServoName : uint8_t {
  R1 = 0,
  R2 = 1,
  L1 = 2,
  L2 = 3,
  R4 = 4,
  R3 = 5,
  L3 = 6,
  L4 = 7
};

const String ServoNames[] = {"R1","R2","L1","L2","R4","R3","L3","L4"};

inline int servoNameToIndex(String servo) {
  if (servo == "R1") return R1;
  if (servo == "R2") return R2;
  if (servo == "L1") return L1;
  if (servo == "L2") return L2;
  if (servo == "R4") return R4;
  if (servo == "R3") return R3;
  if (servo == "L3") return L3;
  if (servo == "L4") return L4;
  return -1;
}

// External globals and helpers
extern int frameDelay;
extern int walkCycles;
extern String currentCommand;

extern void setServoAngle(uint8_t channel, int angle);
extern void scheduleServoDetach(uint8_t channel);
extern void scheduleAllServoDetach();
extern void serviceDelay(unsigned long ms);
extern bool pressingCheck(String cmd, int ms);

// Prototypes
void runStandPose();
void runServoToAngle(uint8_t servoIdx, int angle);
void runWalkPose();
void runWalkBackward();
void runTurnLeft();
void runTurnRight();

// ====== STAND (used as neutral position for walk start/end) ======
inline void runStandPose() {
  Serial.println(F("STAND"));
  setServoAngle(R1, 135);
  setServoAngle(R2, 45);
  setServoAngle(L1, 45);
  setServoAngle(L2, 135);
  setServoAngle(R4, 0);
  setServoAngle(R3, 180);
  setServoAngle(L3, 0);
  setServoAngle(L4, 180);
  scheduleAllServoDetach();
}

// --- SERVO BUTTON CONTROLS ---
inline void runServoToAngle(uint8_t servoIdx, int angle) {
  if (servoIdx < 8 && angle >= 0 && angle <= 180) {
    setServoAngle(servoIdx, angle);
    scheduleServoDetach(servoIdx);
    Serial.print(ServoNames[servoIdx]); Serial.print(F("-")); Serial.println(angle);
  }
}

// --- MOVEMENT ---
inline void runWalkPose() {
  Serial.println(F("WALK FWD"));
  // Initial Step
  setServoAngle(R3, 135); setServoAngle(L3, 45);
  setServoAngle(R2, 100); setServoAngle(L1, 25);
  if (!pressingCheck("forward", frameDelay)) return;

  for (int i = 0; i < walkCycles; i++) {
    setServoAngle(R3, 135); setServoAngle(L3, 0);
    if (!pressingCheck("forward", frameDelay)) return;
    setServoAngle(L4, 135); setServoAngle(L2, 90);
    setServoAngle(R4, 0); setServoAngle(R1, 180);
    if (!pressingCheck("forward", frameDelay)) return;
    setServoAngle(R2, 45); setServoAngle(L1, 90);
    if (!pressingCheck("forward", frameDelay)) return;
    setServoAngle(R4, 45); setServoAngle(L4, 180);
    if (!pressingCheck("forward", frameDelay)) return;
    setServoAngle(R3, 180); setServoAngle(L3, 45);
    setServoAngle(R2, 90); setServoAngle(L1, 0);
    if (!pressingCheck("forward", frameDelay)) return;
    setServoAngle(L2, 135); setServoAngle(R1, 90);
    if (!pressingCheck("forward", frameDelay)) return;
  }
  runStandPose();
}

inline void runWalkBackward() {
  Serial.println(F("WALK BACK"));
  if (!pressingCheck("backward", frameDelay)) return;

  for (int i = 0; i < walkCycles; i++) {
    setServoAngle(R3, 135); setServoAngle(L3, 0);
    if (!pressingCheck("backward", frameDelay)) return;
    setServoAngle(L4, 135); setServoAngle(L2, 135);
    setServoAngle(R4, 0); setServoAngle(R1, 90);
    if (!pressingCheck("backward", frameDelay)) return;
    setServoAngle(R2, 90); setServoAngle(L1, 0);
    if (!pressingCheck("backward", frameDelay)) return;
    setServoAngle(R4, 45); setServoAngle(L4, 180);
    if (!pressingCheck("backward", frameDelay)) return;
    setServoAngle(R3, 180); setServoAngle(L3, 45);
    setServoAngle(R2, 45); setServoAngle(L1, 90);
    if (!pressingCheck("backward", frameDelay)) return;
    setServoAngle(L2, 90); setServoAngle(R1, 180);
    if (!pressingCheck("backward", frameDelay)) return;
  }
  runStandPose();
}

inline void runTurnLeft() {
  Serial.println(F("TURN LEFT"));
  for (int i = 0; i < walkCycles; i++) {
    setServoAngle(R3, 135); setServoAngle(L4, 135);
    if (!pressingCheck("left", frameDelay)) return;
    setServoAngle(R1, 180); setServoAngle(L2, 180);
    if (!pressingCheck("left", frameDelay)) return;
    setServoAngle(R3, 180); setServoAngle(L4, 180);
    if (!pressingCheck("left", frameDelay)) return;
    setServoAngle(R1, 135); setServoAngle(L2, 135);
    if (!pressingCheck("left", frameDelay)) return;
    setServoAngle(R4, 45); setServoAngle(L3, 45);
    if (!pressingCheck("left", frameDelay)) return;
    setServoAngle(R2, 90); setServoAngle(L1, 90);
    if (!pressingCheck("left", frameDelay)) return;
    setServoAngle(R4, 0); setServoAngle(L3, 0);
    if (!pressingCheck("left", frameDelay)) return;
    setServoAngle(R2, 45); setServoAngle(L1, 45);
    if (!pressingCheck("left", frameDelay)) return;
  }
  runStandPose();
}

inline void runTurnRight() {
  Serial.println(F("TURN RIGHT"));
  for (int i = 0; i < walkCycles; i++) {
    setServoAngle(R4, 45); setServoAngle(L3, 45);
    if (!pressingCheck("right", frameDelay)) return;
    setServoAngle(R2, 0); setServoAngle(L1, 0);
    if (!pressingCheck("right", frameDelay)) return;
    setServoAngle(R4, 0); setServoAngle(L3, 0);
    if (!pressingCheck("right", frameDelay)) return;
    setServoAngle(R2, 45); setServoAngle(L1, 45);
    if (!pressingCheck("right", frameDelay)) return;
    setServoAngle(R3, 135); setServoAngle(L4, 135);
    if (!pressingCheck("right", frameDelay)) return;
    setServoAngle(R1, 90); setServoAngle(L2, 90);
    if (!pressingCheck("right", frameDelay)) return;
    setServoAngle(R3, 180); setServoAngle(L4, 180);
    if (!pressingCheck("right", frameDelay)) return;
    setServoAngle(R1, 135); setServoAngle(L2, 135);
    if (!pressingCheck("right", frameDelay)) return;
  }
  runStandPose();
}
