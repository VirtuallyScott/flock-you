#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <vector>
#include <string>

// ============================================================================
// DYNAMIC CONFIGURATION MANAGER FOR FLOCKFINDER
// ============================================================================
// This module manages detection patterns received from the iOS app.
// Patterns are stored in NVS flash for persistence across reboots.

// Configuration Version - increment when format changes
#define CONFIG_VERSION 1

// Storage namespace for NVS
#define CONFIG_NAMESPACE "flockconfig"

// Maximum pattern limits
#define MAX_SSID_PATTERNS 30
#define MAX_MAC_PREFIXES 100
#define MAX_BLE_NAMES 30
#define MAX_BLE_UUIDS 20

// Default scan configuration
#define DEFAULT_BLE_SCAN_DURATION 1
#define DEFAULT_BLE_SCAN_INTERVAL 5000
#define DEFAULT_CHANNEL_HOP_INTERVAL 500
#define DEFAULT_MAX_CHANNEL 13
#define DEFAULT_HEARTBEAT_INTERVAL 10000
#define DEFAULT_DETECTION_TIMEOUT 30000

// ============================================================================
// DATA STRUCTURES
// ============================================================================

struct SsidPattern {
    String pattern;
    String deviceType;
    bool caseSensitive;
    bool enabled;
};

struct MacPrefix {
    String prefix;  // Format: "xx:xx:xx"
    String deviceType;
    bool enabled;
};

struct BleNamePattern {
    String pattern;
    String deviceType;
    bool enabled;
};

struct BleUuidPattern {
    String uuid;
    String deviceType;
    bool enabled;
};

// Stream mode options
enum StreamMode {
    STREAM_ALL = 0,          // Stream all scanned devices
    STREAM_MATCHES_ONLY = 1  // Only stream devices matching patterns
};

struct ScanConfig {
    // Scan enable flags
    bool wifiScanEnabled;         // Enable WiFi promiscuous scanning
    bool bleScanEnabled;          // Enable BLE scanning
    
    // Scan timing
    uint8_t bleScanDuration;      // seconds
    uint16_t bleScanInterval;     // milliseconds between scans
    uint16_t channelHopInterval;  // milliseconds
    uint8_t maxChannel;           // max WiFi channel (13 or 14) - Note: ESP32 only supports 2.4GHz
    uint16_t heartbeatInterval;   // milliseconds
    uint32_t detectionTimeout;    // milliseconds
    
    // Stream configuration
    StreamMode streamMode;        // What to stream to iOS app
};

// ============================================================================
// CONFIGURATION MANAGER CLASS
// ============================================================================

class ConfigManager {
public:
    // Singleton instance
    static ConfigManager& getInstance() {
        static ConfigManager instance;
        return instance;
    }

    // Initialize - load from NVS or use defaults
    void begin() {
        Serial.println("[ConfigManager] Initializing...");
        loadDefaults();
        
        // Try to load from NVS
        if (!loadFromNVS()) {
            Serial.println("[ConfigManager] No saved config, using defaults");
            saveToNVS();  // Save defaults
        }
        
        printConfig();
    }

    // ========== Scan Configuration ==========
    
    ScanConfig& getScanConfig() { return scanConfig; }
    
    void setScanConfig(const ScanConfig& config) {
        scanConfig = config;
    }

    // ========== SSID Patterns ==========
    
    std::vector<SsidPattern>& getSsidPatterns() { return ssidPatterns; }
    
    void clearSsidPatterns() { ssidPatterns.clear(); }
    
    bool addSsidPattern(const String& pattern, const String& deviceType, bool caseSensitive = false, bool enabled = true) {
        if (ssidPatterns.size() >= MAX_SSID_PATTERNS) return false;
        ssidPatterns.push_back({pattern, deviceType, caseSensitive, enabled});
        return true;
    }

    bool checkSsidMatch(const char* ssid, String& deviceTypeOut) {
        if (!ssid) return false;
        
        for (const auto& p : ssidPatterns) {
            if (!p.enabled) continue;
            
            bool match = false;
            if (p.caseSensitive) {
                match = (strstr(ssid, p.pattern.c_str()) != nullptr);
            } else {
                match = (strcasestr(ssid, p.pattern.c_str()) != nullptr);
            }
            
            if (match) {
                deviceTypeOut = p.deviceType;
                return true;
            }
        }
        return false;
    }

