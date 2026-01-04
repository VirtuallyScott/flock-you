# Related Projects

Flock You is part of the OUI-SPY ecosystem of ESP32-S3 BLE detection tools.

## OUI-SPY Family

| Firmware | Repository | Use Case |
|----------|------------|----------|
| **Flock You** | [flock-you](https://github.com/VirtuallyScott/flock-you) | Surveillance camera & gunshot detector detection |
| **OUI-SPY Detector** | [ouispy-detector](https://github.com/colonelpanichacks/ouispy-detector) | Multi-target BLE scanner with OUI filtering |
| **OUI-SPY Foxhunter** | [ouispy-foxhunter](https://github.com/colonelpanichacks/ouispy-foxhunter) | Single-target precision proximity tracker |
| **OUI-SPY UniPwn** | [Oui-Spy-UniPwn](https://github.com/colonelpanichacks/Oui-Spy-UniPwn) | Unitree robot security research |
| **Sky-Spy** | [Sky-Spy](https://github.com/colonelpanichacks/Sky-Spy) | Drone RemoteID detection & mapping |

## Companion Apps

| App | Platform | Description |
|-----|----------|-------------|
| **FlockFinder** | iOS | [FlockFinder iOS App](https://github.com/VirtuallyScott/FlockFinderApp) - BLE companion for FeatherS3 |

## Research & Data Sources

### DeFlock

[DeFlock](https://deflock.me) is a crowdsourced ALPR location and reporting tool.

- GitHub: [FoggedLens/deflock](https://github.com/FoggedLens/deflock)
- Provides comprehensive datasets and methodologies for surveillance device detection
- Real-world device signatures included in the `datasets/` folder

### GainSec

[GainSec](https://github.com/GainSec) provides OSINT and privacy research.

- Specialized in surveillance technology analysis
- Contributed `raven_configurations.json` - verified BLE service UUIDs from SoundThinking/ShotSpotter Raven devices
- Enables detection of acoustic gunshot detection devices

## Hardware References

### ESP32-S3 Documentation

- [ESP32-S3 Getting Started](https://esp32s3.com/getting-started.html)
- [Unexpected Maker ESP32-S3 GitHub](https://github.com/unexpectedmaker/esp32s3)
- [ESP32-S3 Pinout Cards](https://github.com/unexpectedmaker/esp32s3/tree/main/Pinout%20Cards)

### Development Tools

- [PlatformIO Documentation](https://docs.platformio.org/)
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32)
- [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino)

### OUI Reference

- [IEEE OUI Lookup](https://standards.ieee.org/products-services/regauth/oui/index.html)
- [OUI List for OUI-SPY](https://github.com/colonelpanichacks/ouispy-detector/blob/main/ouis.md)

## Acknowledgments

Special thanks to the researchers and contributors who have made this work possible:

- **GainSec** - Raven BLE service UUID dataset for SoundThinking/ShotSpotter detection
- **DeFlock** - Crowdsourced surveillance camera location data and detection methodologies
- **Colonel Panic** - Oui-Spy hardware and ecosystem development
- The broader surveillance detection community for continued research and privacy protection efforts
