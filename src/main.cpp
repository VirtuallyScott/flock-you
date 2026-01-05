#include <Arduino.h>
#include <WiFi.h>
#include <NimBLEDevice.h>
#include <NimBLEScan.h>
#include <NimBLEAdvertisedDevice.h>
#include <ArduinoJson.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include <Adafruit_NeoPixel.h>
#include "config_manager.h"
#include "ble_broadcast.h"

// ============================================================================
// CONFIGURATION
// ============================================================================

// Hardware Configuration - Unexpected Maker FeatherS3 RGB LED
#define RGB_DATA_PIN 40      // FeatherS3 RGB LED data pin
#define RGB_POWER_PIN 39     // FeatherS3 RGB LED power control
#define NUM_PIXELS 1         // Single RGB LED on FeatherS3

// Create NeoPixel object
Adafruit_NeoPixel pixel(NUM_PIXELS, RGB_DATA_PIN, NEO_GRB + NEO_KHZ800);

// LED Colors (RGB format)
#define COLOR_OFF       pixel.Color(0, 0, 0)
#define COLOR_BOOT_LOW  pixel.Color(0, 0, 50)      // Blue - boot sequence
#define COLOR_BOOT_HIGH pixel.Color(0, 50, 0)      // Green - boot complete
#define COLOR_DETECT    pixel.Color(255, 0, 0)     // Red - detection alert!
#define COLOR_HEARTBEAT pixel.Color(50, 0, 50)     // Purple - heartbeat
#define COLOR_SCANNING  pixel.Color(0, 20, 20)     // Cyan dim - scanning
#define COLOR_CONFIG    pixel.Color(0, 50, 50)     // Cyan bright - config update

// Visual Alert Timing
#define BOOT_FLASH_DURATION 300   // Boot flash duration
#define DETECT_FLASH_DURATION 150 // Detection flash duration (faster)
#define HEARTBEAT_DURATION 100    // Short heartbeat pulse

// ============================================================================
// SCAN CONFIGURATION (Now dynamic via ConfigManager)
// ============================================================================
// Scan intervals are now managed by ConfigManager and can be updated via iOS app.
// These are just initial values before config is loaded.

// ============================================================================
// RAVEN SURVEILLANCE DEVICE - These UUIDs are used for detection only
// The actual pattern matching uses ConfigManager's BLE UUID patterns
// ============================================================================

// Raven service UUIDs for firmware version detection (kept for version estimation)
#define RAVEN_DEVICE_INFO_SERVICE       "0000180a-0000-1000-8000-00805f9b34fb"
#define RAVEN_GPS_SERVICE               "00003100-0000-1000-8000-00805f9b34fb"
#define RAVEN_POWER_SERVICE             "00003200-0000-1000-8000-00805f9b34fb"
#define RAVEN_NETWORK_SERVICE           "00003300-0000-1000-8000-00805f9b34fb"
#define RAVEN_UPLOAD_SERVICE            "00003400-0000-1000-8000-00805f9b34fb"
#define RAVEN_ERROR_SERVICE             "00003500-0000-1000-8000-00805f9b34fb"
#define RAVEN_OLD_HEALTH_SERVICE        "00001809-0000-1000-8000-00805f9b34fb"
#define RAVEN_OLD_LOCATION_SERVICE      "00001819-0000-1000-8000-00805f9b34fb"

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

static uint8_t current_channel = 1;
static unsigned long last_channel_hop = 0;
static bool triggered = false;
static bool device_in_range = false;
static unsigned long last_detection_time = 0;
static unsigned long last_heartbeat = 0;
static NimBLEScan* pBLEScan;

// Forward declaration for config update callback
void onConfigurationUpdated();

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================
void led_flash(uint32_t color, int duration_ms);
void init_led();
void boot_led_sequence();
void flock_detected_led_sequence();
void heartbeat_pulse();

// ============================================================================
// LED VISUAL ALERT SYSTEM (FeatherS3 RGB LED)
// ============================================================================