    // ========== MAC Prefixes ==========
    
    std::vector<MacPrefix>& getMacPrefixes() { return macPrefixes; }
    
    void clearMacPrefixes() { macPrefixes.clear(); }
    
    bool addMacPrefix(const String& prefix, const String& deviceType, bool enabled = true) {
        if (macPrefixes.size() >= MAX_MAC_PREFIXES) return false;
        macPrefixes.push_back({prefix, deviceType, enabled});
        return true;
    }

    bool checkMacMatch(const uint8_t* mac, String& deviceTypeOut) {
        if (!mac) return false;
        
        char macStr[9];
        snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x", mac[0], mac[1], mac[2]);
        
        for (const auto& p : macPrefixes) {
            if (!p.enabled) continue;
            
            if (strncasecmp(macStr, p.prefix.c_str(), 8) == 0) {
                deviceTypeOut = p.deviceType;
                return true;
            }
        }
        return false;
    }
    
    bool checkMacMatch(const char* macStr, String& deviceTypeOut) {
        if (!macStr) return false;
        
        for (const auto& p : macPrefixes) {
            if (!p.enabled) continue;
            
            if (strncasecmp(macStr, p.prefix.c_str(), 8) == 0) {
                deviceTypeOut = p.deviceType;
                return true;
            }
        }
        return false;
    }

    // ========== BLE Name Patterns ==========
    
    std::vector<BleNamePattern>& getBleNamePatterns() { return bleNamePatterns; }
    
    void clearBleNamePatterns() { bleNamePatterns.clear(); }
    
    bool addBleNamePattern(const String& pattern, const String& deviceType, bool enabled = true) {
        if (bleNamePatterns.size() >= MAX_BLE_NAMES) return false;
        bleNamePatterns.push_back({pattern, deviceType, enabled});
        return true;
    }

    bool checkBleNameMatch(const char* name, String& deviceTypeOut) {
        if (!name) return false;
        
        for (const auto& p : bleNamePatterns) {
            if (!p.enabled) continue;
            
            if (strcasestr(name, p.pattern.c_str()) != nullptr) {
                deviceTypeOut = p.deviceType;
                return true;
            }
        }
        return false;
    }

    // ========== BLE UUID Patterns ==========
    
    std::vector<BleUuidPattern>& getBleUuidPatterns() { return bleUuidPatterns; }
    
    void clearBleUuidPatterns() { bleUuidPatterns.clear(); }
    
    bool addBleUuidPattern(const String& uuid, const String& deviceType, bool enabled = true) {
        if (bleUuidPatterns.size() >= MAX_BLE_UUIDS) return false;
        bleUuidPatterns.push_back({uuid, deviceType, enabled});
        return true;
    }

    bool checkBleUuidMatch(const char* uuid, String& deviceTypeOut) {
        if (!uuid) return false;
        
        for (const auto& p : bleUuidPatterns) {
            if (!p.enabled) continue;
            
            if (strcasecmp(uuid, p.uuid.c_str()) == 0) {
                deviceTypeOut = p.deviceType;
                return true;
            }
        }
        return false;
    }

    // ========== Configuration Serialization ==========
    
