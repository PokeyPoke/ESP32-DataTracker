#include "config.h"

// Global configuration document (StaticJsonDocument allocated in .bss, not heap)
// Increased to 8KB to support 15-20 dynamic module instances
StaticJsonDocument<8192> config;

// Track last save time to reduce flash wear
static unsigned long lastSaveTime = 0;
#define MIN_SAVE_INTERVAL 30000  // Minimum 30s between saves

bool initStorage() {
    Serial.println("Initializing LittleFS...");

    // Try to mount first
    if (!LittleFS.begin(false)) {
        Serial.println("LittleFS mount failed, formatting...");
        // Format if mount fails
        if (!LittleFS.begin(true)) {
            Serial.println("ERROR: LittleFS format failed");
            return false;
        }
        Serial.println("LittleFS formatted successfully");
    }

    Serial.println("LittleFS mounted successfully");

    // Create config file if it doesn't exist
    if (!LittleFS.exists(CONFIG_FILE)) {
        Serial.println("Creating default config file...");
        File file = LittleFS.open(CONFIG_FILE, "w");
        if (file) {
            file.print("{}");  // Empty JSON object
            file.close();
            Serial.println("Default config file created");
        }
    }

    return true;
}

bool loadConfiguration() {
    File file = LittleFS.open(CONFIG_FILE, "r");
    if (!file) {
        Serial.println("Config file not found, creating default config");
        setDefaultConfig();
        saveConfiguration();
        return false;
    }

    DeserializationError error = deserializeJson(config, file);
    file.close();

    if (error) {
        Serial.print("Failed to parse config: ");
        Serial.println(error.c_str());
        setDefaultConfig();
        saveConfiguration();
        return false;
    }

    Serial.println("Configuration loaded successfully");

    // Check if essential fields exist, populate defaults if missing
    bool needsDefaults = false;
    if (!config.containsKey("device") || !config.containsKey("modules")) {
        Serial.println("Config missing essential fields, populating defaults...");
        needsDefaults = true;
    }

    // Check if crypto modules have required fields
    if (config.containsKey("modules")) {
        JsonObject bitcoin = config["modules"]["bitcoin"];
        if (!bitcoin.containsKey("cryptoId")) {
            Serial.println("Bitcoin module missing crypto fields");
            needsDefaults = true;
        }
    } else {
        needsDefaults = true;
    }

    if (needsDefaults) {
        setDefaultConfig();
        saveConfiguration(true);  // Force save
        Serial.println("Default config populated and saved");
    }

    // Debug: Show what was loaded for crypto and weather modules
    Serial.println("=== Loaded Config (Modules) ===");
    JsonObject crypto = config["modules"]["bitcoin"];
    Serial.print("Crypto 1 - ID: ");
    Serial.print(crypto["cryptoId"] | "NOT SET");
    Serial.print(", Name: ");
    Serial.print(crypto["cryptoName"] | "NOT SET");
    Serial.print(", Symbol: ");
    Serial.println(crypto["cryptoSymbol"] | "NOT SET");

    JsonObject weather = config["modules"]["weather"];
    Serial.print("Weather - Location: ");
    Serial.print(weather["location"] | "NOT SET");
    Serial.print(", Lat: ");
    Serial.print(weather["latitude"] | 0.0, 6);
    Serial.print(", Lon: ");
    Serial.println(weather["longitude"] | 0.0, 6);
    Serial.println("=======================================");

    // Check if config is overflowing
    Serial.print("Config memory usage: ");
    Serial.print(config.memoryUsage());
    Serial.print(" / ");
    Serial.print(config.capacity());
    Serial.println(" bytes");
    if (config.overflowed()) {
        Serial.println("WARNING: Config document overflowed! Some data may be lost!");
    }

    return true;
}

