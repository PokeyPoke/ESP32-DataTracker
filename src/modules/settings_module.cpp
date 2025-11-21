#include "module_interface.h"
#include "config.h"
#include "security.h"

// External security manager (will be initialized in main)
extern SecurityManager security;

class SettingsModule : public ModuleInterface {
public:
    SettingsModule() {
        id = "settings";
        displayName = "Settings";
        defaultRefreshInterval = 300;  // 5 minutes (code expiry)
        minRefreshInterval = 60;       // 1 minute
    }

    bool fetch(String& errorMsg) override {
        // Settings module doesn't fetch data from network
        // Instead, it generates a new security code

        uint32_t code = security.generateNewCode();

        // Store code in config for display
        JsonObject data = config["modules"]["settings"].to<JsonObject>();
        data["securityCode"] = code;
        data["codeTimeRemaining"] = security.getCodeTimeRemaining();
        data["lastUpdate"] = millis() / 1000;
        data["lastSuccess"] = true;

        Serial.print("Settings module: New security code generated: ");
        Serial.println(code);

        return true;
    }

    bool parseResponse(String payload, String& errorMsg) {
        // Not applicable for settings module
        return true;
    }

    String formatDisplay() override {
        JsonObject data = config["modules"]["settings"];
        uint32_t code = data["securityCode"] | 0;

        char buffer[32];
        snprintf(buffer, sizeof(buffer), "Code: %06u", code);
        return String(buffer);
    }
};