    // Serialize current config to JSON for sending to iOS app
    String toJson() {
        DynamicJsonDocument doc(8192);
        
        doc["version"] = CONFIG_VERSION;
        
        // Scan config
        JsonObject scan = doc.createNestedObject("scanConfig");
        scan["wifiScanEnabled"] = scanConfig.wifiScanEnabled;
        scan["bleScanEnabled"] = scanConfig.bleScanEnabled;
        scan["bleScanDuration"] = scanConfig.bleScanDuration;
        scan["bleScanInterval"] = scanConfig.bleScanInterval;
        scan["channelHopInterval"] = scanConfig.channelHopInterval;
        scan["maxChannel"] = scanConfig.maxChannel;
        scan["heartbeatInterval"] = scanConfig.heartbeatInterval;
        scan["detectionTimeout"] = scanConfig.detectionTimeout;
        scan["streamMode"] = static_cast<int>(scanConfig.streamMode);
        
        // Patterns
        JsonObject patterns = doc.createNestedObject("patterns");
        
        // SSID patterns
        JsonArray ssidArr = patterns.createNestedArray("ssidPatterns");
        for (const auto& p : ssidPatterns) {
            JsonObject obj = ssidArr.createNestedObject();
            obj["pattern"] = p.pattern;
            obj["deviceType"] = p.deviceType;
            obj["caseSensitive"] = p.caseSensitive;
            obj["enabled"] = p.enabled;
        }
        
        // MAC prefixes
        JsonArray macArr = patterns.createNestedArray("macPrefixes");
        for (const auto& p : macPrefixes) {
            JsonObject obj = macArr.createNestedObject();
            obj["prefix"] = p.prefix;
            obj["deviceType"] = p.deviceType;
            obj["enabled"] = p.enabled;
        }
        
        // BLE names
        JsonArray bleNameArr = patterns.createNestedArray("bleDeviceNames");
        for (const auto& p : bleNamePatterns) {
            JsonObject obj = bleNameArr.createNestedObject();
            obj["pattern"] = p.pattern;
            obj["deviceType"] = p.deviceType;
            obj["enabled"] = p.enabled;
        }
        
        // BLE UUIDs
        JsonArray bleUuidArr = patterns.createNestedArray("bleServiceUuids");
        for (const auto& p : bleUuidPatterns) {
            JsonObject obj = bleUuidArr.createNestedObject();
            obj["uuid"] = p.uuid;
            obj["deviceType"] = p.deviceType;
            obj["enabled"] = p.enabled;
        }
        
        String output;
        serializeJson(doc, output);
        return output;
    }

    // Parse JSON config from iOS app
    bool fromJson(const char* json) {
        DynamicJsonDocument doc(8192);
        DeserializationError error = deserializeJson(doc, json);
        
        if (error) {
            Serial.printf("[ConfigManager] JSON parse error: %s\n", error.c_str());
            return false;
        }
        
        // Check version
        int version = doc["version"] | 0;
        if (version != CONFIG_VERSION) {
            Serial.printf("[ConfigManager] Config version mismatch: %d vs %d\n", version, CONFIG_VERSION);
            // Could handle migration here
        }
        
        // Parse scan config
        if (doc.containsKey("scanConfig")) {
            JsonObject scan = doc["scanConfig"];
            scanConfig.wifiScanEnabled = scan["wifiScanEnabled"] | true;
            scanConfig.bleScanEnabled = scan["bleScanEnabled"] | true;
            scanConfig.bleScanDuration = scan["bleScanDuration"] | DEFAULT_BLE_SCAN_DURATION;
            scanConfig.bleScanInterval = scan["bleScanInterval"] | DEFAULT_BLE_SCAN_INTERVAL;
            scanConfig.channelHopInterval = scan["channelHopInterval"] | DEFAULT_CHANNEL_HOP_INTERVAL;
            scanConfig.maxChannel = scan["maxChannel"] | DEFAULT_MAX_CHANNEL;
            scanConfig.heartbeatInterval = scan["heartbeatInterval"] | DEFAULT_HEARTBEAT_INTERVAL;
            scanConfig.detectionTimeout = scan["detectionTimeout"] | DEFAULT_DETECTION_TIMEOUT;
            scanConfig.streamMode = static_cast<StreamMode>(scan["streamMode"] | 0);
        }
        
        // Parse patterns
        if (doc.containsKey("patterns")) {
            JsonObject patterns = doc["patterns"];
            
            // SSID patterns
            if (patterns.containsKey("ssidPatterns")) {
                clearSsidPatterns();
                JsonArray arr = patterns["ssidPatterns"];
                for (JsonObject obj : arr) {
                    addSsidPattern(
                        obj["pattern"] | "",
                        obj["deviceType"] | "Unknown",
                        obj["caseSensitive"] | false,
                        obj["enabled"] | true
                    );
                }
            }
            
            // MAC prefixes
            if (patterns.containsKey("macPrefixes")) {
                clearMacPrefixes();
                JsonArray arr = patterns["macPrefixes"];
                for (JsonObject obj : arr) {
                    addMacPrefix(
                        obj["prefix"] | "",
                        obj["deviceType"] | "Unknown",
                        obj["enabled"] | true
                    );
                }
            }
            
            // BLE names
            if (patterns.containsKey("bleDeviceNames")) {
                clearBleNamePatterns();
                JsonArray arr = patterns["bleDeviceNames"];
                for (JsonObject obj : arr) {
                    addBleNamePattern(
                        obj["pattern"] | "",
                        obj["deviceType"] | "Unknown",
                        obj["enabled"] | true
                    );
                }
            }
            
            // BLE UUIDs
            if (patterns.containsKey("bleServiceUuids")) {
                clearBleUuidPatterns();
                JsonArray arr = patterns["bleServiceUuids"];
                for (JsonObject obj : arr) {
                    addBleUuidPattern(
                        obj["uuid"] | "",
                        obj["deviceType"] | "Unknown",
                        obj["enabled"] | true
                    );
                }
            }
        }
        
        Serial.println("[ConfigManager] Configuration updated from JSON");
        printConfig();
        return true;
    }

