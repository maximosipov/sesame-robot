# Program — Update Sesame Robot Firmware

You are updating the firmware for a quadruped robot running on an ESP32-S2 (Lolin S2 Mini). The active firmware is the "bot" variant (no face/animations).

## Task

$ARGUMENTS

## Firmware Location

- Main firmware: `firmware/sesame-firmware-bot/sesame-firmware-bot.ino`
- Movement header: `firmware/sesame-firmware-bot/movement-bot.h`
- Web UI (captive portal): `firmware/sesame-firmware-bot/captive-portal-bot.h`
- Full-featured variant (reference only): `firmware/sesame-firmware-main/`

## Instructions

1. **Read the relevant firmware files** before making changes — understand existing code structure
2. **Make targeted edits** — keep changes minimal and focused on the task
3. **Preserve existing functionality** — don't break working endpoints, servo mapping, or WiFi config
4. **Test-ready code** — the user will compile and flash manually via Arduino IDE

## Architecture Overview

### Hardware
- **MCU:** ESP32-S2 (Lolin S2 Mini)
- **8 MG90 servos** on GPIOs: 1, 2, 4, 6, 8, 10, 13, 14
- **SHARP 2Y0A02 distance sensor** on GPIO 3 (ADC)
- **PWM:** 50Hz, 732-2929us pulse width (0-180 degrees)

### Servo Layout
```
Servo  | Function              | GPIO | Enum
-------|----------------------|------|-----
R1     | right front fwd/back | 1    | R1
R2     | right back fwd/back  | 2    | R2
L1     | left front fwd/back  | 4    | L1
L2     | left back fwd/back   | 6    | L2
R4     | right back up/down   | 8    | R4
R3     | right front up/down  | 10   | R3
L3     | left front up/down   | 13   | L3
L4     | left back up/down    | 14   | L4
```

### Direction Mapping (90 = neutral center)
- **fwd/back:** Left side `fwd = 90-angle`, Right side `fwd = 90+angle`
- **up/down:** `left_front` & `right_back` → `up = 90+angle`; `right_front` & `left_back` → `up = 90-angle`

### Web Server Endpoints
| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/` | GET | Captive portal web UI |
| `/cmd` | GET | Direct servo control (`servoBtn`+`angle` or `motor`+`value`) |
| `/control` | GET | Leg-based control (`leg`+`direction`+`angle`) |
| `/sensors` | GET | SSE stream for distance sensor |
| `/getSettings` | GET | Get motor delay config |
| `/setSettings` | GET | Set motor delay config |
| `/api/status` | GET | JSON status (WiFi, IPs) |
| `/api/command` | POST | JSON command API (`{"command":"L1-90"}`) |

### WiFi
- **AP mode:** SSID `Sesame-Controller-BETA`, password `12345678`
- **Station mode:** Optional, connects to home WiFi when `ENABLE_NETWORK_MODE=true`
- **mDNS:** `sesame-robot.local`

### Key Implementation Details
- Servos auto-detach after 2 seconds of inactivity (saves power, reduces jitter)
- `serviceDelay()` is a non-blocking delay that keeps servicing web requests
- `motorCurrentDelay` (default 20ms) prevents brownout when moving servos
- Subtrim system for per-servo calibration offsets
- Serial CLI supports commands like `L1-90`, `all 90`, `subtrim`

## Libraries Required (Arduino IDE)
- `ESP32Servo` by Kevin Harrington
- Built-in: `WiFi`, `WebServer`, `DNSServer`, `ESPmDNS`
- Built-in: `driver/adc.h` (ESP-IDF ADC driver)

## Coding Conventions
- Use `F()` macro for string literals in `Serial.print()` to save RAM
- Use `uint8_t` for servo channel indices
- Validate all web input (angle ranges, servo names)
- Return JSON responses from API endpoints
- Use `constrain()` for angle bounds
- Keep the `recordInput()` call after any servo movement for future extensibility
- HTML/CSS/JS for the web UI lives entirely in `captive-portal-bot.h` as a raw string literal