void led_flash(uint32_t color, int duration_ms)
{
    pixel.setPixelColor(0, color);
    pixel.show();
    delay(duration_ms);
    pixel.setPixelColor(0, COLOR_OFF);
    pixel.show();
    delay(50);
}

void init_led()
{
    // Enable RGB LED power on FeatherS3
    pinMode(RGB_POWER_PIN, OUTPUT);
    digitalWrite(RGB_POWER_PIN, HIGH);
    delay(10);
    
    // Initialize NeoPixel
    pixel.begin();
    pixel.setBrightness(50);  // Set to 50/255 brightness
    pixel.clear();
    pixel.show();
}

void boot_led_sequence()
{
    printf("Initializing LED visual system...\n");
    printf("Playing boot sequence: Blue -> Green\n");
    led_flash(COLOR_BOOT_LOW, BOOT_FLASH_DURATION);   // Blue flash
    led_flash(COLOR_BOOT_HIGH, BOOT_FLASH_DURATION);  // Green flash
    // Leave green on briefly to show ready
    pixel.setPixelColor(0, COLOR_BOOT_HIGH);
    pixel.show();
    delay(500);
    pixel.setPixelColor(0, COLOR_SCANNING);
    pixel.show();
    printf("LED system ready\n\n");
}

void flock_detected_led_sequence()
{
    printf("FLOCK SAFETY DEVICE DETECTED!\n");
    printf("LED alert sequence: 3 fast RED flashes\n");
    for (int i = 0; i < 3; i++) {
        led_flash(COLOR_DETECT, DETECT_FLASH_DURATION);
        if (i < 2) delay(50); // Short gap between flashes
    }
    printf("Detection complete - device identified!\n\n");
    
    // Mark device as in range and start heartbeat tracking
    device_in_range = true;
    last_detection_time = millis();
    last_heartbeat = millis();
    
    // Keep LED red while device in range
    pixel.setPixelColor(0, COLOR_DETECT);
    pixel.show();
}

void heartbeat_pulse()
{
    printf("Heartbeat: Device still in range\n");
    led_flash(COLOR_HEARTBEAT, HEARTBEAT_DURATION);
    delay(100);
    led_flash(COLOR_HEARTBEAT, HEARTBEAT_DURATION);
    // Return to detection color
    pixel.setPixelColor(0, COLOR_DETECT);
    pixel.show();
}

// ============================================================================
// JSON OUTPUT FUNCTIONS
// ============================================================================

void output_wifi_detection_json(const char* ssid, const uint8_t* mac, int rssi, const char* detection_type, const char* deviceType = "Unknown")
{
    DynamicJsonDocument doc(2048);
    
    // Core detection info
    doc["timestamp"] = millis();
    doc["detection_time"] = String(millis() / 1000.0, 3) + "s";
    doc["protocol"] = "wifi";
    doc["detection_method"] = detection_type;
    doc["alert_level"] = "HIGH";
    doc["device_category"] = deviceType;
    doc["device_type"] = deviceType;
    
    // WiFi specific info
    doc["ssid"] = ssid;
    doc["ssid_length"] = strlen(ssid);
    doc["rssi"] = rssi;
    doc["signal_strength"] = rssi > -50 ? "STRONG" : (rssi > -70 ? "MEDIUM" : "WEAK");
    doc["channel"] = current_channel;
    
    // MAC address info
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x", 
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    doc["mac_address"] = mac_str;
    
    char mac_prefix[9];
    snprintf(mac_prefix, sizeof(mac_prefix), "%02x:%02x:%02x", mac[0], mac[1], mac[2]);
    doc["mac_prefix"] = mac_prefix;
    doc["vendor_oui"] = mac_prefix;
    
    // Detection pattern matching - use ConfigManager
    String ssidDeviceType, macDeviceType;
    bool ssid_match = configManager.checkSsidMatch(ssid, ssidDeviceType);
    bool mac_match = configManager.checkMacMatch(mac, macDeviceType);
    
    if (ssid_match) {
        doc["ssid_match_confidence"] = "HIGH";
    }
    if (mac_match) {
        doc["mac_match_confidence"] = "HIGH";
    }
    
    // Detection summary
    doc["detection_criteria"] = ssid_match && mac_match ? "SSID_AND_MAC" : (ssid_match ? "SSID_ONLY" : "MAC_ONLY");
    doc["threat_score"] = ssid_match && mac_match ? 100 : (ssid_match || mac_match ? 85 : 70);
    
    // Frame type details
    if (strcmp(detection_type, "probe_request") == 0 || strcmp(detection_type, "probe_request_mac") == 0) {
        doc["frame_type"] = "PROBE_REQUEST";
        doc["frame_description"] = "Device actively scanning for networks";
    } else {
        doc["frame_type"] = "BEACON";
        doc["frame_description"] = "Device advertising its network";
    }
    
    String json_output;
    serializeJson(doc, json_output);
    Serial.println(json_output);
}