    // ========== NVS Persistence ==========
    
    bool saveToNVS() {
        Preferences prefs;
        if (!prefs.begin(CONFIG_NAMESPACE, false)) {
            Serial.println("[ConfigManager] Failed to open NVS");
            return false;
        }
        
        String json = toJson();
        prefs.putString("config", json);
        prefs.putInt("version", CONFIG_VERSION);
        prefs.end();
        
        Serial.println("[ConfigManager] Configuration saved to NVS");
        return true;
    }

    bool loadFromNVS() {
        Preferences prefs;
        if (!prefs.begin(CONFIG_NAMESPACE, true)) {  // Read-only
            return false;
        }
        
        if (!prefs.isKey("config")) {
            prefs.end();
            return false;
        }
        
        String json = prefs.getString("config", "");
        prefs.end();
        
        if (json.length() == 0) {
            return false;
        }
        
        Serial.println("[ConfigManager] Loading configuration from NVS");
        return fromJson(json.c_str());
    }

    // Reset to factory defaults
    void resetToDefaults() {
        Serial.println("[ConfigManager] Resetting to defaults...");
        loadDefaults();
        saveToNVS();
        printConfig();
    }

    // Print current configuration
    void printConfig() {
        Serial.println("\n[ConfigManager] Current Configuration:");
        Serial.printf("  Scan Config:\n");
        Serial.printf("    BLE Scan Duration: %d sec\n", scanConfig.bleScanDuration);
        Serial.printf("    BLE Scan Interval: %d ms\n", scanConfig.bleScanInterval);
        Serial.printf("    Channel Hop Interval: %d ms\n", scanConfig.channelHopInterval);
        Serial.printf("    Max Channel: %d\n", scanConfig.maxChannel);
        Serial.printf("    Heartbeat Interval: %d ms\n", scanConfig.heartbeatInterval);
        Serial.printf("    Detection Timeout: %d ms\n", scanConfig.detectionTimeout);
        Serial.printf("  Patterns:\n");
        Serial.printf("    SSID patterns: %d\n", ssidPatterns.size());
        Serial.printf("    MAC prefixes: %d\n", macPrefixes.size());
        Serial.printf("    BLE name patterns: %d\n", bleNamePatterns.size());
        Serial.printf("    BLE UUID patterns: %d\n", bleUuidPatterns.size());
        Serial.println();
    }

private:
    ScanConfig scanConfig;
    std::vector<SsidPattern> ssidPatterns;
    std::vector<MacPrefix> macPrefixes;
    std::vector<BleNamePattern> bleNamePatterns;
    std::vector<BleUuidPattern> bleUuidPatterns;

    ConfigManager() {
        loadDefaults();
    }

