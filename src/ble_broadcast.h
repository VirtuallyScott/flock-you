#ifndef BLE_BROADCAST_H
#define BLE_BROADCAST_H

#include <NimBLEDevice.h>
#include <ArduinoJson.h>
#include "config_manager.h"

// ============================================================================
// BLE BROADCAST SERVICE FOR IOS APP
// ============================================================================
// This module creates a BLE GATT server that broadcasts detection data to
// the FlockFinder iOS companion app.

// Service and Characteristic UUIDs (must match iOS app)
#define FLOCK_SERVICE_UUID           "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define DETECTION_CHAR_UUID          "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define COMMAND_CHAR_UUID            "beb5483e-36e1-4688-b7f5-ea07361b26a9"
#define STREAM_CHAR_UUID             "beb5483e-36e1-4688-b7f5-ea07361b26aa"  // Live scan stream
#define CONFIG_CHAR_UUID             "beb5483e-36e1-4688-b7f5-ea07361b26ab"  // Configuration transfer

// BLE Server objects
static NimBLEServer* pServer = nullptr;
static NimBLECharacteristic* pDetectionCharacteristic = nullptr;
static NimBLECharacteristic* pCommandCharacteristic = nullptr;
static NimBLECharacteristic* pStreamCharacteristic = nullptr;
static NimBLECharacteristic* pConfigCharacteristic = nullptr;  // Config characteristic
static bool deviceConnected = false;
static bool oldDeviceConnected = false;
static bool streamingEnabled = true;  // Enable/disable streaming

// Config transfer state (for chunked transfers)
static String configReceiveBuffer = "";
static bool configTransferInProgress = false;

// Forward declaration for detection callback
void (*onCommandReceived)(const char* command) = nullptr;
void (*onConfigUpdated)() = nullptr;  // Called when config is updated from iOS

// Forward declarations for config functions (needed by callback classes)
void sendCurrentConfig();
void sendConfigResponse(const char* type, bool success, const char* message);
void sendConfigChunked(const String& json);

// ============================================================================
// BLE SERVER CALLBACKS
// ============================================================================

class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) {
        deviceConnected = true;
        Serial.println("[BLE Server] iOS app connected!");
    }

    void onDisconnect(NimBLEServer* pServer) {
        deviceConnected = false;
        Serial.println("[BLE Server] iOS app disconnected");
        // Restart advertising
        NimBLEDevice::startAdvertising();
    }
};

class CommandCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            Serial.printf("[BLE Server] Command received: %s\n", value.c_str());
            
            // Handle built-in commands
            if (value == "stream_on") {
                streamingEnabled = true;
                Serial.println("[BLE Server] Streaming ENABLED");
            } else if (value == "stream_off") {
                streamingEnabled = false;
                Serial.println("[BLE Server] Streaming DISABLED");
            } else if (value == "GET_CONFIG") {
                // Send current config to iOS app
                sendCurrentConfig();
            } else if (value == "SAVE_CONFIG") {
                // Save current config to NVS
                configManager.saveToNVS();
                sendConfigResponse("CONFIG_SAVED", true, "Configuration saved to flash");
            } else if (value == "RESET_CONFIG") {
                // Reset to factory defaults
                configManager.resetToDefaults();
                if (onConfigUpdated) onConfigUpdated();
                sendConfigResponse("CONFIG_RESET", true, "Configuration reset to defaults");
            } else if (onCommandReceived) {
                onCommandReceived(value.c_str());
            }
        }
    }
};

