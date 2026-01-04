# Flock You

<div style="text-align: center;">
    <img src="assets/images/flock.png" alt="Flock You" width="200" />
</div>

**Professional surveillance camera detection firmware for the Oui-Spy device available at [colonelpanic.tech](https://colonelpanic.tech)**

## What is Flock You?

Flock You is an advanced ESP32-S3 firmware designed to detect Flock Safety ALPR cameras, Raven gunshot detectors, and other surveillance devices using multiple detection methodologies. It provides real-time monitoring with audio alerts and comprehensive JSON output.

!!! info "Hardware"
    This firmware is designed for the **Oui-Spy device** available at [colonelpanic.tech](https://colonelpanic.tech), or any compatible Xiao ESP32-S3 board.

## Detection Capabilities

Flock You identifies surveillance devices through multiple methods:

- **WiFi Promiscuous Mode** - Captures probe requests and beacon frames
- **Bluetooth Low Energy Scanning** - Monitors BLE advertisements  
- **MAC Address Filtering** - Detects devices by known MAC prefixes
- **SSID Pattern Matching** - Identifies networks by specific names
- **Device Name Matching** - Detects BLE devices by advertised names
- **BLE Service UUID Detection** - Identifies Raven gunshot detectors by service UUIDs

## Key Features

| Feature | Description |
|---------|-------------|
| **Multi-Method Detection** | WiFi + BLE scanning for comprehensive coverage |
| **Audio Alerts** | Buzzer notifications for detections |
| **JSON Output** | Structured data via serial for integration |
| **Web Dashboard** | Real-time monitoring at `http://localhost:5000` |
| **Data Export** | CSV and KML export capabilities |
| **Heartbeat System** | Continuous alerts while device in range |

## Detected Device Types

| Device | Type | Detection Method |
|--------|------|------------------|
| **Flock Safety** | ALPR Cameras | WiFi SSID, MAC prefix |
| **Raven** | Gunshot Detectors | BLE Service UUIDs |
| **Penguin** | Surveillance | WiFi SSID, BLE name |
| **Pigvision** | Surveillance | WiFi SSID, BLE name |

## Quick Start

1. [Build and flash](building.md) the firmware to your Oui-Spy device
2. [Set up the web interface](web-interface.md) for real-time monitoring
3. Start detecting surveillance cameras!

## Documentation Sections

<div class="grid cards" markdown>

-   :material-hammer-wrench:{ .lg .middle } **Building**

    ---

    Flash the firmware to your ESP32-S3

    [:octicons-arrow-right-24: Build Guide](building.md)

-   :material-monitor-dashboard:{ .lg .middle } **Web Interface**

    ---

    Real-time detection dashboard

    [:octicons-arrow-right-24: Web Interface](web-interface.md)

-   :material-camera-iris:{ .lg .middle } **Detection Types**

    ---

    Supported surveillance systems

    [:octicons-arrow-right-24: Detection Types](detection-types.md)

-   :material-bluetooth:{ .lg .middle } **BLE Protocol**

    ---

    Bluetooth detection methodology

    [:octicons-arrow-right-24: BLE Protocol](ble-protocol.md)

</div>

## Audio Alert System

| Alert | Pattern | Meaning |
|-------|---------|---------|
| **Boot** | 2 beeps (low → high) | Device starting up |
| **Detection** | 3 fast high beeps | Surveillance device found |
| **Heartbeat** | 2 beeps every 10s | Device still in range |

## License

This project is part of the surveillance awareness ecosystem.

---

*Built for the Xiao ESP32-S3 with PlatformIO*
