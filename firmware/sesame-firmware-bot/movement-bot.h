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
extern void setServoAngle(uint8_t channel, int angle);
extern void scheduleServoDetach(uint8_t channel);
extern void scheduleAllServoDetach();

// Prototypes
void runStandPose();
void runServoToAngle(uint8_t servoIdx, int angle);

// ====== STAND ======
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