// Config characteristic callbacks - handles configuration transfer
class ConfigCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() == 0) return;
        
        String data = String(value.c_str());
        Serial.printf("[BLE Config] Received %d bytes\n", data.length());
        
        // Check for chunked transfer markers
        if (data.startsWith("CONFIG_START")) {
            // Start of chunked transfer
            configReceiveBuffer = "";
            configTransferInProgress = true;
            Serial.println("[BLE Config] Starting chunked config receive");
            return;
        }
        
        if (data == "CONFIG_END") {
            // End of chunked transfer - process the complete config
            configTransferInProgress = false;
            Serial.printf("[BLE Config] Received complete config (%d bytes)\n", configReceiveBuffer.length());
            
            if (configManager.fromJson(configReceiveBuffer.c_str())) {
                if (onConfigUpdated) onConfigUpdated();
                sendConfigResponse("CONFIG_UPDATED", true, "Configuration applied successfully");
            } else {
                sendConfigResponse("CONFIG_ERROR", false, "Failed to parse configuration");
            }
            configReceiveBuffer = "";
            return;
        }
        
        if (configTransferInProgress) {
            // Append chunk to buffer
            configReceiveBuffer += data;
            Serial.printf("[BLE Config] Buffered chunk, total: %d bytes\n", configReceiveBuffer.length());
            return;
        }
        
        // Single-packet config (small enough to fit in one write)
        if (data.startsWith("{")) {
            Serial.println("[BLE Config] Processing single-packet config");
            if (configManager.fromJson(data.c_str())) {
                if (onConfigUpdated) onConfigUpdated();
                sendConfigResponse("CONFIG_UPDATED", true, "Configuration applied successfully");
            } else {
                sendConfigResponse("CONFIG_ERROR", false, "Failed to parse configuration");
            }
        }
    }
    
    void onRead(NimBLECharacteristic* pCharacteristic) {
        // Return current config when read
        Serial.println("[BLE Config] Config characteristic read requested");
        // Note: Full config is too large for a single read, use GET_CONFIG command
    }
};

// ============================================================================
// INITIALIZATION
// ============================================================================

void initBLEBroadcast() {
    Serial.println("[BLE Server] Initializing BLE broadcast service...");
    
    // Create server
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
    
    // Create service
    NimBLEService* pService = pServer->createService(FLOCK_SERVICE_UUID);
    
    // Create detection characteristic (notify to iOS app)
    pDetectionCharacteristic = pService->createCharacteristic(
        DETECTION_CHAR_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );
    
    // Create command characteristic (receive from iOS app)
    pCommandCharacteristic = pService->createCharacteristic(
        COMMAND_CHAR_UUID,
        NIMBLE_PROPERTY::WRITE
    );
    pCommandCharacteristic->setCallbacks(new CommandCallbacks());
    
    // Create stream characteristic for live scan data (notify to iOS app)
    pStreamCharacteristic = pService->createCharacteristic(
        STREAM_CHAR_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );
    
    // Create config characteristic for configuration transfer
    pConfigCharacteristic = pService->createCharacteristic(
        CONFIG_CHAR_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY
    );
    pConfigCharacteristic->setCallbacks(new ConfigCallbacks());
    
    // Start the service
    pService->start();
    
    // Configure advertising for maximum iOS visibility
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    
    // Reset advertising data
    pAdvertising->reset();
    
    // Add the service UUID so iOS can find us when scanning for this service
    pAdvertising->addServiceUUID(FLOCK_SERVICE_UUID);
    
    // Set appearance (generic tag for better iOS visibility)
    pAdvertising->setAppearance(0x0200);  // Generic Tag
    
    // Enable scan response to include full device name
    pAdvertising->setScanResponse(true);
    
    // Set preferred connection intervals for iOS compatibility
    pAdvertising->setMinPreferred(0x06);  // 7.5ms minimum (7.5ms)
    pAdvertising->setMaxPreferred(0x12);  // 22.5ms maximum
    
    // Start advertising
    NimBLEDevice::startAdvertising();
    
    Serial.println("[BLE Server] ========================================");
    Serial.println("[BLE Server] BLE ADVERTISING ACTIVE");
    Serial.printf("[BLE Server] Device Name: %s\n", NimBLEDevice::toString().c_str());
    Serial.println("[BLE Server] Service UUID: " FLOCK_SERVICE_UUID);
    Serial.println("[BLE Server] Open FlockFinder iOS app and tap 'Scan'");
    Serial.println("[BLE Server] Note: BLE devices don't appear in iOS Settings!");
    Serial.println("[BLE Server] ========================================");
}

// ============================================================================
// BROADCAST DETECTION TO IOS APP
// ============================================================================

bool isAppConnected() {
    return deviceConnected;
}

