# Sesame Robot

Quadruped robot with ESP32-S2 (Lolin S2 Mini), 8 MG90 servos, and SHARP distance sensor.

## Project Structure

- `firmware/sesame-firmware-bot/` — Active bot firmware (no face/animations)
- `firmware/sesame-firmware-main/` — Full-featured firmware with OLED face
- `firmware/debugging-firmware/` — Motor testing utilities
- `hardware/printing/scad/` — OpenSCAD parametric source files
- `hardware/printing/stl/` — Exported STL files for 3D printing
- `hardware/pcb/` — PCB designs (Distro Board v1/v2)

## Firmware

**Board:** Lolin S2 Mini (ESP32-S2) — Arduino framework
**Libraries:** ESP32Servo, WiFi, WebServer, DNSServer, ESPmDNS

### Servo Mapping (8 servos)

| Name | Function | GPIO | Leg |
|------|----------|------|-----|
| R1 | right front fwd/back | 1 | right_front |
| R2 | right back fwd/back | 2 | right_back |
| L1 | left front fwd/back | 4 | left_front |
| L2 | left back fwd/back | 6 | left_back |
| R4 | right back up/down | 8 | right_back |
| R3 | right front up/down | 10 | right_front |
| L3 | left front up/down | 13 | left_front |
| L4 | left back up/down | 14 | left_back |

### Direction Mapping (90 = neutral)

- **fwd/back:** Left side `fwd = 90-angle`, Right side `fwd = 90+angle`
- **up/down:** `left_front` & `right_back` use `up = 90+angle`; `right_front` & `left_back` use `up = 90-angle`

### Key Endpoints

- `GET /control?leg=<leg>&direction=<dir>&angle=<0-90>` — Leg-based control
- `GET /cmd?servoBtn=<name>&angle=<0-180>` — Direct servo control
- `GET /sensors` — SSE stream for distance sensor
- `POST /api/command` — JSON API `{"command":"L1-90"}`

### WiFi

- AP: `Sesame-Controller-BETA` / `12345678`
- mDNS: `http://sesame-robot.local`

## Hardware (3D Printing)

- OpenSCAD binary: `/Applications/OpenSCAD-2021.01.app/Contents/MacOS/OpenSCAD`
- All parts designed for supportless FDM printing
- Use `/design` command for parametric 3D modeling tasks

## Commands

- `/design` — OpenSCAD parametric 3D modeling (create/modify parts)
- `/program` — Update firmware (edit, compile, flash)
- `/walk` — Execute walking gait
- `/stand` — Move to standing position