void output_ble_detection_json(const char* mac, const char* name, int rssi, const char* detection_method, const char* deviceType = "Unknown")
{
    DynamicJsonDocument doc(2048);
    
    // Core detection info
    doc["timestamp"] = millis();
    doc["detection_time"] = String(millis() / 1000.0, 3) + "s";
    doc["protocol"] = "bluetooth_le";
    doc["detection_method"] = detection_method;
    doc["alert_level"] = "HIGH";
    doc["device_category"] = deviceType;
    doc["device_type"] = deviceType;
    
    // BLE specific info
    doc["mac_address"] = mac;
    doc["rssi"] = rssi;
    doc["signal_strength"] = rssi > -50 ? "STRONG" : (rssi > -70 ? "MEDIUM" : "WEAK");
    
    // Device name info
    if (name && strlen(name) > 0) {
        doc["device_name"] = name;
        doc["device_name_length"] = strlen(name);
        doc["has_device_name"] = true;
    } else {
        doc["device_name"] = "";
        doc["device_name_length"] = 0;
        doc["has_device_name"] = false;
    }
    
    // MAC address analysis
    char mac_prefix[9];
    strncpy(mac_prefix, mac, 8);
    mac_prefix[8] = '\0';
    doc["mac_prefix"] = mac_prefix;
    doc["vendor_oui"] = mac_prefix;
    
    // Detection pattern matching - use ConfigManager
    String macDeviceType, nameDeviceType;
    bool mac_match = configManager.checkMacMatch(mac, macDeviceType);
    bool name_match = (name && strlen(name) > 0) ? configManager.checkBleNameMatch(name, nameDeviceType) : false;
    
    if (mac_match) {
        doc["mac_match_confidence"] = "HIGH";
    }
    if (name_match) {
        doc["name_match_confidence"] = "HIGH";
    }
    
    // Detection summary
    doc["detection_criteria"] = name_match && mac_match ? "NAME_AND_MAC" : 
                               (name_match ? "NAME_ONLY" : "MAC_ONLY");
    doc["threat_score"] = name_match && mac_match ? 100 : 
                         (name_match || mac_match ? 85 : 70);
    
    // BLE advertisement type analysis
    doc["advertisement_type"] = "BLE_ADVERTISEMENT";
    doc["advertisement_description"] = "Bluetooth Low Energy device advertisement";
    
    // Detection method details
    if (strcmp(detection_method, "mac_prefix") == 0) {
        doc["primary_indicator"] = "MAC_ADDRESS";
        doc["detection_reason"] = "MAC address matches known surveillance device prefix";
    } else if (strcmp(detection_method, "device_name") == 0) {
        doc["primary_indicator"] = "DEVICE_NAME";
        doc["detection_reason"] = "Device name matches Flock Safety pattern";
    }
    
    String json_output;
    serializeJson(doc, json_output);
    Serial.println(json_output);
}

// ============================================================================
// DETECTION HELPER FUNCTIONS (Now using ConfigManager)
// ============================================================================

bool check_mac_prefix(const uint8_t* mac)
{
    String deviceType;
    return configManager.checkMacMatch(mac, deviceType);
}

bool check_mac_prefix(const uint8_t* mac, String& deviceTypeOut)
{
    return configManager.checkMacMatch(mac, deviceTypeOut);
}