void broadcastDetection(const char* deviceType, const char* macAddress, const char* ssid, int rssi, double confidence) {
    if (!deviceConnected || !pDetectionCharacteristic) {
        return;  // No app connected, skip broadcast
    }
    
    // Create JSON payload
    StaticJsonDocument<256> doc;
    doc["type"] = deviceType;
    doc["mac"] = macAddress ? macAddress : "";
    doc["ssid"] = ssid ? ssid : "";
    doc["rssi"] = rssi;
    doc["confidence"] = confidence;
    doc["ts"] = millis();
    
    // Serialize to string
    char buffer[256];
    size_t len = serializeJson(doc, buffer);
    
    // Send notification to iOS app
    pDetectionCharacteristic->setValue((uint8_t*)buffer, len);
    pDetectionCharacteristic->notify();
    
    Serial.printf("[BLE Server] Broadcasted detection to iOS app: %s\n", deviceType);
}

// Overload for simpler calls
void broadcastWiFiDetection(const char* ssid, const uint8_t* mac, int rssi) {
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x", 
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    // Determine device type from SSID pattern
    const char* deviceType = "Flock Safety";
    if (strcasestr(ssid, "penguin")) {
        deviceType = "Penguin";
    } else if (strcasestr(ssid, "pigvision")) {
        deviceType = "Pigvision";
    }
    
    broadcastDetection(deviceType, mac_str, ssid, rssi, 0.9);
}

void broadcastBLEDetection(const char* deviceName, const char* macAddress, int rssi, const char* detectedType) {
    const char* deviceType = detectedType ? detectedType : "Unknown";
    broadcastDetection(deviceType, macAddress, deviceName, rssi, 0.85);
}

// ============================================================================
// STREAM LIVE SCAN DATA TO IOS APP
// ============================================================================

// Stream a WiFi scan result to iOS app
// If streamMode is STREAM_MATCHES_ONLY, only streams if isMatch is true
void streamWiFiScan(const char* ssid, const uint8_t* mac, int rssi, int channel, const char* frameType, bool isMatch = false) {
    if (!deviceConnected || !pStreamCharacteristic || !streamingEnabled) {
        return;
    }
    
    // Check stream mode - only stream if mode is ALL or this is a match
    ConfigManager& cfg = ConfigManager::getInstance();
    if (cfg.getScanConfig().streamMode == STREAM_MATCHES_ONLY && !isMatch) {
        return;
    }
    
    StaticJsonDocument<256> doc;
    doc["evt"] = "wifi_scan";
    doc["ssid"] = ssid ? ssid : "";
    
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x", 
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    doc["mac"] = mac_str;
    doc["rssi"] = rssi;
    doc["ch"] = channel;
    doc["type"] = frameType;
    doc["ts"] = millis();
    doc["match"] = isMatch;  // Include match status
    
    char buffer[256];
    size_t len = serializeJson(doc, buffer);
    
    pStreamCharacteristic->setValue((uint8_t*)buffer, len);
    pStreamCharacteristic->notify();
}

// Stream a BLE scan result to iOS app
// If streamMode is STREAM_MATCHES_ONLY, only streams if isMatch is true
void streamBLEScan(const char* name, const char* mac, int rssi, bool hasServices, bool isMatch = false) {
    if (!deviceConnected || !pStreamCharacteristic || !streamingEnabled) {
        return;
    }
    
    // Check stream mode - only stream if mode is ALL or this is a match
    ConfigManager& cfg = ConfigManager::getInstance();
    if (cfg.getScanConfig().streamMode == STREAM_MATCHES_ONLY && !isMatch) {
        return;
    }
    
    StaticJsonDocument<256> doc;
    doc["evt"] = "ble_scan";
    doc["name"] = name ? name : "";
    doc["mac"] = mac;
    doc["rssi"] = rssi;
    doc["svc"] = hasServices;
    doc["ts"] = millis();
    doc["match"] = isMatch;  // Include match status
    
    char buffer[256];
    size_t len = serializeJson(doc, buffer);
    
    pStreamCharacteristic->setValue((uint8_t*)buffer, len);
    pStreamCharacteristic->notify();
}

