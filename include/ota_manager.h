#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <Update.h>

/**
 * OTA Manager - Handles Over-The-Air firmware updates
 *
 * Provides two update methods:
 * 1. ArduinoOTA - Network-based OTA via Arduino IDE or platformio
 * 2. HTTP OTA - Web-based firmware upload through settings interface
 */
class OTAManager {
private:
    bool otaEnabled;
    bool updateInProgress;
    int updateProgress;
    String updateError;

    // Callbacks for update status
    void (*onProgressCallback)(int progress);
    void (*onCompleteCallback)();
    void (*onErrorCallback)(const char* error);

public:
    OTAManager();

    // Arduino OTA setup (for IDE/platformio updates)
    void begin(const char* hostname);
    void handle();  // Call in main loop
    void end();

    // HTTP OTA methods (for web-based updates)
    bool beginHTTPUpdate(size_t contentLength);
    bool writeHTTPUpdate(uint8_t* data, size_t len);
    bool endHTTPUpdate();

    // Status methods
    bool isUpdateInProgress() { return updateInProgress; }
    int getProgress() { return updateProgress; }
    String getError() { return updateError; }

    // Callback setters
    void onProgress(void (*callback)(int progress)) { onProgressCallback = callback; }
    void onComplete(void (*callback)()) { onCompleteCallback = callback; }
    void onError(void (*callback)(const char* error)) { onErrorCallback = callback; }

    // Utility methods
    String getCurrentVersion();
    String getSketchMD5();
    size_t getFreeSketchSpace();
};

#endif // OTA_MANAGER_H