bool check_ssid_pattern(const char* ssid)
{
    String deviceType;
    return configManager.checkSsidMatch(ssid, deviceType);
}

bool check_ssid_pattern(const char* ssid, String& deviceTypeOut)
{
    return configManager.checkSsidMatch(ssid, deviceTypeOut);
}

bool check_device_name_pattern(const char* name)
{
    String deviceType;
    return configManager.checkBleNameMatch(name, deviceType);
}

bool check_device_name_pattern(const char* name, String& deviceTypeOut)
{
    return configManager.checkBleNameMatch(name, deviceTypeOut);
}

// ============================================================================
// RAVEN UUID DETECTION (Now using ConfigManager for pattern matching)
// ============================================================================

// Check if a BLE device advertises any surveillance service UUIDs
bool check_raven_service_uuid(NimBLEAdvertisedDevice* device, char* detected_service_out = nullptr, String* deviceTypeOut = nullptr)
{
    if (!device) return false;
    
    // Check if device has service UUIDs
    if (!device->haveServiceUUID()) return false;
    
    // Get the number of service UUIDs
    int serviceCount = device->getServiceUUIDCount();
    if (serviceCount == 0) return false;
    
    // Check each advertised service UUID against known patterns
    for (int i = 0; i < serviceCount; i++) {
        NimBLEUUID serviceUUID = device->getServiceUUID(i);
        std::string uuidStr = serviceUUID.toString();
        
        // Use ConfigManager to check UUID patterns
        String deviceType;
        if (configManager.checkBleUuidMatch(uuidStr.c_str(), deviceType)) {
            if (detected_service_out != nullptr) {
                strncpy(detected_service_out, uuidStr.c_str(), 40);
            }
            if (deviceTypeOut != nullptr) {
                *deviceTypeOut = deviceType;
            }
            return true;
        }
    }
    
    return false;
}

// Get a human-readable description of the Raven service
const char* get_raven_service_description(const char* uuid)
{
    if (!uuid) return "Unknown Service";
    
    if (strcasecmp(uuid, RAVEN_DEVICE_INFO_SERVICE) == 0)
        return "Device Information (Serial, Model, Firmware)";
    if (strcasecmp(uuid, RAVEN_GPS_SERVICE) == 0)
        return "GPS Location Service (Lat/Lon/Alt)";
    if (strcasecmp(uuid, RAVEN_POWER_SERVICE) == 0)
        return "Power Management (Battery/Solar)";
    if (strcasecmp(uuid, RAVEN_NETWORK_SERVICE) == 0)
        return "Network Status (LTE/WiFi)";
    if (strcasecmp(uuid, RAVEN_UPLOAD_SERVICE) == 0)
        return "Upload Statistics Service";
    if (strcasecmp(uuid, RAVEN_ERROR_SERVICE) == 0)
        return "Error/Failure Tracking Service";
    if (strcasecmp(uuid, RAVEN_OLD_HEALTH_SERVICE) == 0)
        return "Health/Temperature Service (Legacy)";
    if (strcasecmp(uuid, RAVEN_OLD_LOCATION_SERVICE) == 0)
        return "Location Service (Legacy)";
    
    return "Unknown Raven Service";
}

// Estimate firmware version based on detected service UUIDs
const char* estimate_raven_firmware_version(NimBLEAdvertisedDevice* device)
{
    if (!device || !device->haveServiceUUID()) return "Unknown";
    
    bool has_new_gps = false;
    bool has_old_location = false;
    bool has_power_service = false;
    
    int serviceCount = device->getServiceUUIDCount();
    for (int i = 0; i < serviceCount; i++) {
        NimBLEUUID serviceUUID = device->getServiceUUID(i);
        std::string uuidStr = serviceUUID.toString();
        
        if (strcasecmp(uuidStr.c_str(), RAVEN_GPS_SERVICE) == 0)
            has_new_gps = true;
        if (strcasecmp(uuidStr.c_str(), RAVEN_OLD_LOCATION_SERVICE) == 0)
            has_old_location = true;
        if (strcasecmp(uuidStr.c_str(), RAVEN_POWER_SERVICE) == 0)
            has_power_service = true;
    }
    
    // Firmware version heuristics based on service presence
    if (has_old_location && !has_new_gps)
        return "1.1.x (Legacy)";
    if (has_new_gps && !has_power_service)
        return "1.2.x";
    if (has_new_gps && has_power_service)
        return "1.3.x (Latest)";
    
    return "Unknown Version";
}

