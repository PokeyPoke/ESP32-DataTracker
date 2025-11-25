#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <qrcode.h>

// Display states
enum DisplayState {
    SPLASH,       // Boot logo
    CONNECTING,   // "Connecting to WiFi..."
    CONFIG_MODE,  // "Setup: DataTracker-XXXX"
    NORMAL,       // Showing metric data
    ERROR_STATE   // Error message display
};

// Config QR states
enum ConfigQRState {
    WAITING_FOR_CLIENT,   // Show WiFi QR
    CLIENT_CONNECTED      // Show URL QR
};

class DisplayManager {
private:
    U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;
    DisplayState currentState;
    uint8_t currentBrightness;
    uint8_t savedBrightness;  // Brightness before screensaver
    bool brightnessIncreasing;

    // Screensaver state
    bool screensaverActive;
    unsigned long lastActivityTime;
    unsigned long screensaverTimeout;

    // Helper drawing functions
    void drawCenteredText(const char* text, int y, const uint8_t* font);
    void drawCenteredValue(const char* value, int y);
    void drawStatusBar(bool wifiConnected, unsigned long lastUpdate, bool isStale, const char* label = nullptr);
    void drawHeader(const char* title);
    void drawQRCode(const char* data, int x, int y, int scale);

public:
    DisplayManager();

    void init();
    void clear();

    // State-specific display functions
    void showSplash();
    void showConnecting(const char* ssid);
    void showConfigMode(const char* apName);
    void showError(const char* message);

    // Module-specific display functions
    void showCrypto(const char* moduleId, float price, float change24h, unsigned long lastUpdate, bool stale);
    void showStock(const char* ticker, float price, float change, unsigned long lastUpdate, bool stale);
    void showWeather(float temp, const char* condition, const char* location, unsigned long lastUpdate, bool stale);
    void showCustom(float value, const char* label, const char* unit, unsigned long lastUpdate);
    void showCustomValue(const char* valueStr, const char* label, const char* unit, unsigned long lastUpdate);
    void showTransit(const char* routeName, float nextArrival, const char* stopName, unsigned long lastUpdate, bool stale);

    // Generic module display
    void showModule(const char* moduleId);

    // Button debug
    void showButtonStatus(bool isPressed, int digitalValue, int analogValue);

    // Adaptive QR configuration
    void showWiFiQR(const char* ssid, const char* password);
    void showURLQR();

    // Settings module
    void showSettings(uint32_t securityCode, const char* deviceIP, unsigned long timeRemaining);

    // Quad screen module (2x2 grid)
    void showQuadScreen(const char* slot1, const char* slot2, const char* slot3, const char* slot4, unsigned long lastUpdate, bool stale);

    // Loading state with progress bar
    void showModuleLoading(const char* moduleName, int progress);

    // Brightness control
    void setBrightness(uint8_t level);  // 0-255
    void cycleBrightness();             // Ping-pong cycle
    uint8_t getBrightness();

    // Screensaver management
    void updateActivity();              // Reset activity timer (call on user interaction)
    void checkScreensaver();            // Check and activate/deactivate screensaver
    void setScreensaverTimeout(unsigned long timeoutMs);  // Set timeout in milliseconds
    bool isScreensaverActive();         // Check if screensaver is currently active
    void disableScreensaver();          // Temporarily disable screensaver
    void enableScreensaver();           // Re-enable screensaver
};

#endif // DISPLAY_H