    void loadDefaults() {
        // Default scan configuration
        scanConfig.wifiScanEnabled = true;   // WiFi scanning enabled by default
        scanConfig.bleScanEnabled = true;    // BLE scanning enabled by default
        scanConfig.bleScanDuration = DEFAULT_BLE_SCAN_DURATION;
        scanConfig.bleScanInterval = DEFAULT_BLE_SCAN_INTERVAL;
        scanConfig.channelHopInterval = DEFAULT_CHANNEL_HOP_INTERVAL;
        scanConfig.maxChannel = DEFAULT_MAX_CHANNEL;
        scanConfig.heartbeatInterval = DEFAULT_HEARTBEAT_INTERVAL;
        scanConfig.detectionTimeout = DEFAULT_DETECTION_TIMEOUT;
        scanConfig.streamMode = STREAM_ALL;  // Stream all data by default

        // Clear existing patterns
        ssidPatterns.clear();
        macPrefixes.clear();
        bleNamePatterns.clear();
        bleUuidPatterns.clear();

        // Default SSID patterns
        addSsidPattern("flock", "Flock Safety", false, true);
        addSsidPattern("Flock", "Flock Safety", false, true);
        addSsidPattern("FLOCK", "Flock Safety", false, true);
        addSsidPattern("FS Ext Battery", "FS Ext Battery", true, true);
        addSsidPattern("Penguin", "Penguin", false, true);
        addSsidPattern("Pigvision", "Pigvision", false, true);

        // Default MAC prefixes - FS Ext Battery
        addMacPrefix("58:8e:81", "FS Ext Battery", true);
        addMacPrefix("cc:cc:cc", "FS Ext Battery", true);
        addMacPrefix("ec:1b:bd", "FS Ext Battery", true);
        addMacPrefix("90:35:ea", "FS Ext Battery", true);
        addMacPrefix("04:0d:84", "FS Ext Battery", true);
        addMacPrefix("f0:82:c0", "FS Ext Battery", true);
        addMacPrefix("1c:34:f1", "FS Ext Battery", true);
        addMacPrefix("38:5b:44", "FS Ext Battery", true);
        addMacPrefix("94:34:69", "FS Ext Battery", true);
        addMacPrefix("b4:e3:f9", "FS Ext Battery", true);
        
        // Default MAC prefixes - Flock Safety WiFi
        addMacPrefix("70:c9:4e", "Flock Safety", true);
        addMacPrefix("3c:91:80", "Flock Safety", true);
        addMacPrefix("d8:f3:bc", "Flock Safety", true);
        addMacPrefix("80:30:49", "Flock Safety", true);
        addMacPrefix("14:5a:fc", "Flock Safety", true);
        addMacPrefix("74:4c:a1", "Flock Safety", true);
        addMacPrefix("08:3a:88", "Flock Safety", true);
        addMacPrefix("9c:2f:9d", "Flock Safety", true);
        addMacPrefix("94:08:53", "Flock Safety", true);
        addMacPrefix("e4:aa:ea", "Flock Safety", true);
        addMacPrefix("b4:1e:52", "Flock Safety", true);  // IEEE OUI

        // Default BLE name patterns
        addBleNamePattern("FS Ext Battery", "FS Ext Battery", true);
        addBleNamePattern("Penguin", "Penguin", true);
        addBleNamePattern("Flock", "Flock Safety", true);
        addBleNamePattern("Pigvision", "Pigvision", true);

        // Default BLE UUID patterns (Raven)
        addBleUuidPattern("0000180a-0000-1000-8000-00805f9b34fb", "Raven", true);
        addBleUuidPattern("00003100-0000-1000-8000-00805f9b34fb", "Raven", true);
        addBleUuidPattern("00003200-0000-1000-8000-00805f9b34fb", "Raven", true);
        addBleUuidPattern("00003300-0000-1000-8000-00805f9b34fb", "Raven", true);
        addBleUuidPattern("00003400-0000-1000-8000-00805f9b34fb", "Raven", true);
        addBleUuidPattern("00003500-0000-1000-8000-00805f9b34fb", "Raven", true);
        addBleUuidPattern("00001809-0000-1000-8000-00805f9b34fb", "Raven", true);
        addBleUuidPattern("00001819-0000-1000-8000-00805f9b34fb", "Raven", true);
    }
};

// Global accessor
#define configManager ConfigManager::getInstance()

#endif // CONFIG_MANAGER_H