// ============================================================================
// WIFI PROMISCUOUS MODE HANDLER
// ============================================================================

typedef struct {
    unsigned frame_ctrl:16;
    unsigned duration_id:16;
    uint8_t addr1[6]; /* receiver address */
    uint8_t addr2[6]; /* sender address */
    uint8_t addr3[6]; /* filtering address */
    unsigned sequence_ctrl:16;
    uint8_t addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;

typedef struct {
    wifi_ieee80211_mac_hdr_t hdr;
    uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;

void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type)
{
    
    const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
    const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
    
    // Check for probe requests (subtype 0x04) and beacons (subtype 0x08)
    uint8_t frame_type = (hdr->frame_ctrl & 0xFF) >> 2;
    if (frame_type != 0x20 && frame_type != 0x80) { // Probe request or beacon
        return;
    }
    
    // Extract SSID from probe request or beacon
    char ssid[33] = {0};
    uint8_t *payload = (uint8_t *)ipkt + 24; // Skip MAC header
    
    if (frame_type == 0x20) { // Probe request
        payload += 0; // Probe requests start with SSID immediately
    } else { // Beacon frame
        payload += 12; // Skip fixed parameters in beacon
    }
    
    // Parse SSID element (tag 0, length, data)
    if (payload[0] == 0 && payload[1] <= 32) {
        memcpy(ssid, &payload[2], payload[1]);
        ssid[payload[1]] = '\0';
    }
    
    const char* frameTypeStr = (frame_type == 0x20) ? "probe" : "beacon";
    
    // Stream ALL WiFi packets to iOS app for debug view
    streamWiFiScan(ssid[0] ? ssid : "(hidden)", hdr->addr2, ppkt->rx_ctrl.rssi, current_channel, frameTypeStr);
    
    // Check if SSID matches our patterns
    String ssidDeviceType;
    if (strlen(ssid) > 0 && check_ssid_pattern(ssid, ssidDeviceType)) {
        const char* detection_type = (frame_type == 0x20) ? "probe_request" : "beacon";
        output_wifi_detection_json(ssid, hdr->addr2, ppkt->rx_ctrl.rssi, detection_type, ssidDeviceType.c_str());
        
        // Broadcast to iOS app if connected
        broadcastDetection(ssidDeviceType.c_str(), nullptr, ssid, ppkt->rx_ctrl.rssi, 0.9);
        
        if (!triggered) {
            triggered = true;
            flock_detected_led_sequence();
        }
        // Always update detection time for heartbeat tracking
        last_detection_time = millis();
        return;
    }
    
    // Check MAC address
    String macDeviceType;
    if (check_mac_prefix(hdr->addr2, macDeviceType)) {
        const char* detection_type = (frame_type == 0x20) ? "probe_request_mac" : "beacon_mac";
        output_wifi_detection_json(ssid[0] ? ssid : "hidden", hdr->addr2, ppkt->rx_ctrl.rssi, detection_type, macDeviceType.c_str());
        
        // Broadcast to iOS app if connected
        char mac_str[18];
        snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x", 
                 hdr->addr2[0], hdr->addr2[1], hdr->addr2[2], hdr->addr2[3], hdr->addr2[4], hdr->addr2[5]);
        broadcastDetection(macDeviceType.c_str(), mac_str, ssid[0] ? ssid : "unknown", ppkt->rx_ctrl.rssi, 0.85);
        
        if (!triggered) {
            triggered = true;
            flock_detected_led_sequence();
        }
        // Always update detection time for heartbeat tracking
        last_detection_time = millis();
        return;
    }
}

// ============================================================================
// BLE SCANNING
// ============================================================================

class AdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
        
        NimBLEAddress addr = advertisedDevice->getAddress();
        std::string addrStr = addr.toString();
        uint8_t mac[6];
        sscanf(addrStr.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x", 
               &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
        
        int rssi = advertisedDevice->getRSSI();
        std::string name = "";
        if (advertisedDevice->haveName()) {
            name = advertisedDevice->getName();
        }
        
        bool hasServices = advertisedDevice->haveServiceUUID();
        
        // Stream ALL BLE devices to iOS app for debug view
        streamBLEScan(name.c_str(), addrStr.c_str(), rssi, hasServices);
        
        // Check MAC prefix using ConfigManager
        String macDeviceType;
        if (check_mac_prefix(mac, macDeviceType)) {
            output_ble_detection_json(addrStr.c_str(), name.c_str(), rssi, "mac_prefix", macDeviceType.c_str());
            
            // Broadcast to iOS app
            broadcastDetection(macDeviceType.c_str(), addrStr.c_str(), name.c_str(), rssi, 0.9);
            
            if (!triggered) {
                triggered = true;
                flock_detected_led_sequence();
            }
            // Always update detection time for heartbeat tracking
            last_detection_time = millis();
            return;
        }
        
        // Check device name using ConfigManager
        String nameDeviceType;
        if (!name.empty() && check_device_name_pattern(name.c_str(), nameDeviceType)) {
            output_ble_detection_json(addrStr.c_str(), name.c_str(), rssi, "device_name", nameDeviceType.c_str());
            
            // Broadcast to iOS app with correct type from config
            broadcastDetection(nameDeviceType.c_str(), addrStr.c_str(), name.c_str(), rssi, 0.85);
            
            if (!triggered) {
                triggered = true;
                flock_detected_led_sequence();
            }
            // Always update detection time for heartbeat tracking
            last_detection_time = millis();
            return;
        }
        
        // Check for surveillance device service UUIDs using ConfigManager
        char detected_service_uuid[41] = {0};
        String uuidDeviceType;
        if (check_raven_service_uuid(advertisedDevice, detected_service_uuid, &uuidDeviceType)) {
            // Surveillance device detected! Get firmware version estimate
            const char* fw_version = estimate_raven_firmware_version(advertisedDevice);
            const char* service_desc = get_raven_service_description(detected_service_uuid);
            
            // Create enhanced JSON output with device-specific data
            StaticJsonDocument<1024> doc;
            doc["protocol"] = "bluetooth_le";
            doc["detection_method"] = "service_uuid";
            doc["device_type"] = uuidDeviceType.c_str();
            doc["mac_address"] = addrStr.c_str();
            doc["rssi"] = rssi;
            doc["signal_strength"] = rssi > -50 ? "STRONG" : (rssi > -70 ? "MEDIUM" : "WEAK");
            
            if (!name.empty()) {
                doc["device_name"] = name.c_str();
            }
            
            // Service-specific information
            doc["detected_service_uuid"] = detected_service_uuid;
            doc["service_description"] = service_desc;
            if (uuidDeviceType == "Raven") {
                doc["manufacturer"] = "SoundThinking/ShotSpotter";
                doc["firmware_version"] = fw_version;
            }
            doc["threat_level"] = "CRITICAL";
            doc["threat_score"] = 100;
            
            // List all detected service UUIDs
            if (advertisedDevice->haveServiceUUID()) {
                JsonArray services = doc.createNestedArray("service_uuids");
                int serviceCount = advertisedDevice->getServiceUUIDCount();
                for (int i = 0; i < serviceCount; i++) {
                    NimBLEUUID serviceUUID = advertisedDevice->getServiceUUID(i);
                    services.add(serviceUUID.toString().c_str());
                }
            }
            
            // Output the detection
            serializeJson(doc, Serial);
            Serial.println();
            
            // Broadcast detection to iOS app
            broadcastDetection(uuidDeviceType.c_str(), addrStr.c_str(), name.c_str(), rssi, 1.0);
            
            if (!triggered) {
                triggered = true;
                flock_detected_led_sequence();
            }
            // Always update detection time for heartbeat tracking
            last_detection_time = millis();
            return;
        }
    }
};

