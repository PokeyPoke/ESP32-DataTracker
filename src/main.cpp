#include <Arduino.h>
#include "constants.h"
#include "config.h"
#include "display.h"
#include "network.h"
#include "scheduler.h"
#include "button.h"
#include "security.h"
#include "version.h"
#include "modules/module_interface.h"
#include "module_factory.h"
#include <time.h>

// Global objects
DisplayManager display;
NetworkManager network;
Scheduler scheduler;
SecurityManager security;

#ifdef ENABLE_BUTTON
ButtonHandler button(BUTTON_PIN);
#endif

// State management
bool configMode = false;
ConfigQRState qrState = WAITING_FOR_CLIENT;  // Track QR display state
bool brightnessMode = false;  // Brightness adjustment mode
bool buttonDebugMode = false;  // Button debug mode - disabled by default (use 'button' command to enable)
unsigned long buttonDebugStartTime = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastSerialCheck = 0;
unsigned long lastSettingsCodeRefresh = 0;
String lastDisplayedModule = "";  // Track which module is currently shown
// Timing constants now defined in constants.h

// Function prototypes
void handleButtonEvent(ButtonEvent event);
void cycleToNextModule();
void handleSerialCommand();

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\n=== " VERSION_STRING " ===");
    Serial.println("Build: " BUILD_NAME " - " BUILD_DATE);
    Serial.println("Initializing...\n");

    // Initialize storage
    if (!initStorage()) {
        Serial.println("FATAL ERROR: Storage initialization failed");
        while(1) {
            delay(1000);
        }
    }

    // Load configuration
    loadConfiguration();

    // Initialize display
    display.init();
    Serial.println("Display initialized");

    // Load and apply saved brightness
    uint8_t savedBrightness = config["device"]["brightness"] | 255;
    display.setBrightness(savedBrightness);
    Serial.print("Loaded brightness: ");
    Serial.println(savedBrightness);

    // Initialize button (if enabled)
    #ifdef ENABLE_BUTTON
    if (config["device"]["enableButton"] | true) {
        button.init();
        Serial.println("Button support enabled");
    }
    #endif

    // Setup WiFi
    String ssid = config["wifi"]["ssid"] | "";
    if (ssid.length() == 0) {
        // No WiFi configured → Start AP mode
        Serial.println("No WiFi configuration found");
        Serial.println("Starting configuration AP mode...");
        configMode = true;
        qrState = WAITING_FOR_CLIENT;
        network.startConfigAP();
        // Show WiFi QR code with credentials
        display.showWiFiQR(network.getAPName().c_str(), network.getAPPassword().c_str());
    } else {
        // Connect to WiFi
        Serial.print("Connecting to WiFi: ");
        Serial.println(ssid);
        display.showConnecting(ssid.c_str());

        if (network.connectWiFi(ssid.c_str(), config["wifi"]["password"])) {
            Serial.println("WiFi connected successfully!");
            configMode = false;

            // Initialize NTP time synchronization
            Serial.println("Syncing time via NTP...");
            configTime(0, 0, "pool.ntp.org", "time.nist.gov");

            // Wait for time to be set (with timeout)
            int ntpRetries = 0;
            struct tm timeinfo;
            while (!getLocalTime(&timeinfo) && ntpRetries < 10) {
                Serial.print(".");
                delay(500);
                ntpRetries++;
            }

            if (getLocalTime(&timeinfo)) {
                Serial.println("\nTime synced successfully!");
                Serial.print("Current time: ");
                Serial.println(&timeinfo, "%Y-%m-%d %H:%M:%S");
            } else {
                Serial.println("\nWarning: Time sync failed - countdown timer may not work");
            }

            // Start settings web server (always available on local network)
            network.startSettingsServer();

            // Initialize scheduler and load modules dynamically from config
            Serial.println("\n=== Initializing Scheduler ===");
            scheduler.init();
            scheduler.loadModulesFromConfig();

            // TEMPORARY DEBUG: Always fetch stock at startup to verify it works
            Serial.println("\n*** TEMPORARY DEBUG: Force-fetching STOCK module at startup ***");
            scheduler.requestFetch("stock", true);
            delay(2000);
            Serial.println("*** DEBUG: Check if stock lastUpdate changed ***\n");

            // Force initial fetch of active module
            String activeModule = config["device"]["activeModule"] | "bitcoin";
            Serial.print("Active module: ");
            Serial.println(activeModule);
            Serial.println("Requesting forced fetch at startup...");
            scheduler.requestFetch(activeModule.c_str(), true);
            Serial.println("Startup fetch requested");
        } else {
            Serial.println("WiFi connection failed");
            Serial.println("Starting configuration AP mode...");
            configMode = true;
            qrState = WAITING_FOR_CLIENT;
            network.startConfigAP();
            // Show WiFi QR code with credentials
            display.showWiFiQR(network.getAPName().c_str(), network.getAPPassword().c_str());
        }
    }

    Serial.println("\n=== Setup Complete ===");
    Serial.println("Type 'help' for available commands\n");
}