// Stream status/info messages to iOS app
void streamStatus(const char* message) {
    if (!deviceConnected || !pStreamCharacteristic || !streamingEnabled) {
        return;
    }
    
    StaticJsonDocument<150> doc;
    doc["evt"] = "status";
    doc["msg"] = message;
    doc["ts"] = millis();
    
    char buffer[150];
    size_t len = serializeJson(doc, buffer);
    
    pStreamCharacteristic->setValue((uint8_t*)buffer, len);
    pStreamCharacteristic->notify();
}

// Stream channel hop notification
void streamChannelHop(int channel) {
    if (!deviceConnected || !pStreamCharacteristic || !streamingEnabled) {
        return;
    }
    
    StaticJsonDocument<100> doc;
    doc["evt"] = "channel";
    doc["ch"] = channel;
    doc["ts"] = millis();
    
    char buffer[100];
    size_t len = serializeJson(doc, buffer);
    
    pStreamCharacteristic->setValue((uint8_t*)buffer, len);
    pStreamCharacteristic->notify();
}

// Set callback for commands from iOS app
void setCommandCallback(void (*callback)(const char*)) {
    onCommandReceived = callback;
}

// Set callback for when config is updated from iOS
void setConfigUpdatedCallback(void (*callback)()) {
    onConfigUpdated = callback;
}

// ============================================================================
// CONFIGURATION TRANSFER FUNCTIONS
// ============================================================================

// Send a config response/status to iOS app
void sendConfigResponse(const char* type, bool success, const char* message) {
    if (!deviceConnected || !pConfigCharacteristic) return;
    
    StaticJsonDocument<256> doc;
    doc["evt"] = type;
    doc["success"] = success;
    doc["msg"] = message;
    doc["ts"] = millis();
    
    char buffer[256];
    size_t len = serializeJson(doc, buffer);
    
    pConfigCharacteristic->setValue((uint8_t*)buffer, len);
    pConfigCharacteristic->notify();
    
    Serial.printf("[BLE Config] Sent response: %s - %s\n", type, message);
}

// Send config in chunks (for large configs that exceed MTU)
void sendConfigChunked(const String& json) {
    if (!deviceConnected || !pConfigCharacteristic) return;
    
    const int CHUNK_SIZE = 500;  // BLE MTU is typically 512 max, leave room for overhead
    int totalLen = json.length();
    int offset = 0;
    int chunkNum = 0;
    
    Serial.printf("[BLE Config] Sending config in chunks (%d bytes total)\n", totalLen);
    
    // Send start marker
    pConfigCharacteristic->setValue("CONFIG_START");
    pConfigCharacteristic->notify();
    delay(20);  // Give iOS time to process
    
    // Send chunks
    while (offset < totalLen) {
        int chunkLen = min(CHUNK_SIZE, totalLen - offset);
        String chunk = json.substring(offset, offset + chunkLen);
        
        pConfigCharacteristic->setValue((uint8_t*)chunk.c_str(), chunk.length());
        pConfigCharacteristic->notify();
        
        chunkNum++;
        offset += chunkLen;
        Serial.printf("[BLE Config] Sent chunk %d (%d/%d bytes)\n", chunkNum, offset, totalLen);
        delay(20);  // Give iOS time to process
    }
    
    // Send end marker
    pConfigCharacteristic->setValue("CONFIG_END");
    pConfigCharacteristic->notify();
    
    Serial.printf("[BLE Config] Config transfer complete (%d chunks)\n", chunkNum);
}

// Send current config to iOS app
void sendCurrentConfig() {
    if (!deviceConnected || !pConfigCharacteristic) {
        Serial.println("[BLE Config] Cannot send config - not connected");
        return;
    }
    
    String json = configManager.toJson();
    Serial.printf("[BLE Config] Sending current config (%d bytes)\n", json.length());
    
    // If config fits in one packet, send directly
    if (json.length() <= 500) {
        pConfigCharacteristic->setValue((uint8_t*)json.c_str(), json.length());
        pConfigCharacteristic->notify();
    } else {
        // Send in chunks
        sendConfigChunked(json);
    }
}

#endif // BLE_BROADCAST_H
