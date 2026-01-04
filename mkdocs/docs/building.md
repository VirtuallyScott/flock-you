# Building Flock You

This guide covers building and flashing the Flock You firmware to your ESP32-S3 device.

## Prerequisites

| Requirement | Description |
|-------------|-------------|
| **PlatformIO** | IDE or CLI |
| **USB-C Cable** | Data-capable cable for programming |
| **Python** | 3.8+ (PlatformIO dependency) |

!!! tip "PlatformIO Installation"
    Install PlatformIO as a VS Code extension or use the CLI: `pip install platformio`

## Supported Hardware

| Board | Features | Build Target |
|-------|----------|--------------|
| **Oui-Spy** | Xiao ESP32-S3 + Buzzer | `xiao_esp32s3` |
| **UM FeatherS3** | RGB LED + iOS App BLE | `um_feathers3` |
| **Xiao ESP32-S3** | DIY buzzer setup | `xiao_esp32s3` |
| **Xiao ESP32-C3** | DIY buzzer setup | `xiao_esp32c3` |

## Clone the Repository

```bash
git clone https://github.com/VirtuallyScott/flock-you.git
cd flock-you
```

## Hardware Setup

### Option 1: Oui-Spy Device (Recommended)

The Oui-Spy device from [colonelpanic.tech](https://colonelpanic.tech) comes pre-configured with:

- Xiao ESP32-S3 microcontroller
- Built-in buzzer system
- USB-C connectivity

Simply connect via USB-C and proceed to flashing.

### Option 2: Unexpected Maker FeatherS3

The FeatherS3 provides RGB LED alerts and iOS app connectivity:

- ESP32-S3 with 16MB Flash, 8MB PSRAM
- Built-in RGB NeoPixel LED
- BLE GATT server for FlockFinder iOS app

No additional wiring required.

### Option 3: DIY Xiao ESP32-S3 Setup

Wire a 3V buzzer to your board:

```
Xiao ESP32 S3    Buzzer
GPIO3 (D2)  ---> Positive (+)
GND         ---> Negative (-)
```

## ESP32-S3 Boot Modes

### DOWNLOAD Mode (Required for First Flash)

When flashing from CircuitPython or first-time Arduino flash:

1. Press and hold **[BOOT]** button
2. Press and release **[RESET]** button
3. Release **[BOOT]** button
4. A new serial device will appear

!!! warning "First-time Flash"
    If your board came with CircuitPython, you **must** use DOWNLOAD mode for the initial Arduino/PlatformIO flash.

### UF2 Bootloader Mode (CircuitPython Only)

For CircuitPython updates via drag-and-drop:

1. Press and release **[RESET]**
2. When RGB LED turns purple, press and release **[BOOT]**
3. RGB LED turns green when filesystem mounts
4. Drag-and-drop UF2 file to mounted drive

## Build and Flash

### Choose Your Board

```bash
# For Unexpected Maker FeatherS3 (RGB LED + iOS app)
pio run -e um_feathers3 --target upload

# For Oui-Spy / Xiao ESP32-S3 (buzzer)
pio run -e xiao_esp32s3 --target upload

# For Xiao ESP32-C3 (buzzer)
pio run -e xiao_esp32c3 --target upload
```

### Using PlatformIO IDE (VS Code)

1. Open the `flock-you` folder in VS Code
2. Click the PlatformIO icon in the sidebar
3. Select your environment (`um_feathers3`, `xiao_esp32s3`, etc.)
4. Click **Upload** under Project Tasks

!!! note "ESP32-S3 Reset Bug"
    After upload completes, you may need to press **[RESET]** manually. This is a known ESP32-S3 silicon limitation.

## Monitor Serial Output

View detection data in real-time:

```bash
pio device monitor
```

You'll see JSON output for each detection:

```json
{"type":"Flock Safety","mac":"3C:71:BF:12:34:56","rssi":-62}
```

## PlatformIO Configuration

The `platformio.ini` includes board-specific settings:

```ini
[env:um_feathers3]
platform = espressif32
board = um_feathers3
framework = arduino

build_flags = 
    -D ARDUINO_USB_MODE=1
    -D ARDUINO_USB_CDC_ON_BOOT=1

monitor_speed = 115200
```

## Firmware Configuration

Key settings in `src/main.cpp`:

| Setting | Default | Description |
|---------|---------|-------------|
| `BUZZER_PIN` | 3 | GPIO pin for buzzer (Oui-Spy/Xiao) |
| `BLE_SCAN_DURATION` | 1s | BLE scan window |
| `BLE_SCAN_INTERVAL` | 5000ms | Time between BLE scans |
| `CHANNEL_HOP_INTERVAL` | 500ms | WiFi channel hop rate |

## Troubleshooting

### No Serial Port Appears

| Cause | Solution |
|-------|----------|
| Charge-only cable | Use a data-capable USB-C cable |
| Wrong boot mode | Enter DOWNLOAD mode (see above) |
| Missing USB driver | ESP32-S3 uses native USB-CDC, no driver needed |

### Upload Fails

| Error | Solution |
|-------|----------|
| "No serial port" | Enter DOWNLOAD mode before upload |
| "Timed out" | Press RESET after upload, try again |
| "Wrong board" | Verify correct environment in platformio.ini |

### No WiFi AP / No Detections

| Issue | Solution |
|-------|----------|
| AP doesn't appear | Wait 10-15 seconds, check serial for errors |
| No BLE detections | Verify device is in range, check scan output |
| No audio alerts | Check buzzer wiring (GPIO3 → buzzer+, GND → buzzer-) |

### FeatherS3 RGB LED Not Working

| Issue | Solution |
|-------|----------|
| No LED activity | Verify correct build target (`um_feathers3`) |
| Wrong colors | Check GPIO40 (data) and GPIO39 (power) assignments |

## Verification Checklist

- [ ] USB-C connection establishes serial port
- [ ] Board enters DOWNLOAD mode correctly
- [ ] Firmware uploads successfully
- [ ] Serial output visible at 115200 baud
- [ ] Boot sequence plays (beeps or LED flash)
- [ ] Scanning activity in serial output

## Next Steps

- [Set up the web interface](web-interface.md) for real-time monitoring
- [Learn about detection types](detection-types.md)
- [View project architecture](architecture.md)