bool saveConfiguration(bool force) {
    // Throttle saves to reduce flash wear (unless forced)
    unsigned long now = millis();
    if (!force && now - lastSaveTime < MIN_SAVE_INTERVAL) {
        Serial.println("Skipping save (too soon since last save)");
        return true;  // Not an error, just throttled
    }

    if (force) {
        Serial.println("FORCED SAVE - bypassing throttle");
    }

    File file = LittleFS.open(CONFIG_FILE, "w");
    if (!file) {
        Serial.println("ERROR: Failed to open config file for writing");
        return false;
    }

    if (serializeJson(config, file) == 0) {
        Serial.println("ERROR: Failed to write config");
        file.close();
        return false;
    }

    file.close();
    lastSaveTime = now;
    Serial.println("Configuration saved successfully");

    // Debug: Show what was saved for crypto and weather modules
    Serial.println("=== Saved Config (Modules) ===");
    JsonObject crypto = config["modules"]["bitcoin"];
    Serial.print("Crypto 1 - ID: ");
    Serial.print(crypto["cryptoId"] | "NOT SET");
    Serial.print(", Name: ");
    Serial.print(crypto["cryptoName"] | "NOT SET");
    Serial.print(", Symbol: ");
    Serial.println(crypto["cryptoSymbol"] | "NOT SET");

    JsonObject weather = config["modules"]["weather"];
    Serial.print("Weather - Location: ");
    Serial.print(weather["location"] | "NOT SET");
    Serial.print(", Lat: ");
    Serial.print(weather["latitude"] | 0.0, 6);
    Serial.print(", Lon: ");
    Serial.println(weather["longitude"] | 0.0, 6);
    Serial.println("======================================");

    // Check if config is overflowing
    Serial.print("Config memory usage: ");
    Serial.print(config.memoryUsage());
    Serial.print(" / ");
    Serial.print(config.capacity());
    Serial.println(" bytes");
    if (config.overflowed()) {
        Serial.println("ERROR: Config document overflowed during save! Data loss occurred!");
    }

    return true;
}