// ============================================================================
// CHANNEL HOPPING (Now using dynamic config)
// ============================================================================

void hop_channel()
{
    ScanConfig& cfg = configManager.getScanConfig();
    unsigned long now = millis();
    if (now - last_channel_hop > cfg.channelHopInterval) {
        current_channel++;
        if (current_channel > cfg.maxChannel) {
            current_channel = 1;
        }
        esp_wifi_set_channel(current_channel, WIFI_SECOND_CHAN_NONE);
        last_channel_hop = now;
        // Stream channel hop to iOS app
        streamChannelHop(current_channel);
    }
}

// ============================================================================
// CONFIGURATION UPDATE CALLBACK
// ============================================================================

void onConfigurationUpdated() {
    Serial.println("[Main] Configuration updated from iOS app!");
    
    // Flash LED to indicate config update
    led_flash(COLOR_CONFIG, 200);
    led_flash(COLOR_CONFIG, 200);
    
    // Print new config
    configManager.printConfig();
    
    // Note: Scan parameters will take effect on next scan cycle
}

// ============================================================================
// BLE SCAN TIMING
// ============================================================================

static unsigned long last_ble_scan = 0;

// ============================================================================
// MAIN FUNCTIONS
// ============================================================================

void setup()
{
    Serial.begin(115200);
    delay(1000);
    
    // Initialize RGB LED (FeatherS3)
    init_led();
    boot_led_sequence();
    
    printf("Starting Flock Squawk Enhanced Detection System...\n\n");
    
    // Initialize Configuration Manager (loads from NVS or uses defaults)
    printf("[Config] Initializing configuration manager...\n");
    configManager.begin();
    
    // Initialize WiFi in promiscuous mode for surveillance device detection
    printf("[WiFi] Initializing promiscuous scanning mode...\n");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler);
    esp_wifi_set_channel(current_channel, WIFI_SECOND_CHAN_NONE);
    
    printf("WiFi promiscuous mode enabled on channel %d\n", current_channel);
    printf("Monitoring probe requests and beacons...\n");
    
    // Initialize BLE with device name for iOS app discovery
    printf("Initializing BLE...\n");
    NimBLEDevice::init("FlockFinder-S3");
    
    // Initialize BLE broadcast service for iOS app connection
    initBLEBroadcast();
    
    // Set config update callback
    setConfigUpdatedCallback(onConfigurationUpdated);
    
    // Initialize BLE scanner for detecting surveillance devices
    pBLEScan = NimBLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    
    printf("BLE scanner initialized\n");
    printf("System ready - hunting for surveillance devices...\n");
    printf("iOS app can connect via Bluetooth to 'FlockFinder-S3'\n\n");
    
    last_channel_hop = millis();
}

void loop()
{
    ScanConfig& cfg = configManager.getScanConfig();
    
    // Handle channel hopping for WiFi promiscuous mode
    hop_channel();
    
    // Handle heartbeat pulse if device is in range
    if (device_in_range) {
        unsigned long now = millis();
        
        // Check if heartbeat interval has passed
        if (now - last_heartbeat >= cfg.heartbeatInterval) {
            heartbeat_pulse();
            last_heartbeat = now;
        }
        
        // Check if device has gone out of range (no detection for timeout period)
        if (now - last_detection_time >= cfg.detectionTimeout) {
            printf("Device out of range - stopping heartbeat\n");
            device_in_range = false;
            triggered = false; // Allow new detections
            // Return to scanning color
            pixel.setPixelColor(0, COLOR_SCANNING);
            pixel.show();
        }
    }
    
    // BLE scanning with dynamic interval
    if (millis() - last_ble_scan >= cfg.bleScanInterval && !pBLEScan->isScanning()) {
        streamStatus("BLE scan starting...");
        pBLEScan->start(cfg.bleScanDuration, false);
        last_ble_scan = millis();
    }
    
    if (pBLEScan->isScanning() == false && millis() - last_ble_scan > cfg.bleScanDuration * 1000) {
        pBLEScan->clearResults();
    }
    
    delay(100);
}