void loop() {
    unsigned long now = millis();

    // Handle config mode with adaptive QR display
    if (configMode) {
        // Check for client connection changes every 500ms
        static unsigned long lastQRCheck = 0;
        if (now - lastQRCheck > QR_UPDATE_INTERVAL) {
            bool clientConnected = network.hasClientConnected();

            // State transition: no client → client connected
            if (clientConnected && qrState == WAITING_FOR_CLIENT) {
                qrState = CLIENT_CONNECTED;
                Serial.println("\n✓ Client connected! Showing URL QR");
                display.showURLQR();
            }
            // State transition: client connected → no client
            else if (!clientConnected && qrState == CLIENT_CONNECTED) {
                qrState = WAITING_FOR_CLIENT;
                Serial.println("\n⚠ Client disconnected. Showing WiFi QR");
                display.showWiFiQR(network.getAPName().c_str(), network.getAPPassword().c_str());
            }

            lastQRCheck = now;
        }

        network.handleClient();
        return;
    }

    // Check serial commands
    if (now - lastSerialCheck > SERIAL_CHECK_INTERVAL) {
        handleSerialCommand();
        lastSerialCheck = now;
    }

    // Button debug mode - show button status on display
    if (buttonDebugMode) {
        #ifdef ENABLE_BUTTON
        // Auto-disable after timeout
        if (millis() - buttonDebugStartTime > BUTTON_DEBUG_DURATION) {
            buttonDebugMode = false;
            Serial.println("\n*** Button debug auto-disabled after 30s ***");
            Serial.println("Type 'button' to re-enable\n");
            return;
        }

        int digitalVal = digitalRead(BUTTON_PIN);
        int analogVal = analogRead(BUTTON_PIN);
        bool pressed = analogVal > 2000;  // Use analog threshold (works when digital doesn't)

        display.showButtonStatus(pressed, digitalVal, analogVal);
        delay(50);  // Update frequently in debug mode
        #endif
        return;
    }

    // Handle button (if enabled)
    #ifdef ENABLE_BUTTON
    if (config["device"]["enableButton"] | true) {
        ButtonEvent event = button.check();
        if (event != NONE) {
            handleButtonEvent(event);
        }
    }
    #endif

    // No auto-timeout - manual exit only with long press

    // Monitor WiFi connection
    if (!network.isConnected()) {
        network.reconnect();
    }

    // Handle settings web server requests (in normal operation mode)
    network.handleClient();

    // Run scheduler (fetch data if needed)
    scheduler.tick();

    // Update display
    if (brightnessMode) {
        // In brightness mode: keep showing brightness screen (no need to update frequently)
        // The brightness screen stays visible until mode exits
    } else {
        // Normal mode: show modules
        String activeModule = config["device"]["activeModule"] | "bitcoin";
        bool moduleChanged = (activeModule != lastDisplayedModule);

        // Settings module: refresh security code every 30 seconds
        if (activeModule == "settings") {
            if (moduleChanged) {
                // First time showing settings - generate code immediately
                scheduler.requestFetch("settings", true);
                lastSettingsCodeRefresh = now;
                display.showModule(activeModule.c_str());
                lastDisplayedModule = activeModule;
                lastDisplayUpdate = now;
            } else if (now - lastSettingsCodeRefresh > SETTINGS_CODE_REFRESH) {
                // Refresh code every 30 seconds while on settings screen
                Serial.println("Refreshing security code (30s interval)");
                scheduler.requestFetch("settings", true);
                lastSettingsCodeRefresh = now;
                display.showModule(activeModule.c_str());
                lastDisplayUpdate = now;
            }
        } else {
            // Other modules: update every 1 second for real-time data
            bool shouldUpdate = moduleChanged || (now - lastDisplayUpdate > DISPLAY_UPDATE_INTERVAL);
            if (shouldUpdate) {
                display.showModule(activeModule.c_str());
                lastDisplayedModule = activeModule;
                lastDisplayUpdate = now;
            }
        }
    }

    // Small delay to prevent watchdog
    delay(10);
}

