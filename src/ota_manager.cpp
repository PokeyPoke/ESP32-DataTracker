#include "ota_manager.h"
#include "constants.h"
#include "version.h"
#include <Arduino.h>

OTAManager::OTAManager()
    : otaEnabled(false), updateInProgress(false), updateProgress(0),
      onProgressCallback(nullptr), onCompleteCallback(nullptr), onErrorCallback(nullptr) {
}

void OTAManager::begin(const char* hostname) {
    Serial.println("Initializing OTA updates...");

    // Set hostname for identification on network
    ArduinoOTA.setHostname(hostname);

    // Set password (optional - can be configured)
    // ArduinoOTA.setPassword("admin");

    // OTA callbacks
    ArduinoOTA.onStart([this]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else { // U_SPIFFS
            type = "filesystem";
        }

        Serial.println("OTA: Start updating " + type);
        updateInProgress = true;
        updateProgress = 0;
    });

    ArduinoOTA.onEnd([this]() {
        Serial.println("\nOTA: Update complete!");
        updateInProgress = false;
        updateProgress = 100;

        if (onCompleteCallback) {
            onCompleteCallback();
        }
    });

    ArduinoOTA.onProgress([this](unsigned int progress, unsigned int total) {
        updateProgress = (progress * 100) / total;
        Serial.printf("OTA Progress: %u%%\r", updateProgress);

        if (onProgressCallback) {
            onProgressCallback(updateProgress);
        }
    });

    ArduinoOTA.onError([this](ota_error_t error) {
        const char* errorMsg;

        switch (error) {
            case OTA_AUTH_ERROR:
                errorMsg = "Auth Failed";
                break;
            case OTA_BEGIN_ERROR:
                errorMsg = "Begin Failed";
                break;
            case OTA_CONNECT_ERROR:
                errorMsg = "Connect Failed";
                break;
            case OTA_RECEIVE_ERROR:
                errorMsg = "Receive Failed";
                break;
            case OTA_END_ERROR:
                errorMsg = "End Failed";
                break;
            default:
                errorMsg = "Unknown Error";
                break;
        }

        Serial.printf("OTA Error[%u]: %s\n", error, errorMsg);
        updateError = String(errorMsg);
        updateInProgress = false;

        if (onErrorCallback) {
            onErrorCallback(errorMsg);
        }
    });

    ArduinoOTA.begin();
    otaEnabled = true;

    Serial.println("OTA ready");
    Serial.print("  Hostname: ");
    Serial.println(hostname);
    Serial.print("  IP: ");
    Serial.println(WiFi.localIP());
}

void OTAManager::handle() {
    if (otaEnabled) {
        ArduinoOTA.handle();
    }
}

void OTAManager::end() {
    if (otaEnabled) {
        ArduinoOTA.end();
        otaEnabled = false;
    }
}

// HTTP OTA implementation (for web-based uploads)
bool OTAManager::beginHTTPUpdate(size_t contentLength) {
    Serial.printf("Starting HTTP OTA update, size: %d bytes\n", contentLength);

    if (contentLength == 0) {
        updateError = "Empty content";
        return false;
    }

    // Check if there's enough space
    size_t freeSpace = getFreeSketchSpace();
    if (contentLength > freeSpace) {
        updateError = "Not enough space";
        Serial.printf("ERROR: Update size (%d) exceeds free space (%d)\n",
                     contentLength, freeSpace);
        return false;
    }

    // Begin update
    if (!Update.begin(contentLength)) {
        updateError = Update.errorString();
        Serial.printf("ERROR: Update.begin() failed: %s\n", updateError.c_str());
        return false;
    }

    updateInProgress = true;
    updateProgress = 0;
    updateError = "";

    Serial.println("HTTP OTA update started");
    return true;
}

bool OTAManager::writeHTTPUpdate(uint8_t* data, size_t len) {
    if (!updateInProgress) {
        return false;
    }

    size_t written = Update.write(data, len);

    if (written != len) {
        updateError = "Write failed";
        Serial.printf("ERROR: Write mismatch - expected %d, wrote %d\n", len, written);
        return false;
    }

    // Update progress
    updateProgress = (Update.progress() * 100) / Update.size();

    if (onProgressCallback && updateProgress % 10 == 0) {
        onProgressCallback(updateProgress);
    }

    return true;
}

bool OTAManager::endHTTPUpdate() {
    if (!updateInProgress) {
        return false;
    }

    if (!Update.end(true)) {
        updateError = Update.errorString();
        Serial.printf("ERROR: Update.end() failed: %s\n", updateError.c_str());
        updateInProgress = false;

        if (onErrorCallback) {
            onErrorCallback(updateError.c_str());
        }
        return false;
    }

    updateInProgress = false;
    updateProgress = 100;

    Serial.println("HTTP OTA update completed successfully");
    Serial.println("Rebooting in 3 seconds...");

    if (onCompleteCallback) {
        onCompleteCallback();
    }

    return true;
}

String OTAManager::getCurrentVersion() {
    return String(FIRMWARE_VERSION);
}

String OTAManager::getSketchMD5() {
    return ESP.getSketchMD5();
}

size_t OTAManager::getFreeSketchSpace() {
    return ESP.getFreeSketchSpace();
}