void setDefaultConfig() {
    // Clear existing config
    config.clear();

    // WiFi settings
    config["wifi"]["ssid"] = "";
    config["wifi"]["password"] = "";

    // Device settings
    config["device"]["activeModule"] = "bitcoin";
    config["device"]["enableButton"] = true;
    config["device"]["refreshInterval"] = 300;  // 5 minutes default
    config["device"]["currency"] = "USD";        // Default currency for all modules

    // Module display order (array of module IDs in display order)
    JsonArray moduleOrder = config["device"]["moduleOrder"].to<JsonArray>();
    moduleOrder.add("bitcoin");
    moduleOrder.add("stock");
    moduleOrder.add("weather");
    moduleOrder.add("custom");
    moduleOrder.add("settings");

    // Initialize empty module data
    JsonObject bitcoin = config["modules"]["bitcoin"].to<JsonObject>();
    bitcoin["type"] = "crypto";              // Module type for future dynamic creation
    bitcoin["cryptoId"] = "bitcoin";
    bitcoin["cryptoSymbol"] = "BTC";
    bitcoin["cryptoName"] = "Bitcoin";
    bitcoin["value"] = 0.0;
    bitcoin["change24h"] = 0.0;
    bitcoin["lastUpdate"] = 0;
    bitcoin["lastSuccess"] = false;
    bitcoin["refreshInterval"] = 300;        // 5 minutes default

    JsonObject stock = config["modules"]["stock"].to<JsonObject>();
    stock["type"] = "stock";                 // Module type for future dynamic creation
    stock["ticker"] = "AAPL";
    stock["name"] = "Apple Inc.";
    stock["value"] = 0.0;
    stock["change"] = 0.0;
    stock["lastUpdate"] = 0;
    stock["lastSuccess"] = false;
    stock["refreshInterval"] = 300;          // 5 minutes default

    JsonObject weather = config["modules"]["weather"].to<JsonObject>();
    weather["type"] = "weather";             // Module type for future dynamic creation
    weather["latitude"] = 37.7749;
    weather["longitude"] = -122.4194;
    weather["location"] = "";                // Empty = IP-based auto-location
    weather["detectedLocation"] = "";        // Populated by auto-location
    weather["temperature"] = 0.0;
    weather["condition"] = "Unknown";
    weather["lastUpdate"] = 0;
    weather["lastSuccess"] = false;
    weather["refreshInterval"] = 900;        // 15 minutes default for weather

    JsonObject custom = config["modules"]["custom"].to<JsonObject>();
    custom["type"] = "custom";
    custom["value"] = 0.0;
    custom["label"] = "My Metric";
    custom["unit"] = "units";
    custom["decimals"] = -1;
    custom["lastUpdate"] = 0;
    custom["lastSuccess"] = true;
    custom["refreshInterval"] = 0;

    JsonObject countdown = config["modules"]["countdown"].to<JsonObject>();
    countdown["type"] = "countdown";
    countdown["label"] = "Event";
    countdown["targetDate"] = "";
    countdown["targetTime"] = "00:00";
    countdown["countdownType"] = "date";
    countdown["value"] = 0.0;
    countdown["decimals"] = -1;
    countdown["lastUpdate"] = 0;
    countdown["lastSuccess"] = false;
    countdown["refreshInterval"] = 3600;

    JsonObject http = config["modules"]["http"].to<JsonObject>();
    http["type"] = "http";
    http["label"] = "API Data";
    http["url"] = "";
    http["jsonPath"] = "";
    http["unit"] = "";
    http["value"] = 0.0;
    http["lastUpdate"] = 0;
    http["lastSuccess"] = false;
    http["refreshInterval"] = 300;

    JsonObject settings = config["modules"]["settings"].to<JsonObject>();
    settings["type"] = "settings";           // Module type for future dynamic creation
    settings["securityCode"] = 0;            // Will be generated by SecurityManager on first display
    settings["codeTimeRemaining"] = 0;
    settings["lastUpdate"] = 0;
    settings["lastSuccess"] = true;

    // Custom transit agencies (user-addable)
    JsonArray customAgencies = config["customAgencies"].to<JsonArray>();
    // Empty by default - users can add their own agencies via web UI

    Serial.println("Default configuration created");
}

void updateModuleCache(const char* moduleId, JsonObject data) {
    JsonObject module = config["modules"][moduleId];

    // Update timestamp
    module["lastUpdate"] = millis() / 1000;
    module["lastSuccess"] = true;

    // Copy all data fields
    for (JsonPair kv : data) {
        module[kv.key()] = kv.value();
    }

    // Request save (will be throttled if too frequent)
    saveConfiguration();
}

bool isCacheStale(const char* moduleId) {
    JsonObject module = config["modules"][moduleId];
    unsigned long lastUpdate = module["lastUpdate"] | 0;
    unsigned long now = millis() / 1000;

    // Get module-specific refresh interval, fall back to global default
    uint16_t refreshInterval = module["refreshInterval"] | 0;
    if (refreshInterval == 0) {
        refreshInterval = config["device"]["refreshInterval"] | 300;
    }

    // Cache is stale if older than 2× refresh interval
    return (now - lastUpdate) > (refreshInterval * 2);
}

unsigned long getCacheAge(const char* moduleId) {
    JsonObject module = config["modules"][moduleId];
    unsigned long lastUpdate = module["lastUpdate"] | 0;
    unsigned long now = millis() / 1000;
    return now - lastUpdate;
}

String getTimeAgo(unsigned long timestamp) {
    if (timestamp == 0) return "Never";

    unsigned long now = millis() / 1000;
    unsigned long diff = now - timestamp;

    if (diff < 60) return String(diff) + "s ago";
    if (diff < 3600) return String(diff / 60) + "m ago";
    if (diff < 86400) return String(diff / 3600) + "h ago";
    return String(diff / 86400) + "d ago";
}
