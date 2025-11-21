#ifndef MODULE_INTERFACE_H
#define MODULE_INTERFACE_H

#include <Arduino.h>
#include <ArduinoJson.h>

// Base interface for all metric modules
class ModuleInterface {
public:
    // Module identity
    const char* id;
    const char* displayName;

    // Timing constraints
    uint16_t defaultRefreshInterval;  // seconds
    uint16_t minRefreshInterval;      // seconds

    virtual ~ModuleInterface() {}

    // Core functions that all modules must implement
    virtual bool fetch(String& errorMsg) = 0;
    virtual String formatDisplay() = 0;

    // Optional configuration functions
    virtual bool parseConfig(JsonObject cfg) { return true; }
    virtual JsonObject getConfig() { return JsonObject(); }
};

#endif // MODULE_INTERFACE_H
