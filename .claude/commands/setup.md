# Setup — Verify and Install Sesame Robot Development Environment

Check that all tools needed for `/design` and `/program` commands are available. Install anything missing. Report status at the end.

## Checks to Perform

Run all checks in parallel where possible, then install missing pieces.

### 1. OpenSCAD (for /design)

**Check:** Run `/Applications/OpenSCAD-2021.01.app/Contents/MacOS/OpenSCAD --version`

**If missing:** Install via Homebrew:
```bash
brew install --cask openscad
```
Then locate the binary path and report it.

### 2. VSCode Extensions (for /design and /program)

**Check:** Run `code --list-extensions` and look for:

- `antyos.openscad` — OpenSCAD syntax highlighting
- `slevesque.vscode-3dviewer` — STL preview
- `platformio.platformio-ide` — PlatformIO for firmware builds

**If missing:** Install each missing extension:

```bash
code --install-extension antyos.openscad
code --install-extension slevesque.vscode-3dviewer
code --install-extension platformio.platformio-ide
```

### 3. PlatformIO CLI (for /program)

**Check:** Run `~/.platformio/penv/bin/pio --version`

**If missing:** PlatformIO CLI installs automatically with the VSCode extension. If the extension is installed but CLI is missing:

```bash
curl -fsSL -o /tmp/get-platformio.py https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py
python3 /tmp/get-platformio.py
```

### 4. ESP32-S2 Platform (for /program)

**Check:** Run `~/.platformio/penv/bin/pio pkg list -g -p` and look for `espressif32`

If not installed globally, check if `platformio.ini` exists in the project. If neither:

**Install platform and create platformio.ini:**

```bash
~/.platformio/penv/bin/pio pkg install -g -p "platformio/espressif32"
```

Then create `firmware/sesame-firmware-bot/platformio.ini`:

```ini
[env:lolin_s2_mini]
platform = espressif32
board = lolin_s2_mini
framework = arduino
lib_deps =
    madhephaestus/ESP32Servo@^3.0.6
monitor_speed = 115200
upload_protocol = esptool
```

### 5. Network Connectivity to Robot (for /walk, /stand)

**Check:** Run `curl -s --connect-timeout 2 http://sesame-robot.local/api/status`

**If fails:** This is not a setup issue — the robot may be off or on a different network. Just report the status; don't try to fix it.

## Output

Print a summary table:

```
Sesame Robot Dev Environment
============================
OpenSCAD ............. [OK/MISSING]
VSCode: openscad ..... [OK/MISSING]
VSCode: 3dviewer ..... [OK/MISSING]
VSCode: platformio ... [OK/MISSING]
PlatformIO CLI ....... [OK/MISSING]
ESP32-S2 platform .... [OK/MISSING]
platformio.ini ....... [OK/CREATED/MISSING]
Robot connection ..... [OK/OFFLINE]
```

If everything is OK, say "Ready to go." If anything was installed, list what was installed.