void handleButtonEvent(ButtonEvent event) {
    switch (event) {
        case SHORT_PRESS:
            if (brightnessMode) {
                // In brightness mode: cycle brightness level
                display.cycleBrightness();
            } else {
                // Normal mode: cycle to next module
                cycleToNextModule();
            }
            break;

        case LONG_PRESS:
            // Toggle brightness mode
            brightnessMode = !brightnessMode;

            if (brightnessMode) {
                // Entering brightness mode
                Serial.println("Entering brightness mode");
                display.cycleBrightness();
            } else {
                // Exiting brightness mode
                Serial.println("Exiting brightness mode");
                config["device"]["brightness"] = display.getBrightness();
                saveConfiguration();
            }
            break;

        default:
            break;
    }
}

void cycleToNextModule() {
    // Get module order from config (supports dynamic module management)
    JsonArray moduleOrder = config["device"]["moduleOrder"];
    int moduleCount = moduleOrder.size();

    // Fallback if moduleOrder is empty (shouldn't happen with default config)
    if (moduleCount == 0) {
        Serial.println("ERROR: moduleOrder is empty, cannot cycle");
        return;
    }

    Serial.print("DEBUG: Total modules available: ");
    Serial.println(moduleCount);
    Serial.print("DEBUG: Module order: ");
    for (int i = 0; i < moduleCount; i++) {
        Serial.print(moduleOrder[i].as<const char*>());
        if (i < moduleCount - 1) Serial.print(", ");
    }
    Serial.println();

    // Find current module index
    String currentModule = config["device"]["activeModule"] | "bitcoin";
    int currentIndex = -1;
    for (int i = 0; i < moduleCount; i++) {
        if (currentModule == moduleOrder[i].as<String>()) {
            currentIndex = i;
            break;
        }
    }

    // If current module not in order, start from beginning
    if (currentIndex == -1) {
        currentIndex = 0;
        Serial.println("DEBUG: Current module not in order, starting from beginning");
    }

    Serial.print("DEBUG: Current index: ");
    Serial.println(currentIndex);

    // Cycle to next (with wraparound)
    int nextIndex = (currentIndex + 1) % moduleCount;
    String nextModule = moduleOrder[nextIndex].as<String>();

    Serial.print("DEBUG: Next index: ");
    Serial.println(nextIndex);
    Serial.print("Cycling from ");
    Serial.print(currentModule);
    Serial.print(" to ");
    Serial.println(nextModule);

    config["device"]["activeModule"] = nextModule;

    // Note: Settings code generation is now handled in main loop
    // Display will be updated automatically on next loop iteration

    // Schedule fetch if cache is stale (for non-settings modules)
    if (nextModule != "settings") {
        scheduler.requestFetch(nextModule.c_str(), false);
    }

    // Save active module to config (throttled)
    saveConfiguration();
}

void handleSerialCommand() {
    if (!Serial.available()) return;

    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();

    if (cmd.length() == 0) return;

    if (cmd == "help") {
        Serial.println("\n=== Available Commands ===");
        Serial.println("help      - Show this help message");
        Serial.println("config    - Show current configuration");
        Serial.println("wifi      - Show WiFi status");
        Serial.println("fetch     - Force fetch now");
        Serial.println("cache     - Show all cached values");
        Serial.println("reset     - Factory reset");
        Serial.println("restart   - Reboot device");
        Serial.println("modules   - List available modules");
        Serial.println("switch    - Switch to next module");
        Serial.println("button    - Toggle button debug mode (shows on display)");
        Serial.println("==========================\n");
    }
    else if (cmd == "config") {
        Serial.println("\n=== Current Configuration ===");
        serializeJsonPretty(config, Serial);
        Serial.println("\n=============================\n");
    }
    else if (cmd == "wifi") {
        Serial.println("\n=== WiFi Status ===");
        if (network.isConnected()) {
            Serial.print("Status: CONNECTED\n");
            Serial.print("SSID: ");
            Serial.println(WiFi.SSID());
            Serial.print("IP: ");
            Serial.println(WiFi.localIP());
            Serial.print("RSSI: ");
            Serial.print(WiFi.RSSI());
            Serial.println(" dBm");
        } else {
            Serial.println("Status: DISCONNECTED");
        }
        Serial.println("===================\n");
    }
    else if (cmd == "fetch") {
        String activeModule = config["device"]["activeModule"] | "bitcoin";
        Serial.print("Forcing fetch for: ");
        Serial.println(activeModule);
        scheduler.requestFetch(activeModule.c_str(), true);
    }
    else if (cmd == "cache") {
        Serial.println("\n=== Cached Module Data ===");
        JsonObject modules = config["modules"];
        for (JsonPair kv : modules) {
            Serial.print(kv.key().c_str());
            Serial.print(": ");
            serializeJson(kv.value(), Serial);
            Serial.println();
        }
        Serial.println("==========================\n");
    }
    else if (cmd == "reset") {
        Serial.println("\nFactory reset in 3 seconds...");
        Serial.println("Press Ctrl+C to cancel\n");
        delay(3000);
        Serial.println("Formatting filesystem...");
        LittleFS.format();
        Serial.println("Restarting...");
        ESP.restart();
    }
    else if (cmd == "restart") {
        Serial.println("\nRestarting device...\n");
        delay(500);
        ESP.restart();
    }
    else if (cmd == "modules") {
        Serial.println("\n=== Available Modules ===");
        Serial.println("1. bitcoin  - Crypto Price (configurable)");
        Serial.println("2. stock    - Stock Price (configurable)");
        Serial.println("3. weather  - Weather Data (configurable)");
        Serial.println("4. custom   - Custom Number (manual entry)");
        Serial.println("5. settings - Settings & Configuration");
        Serial.println("=========================\n");
    }
    else if (cmd == "switch") {
        cycleToNextModule();
    }
    else if (cmd == "button") {
        if (buttonDebugMode) {
            // Disable debug mode
            buttonDebugMode = false;
            Serial.println("\n=== Button Debug Mode DISABLED ===");
            Serial.println("Returning to normal operation\n");
        } else {
            // Enable debug mode
            buttonDebugMode = true;
            buttonDebugStartTime = millis();  // Reset timer
            Serial.println("\n=== Button Debug Mode ENABLED ===");
            Serial.println("Watch the DISPLAY - it will show:");
            Serial.println("- ON/OFF status (large text)");
            Serial.println("- Digital value (0=LOW, 1=HIGH)");
            Serial.println("- Analog value (0-4095)");
            Serial.println();
            Serial.println("Expected:");
            Serial.println("- NOT touched: Should show 'OFF'");
            Serial.println("- Touched: Should show 'ON'");
            Serial.println("- Red LED: Indicates touch module is active");
            Serial.println();
            Serial.println("Type 'button' again to exit debug mode");
            Serial.println("Auto-disables after 30 seconds");
            Serial.println("==================================\n");
        }
    }
    else {
        Serial.print("Unknown command: ");
        Serial.println(cmd);
        Serial.println("Type 'help' for available commands");
    }
}
