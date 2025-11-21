#include "module_interface.h"
#include "config.h"
#include "network.h"
#include <ArduinoJson.h>

extern NetworkManager network;
extern StaticJsonDocument<8192> config;

// Helper to extract JSON values
float extractJsonValue(const String& json, const String& path);

class HttpModule : public ModuleInterface {
private:
    String moduleId;

public:
    HttpModule(const char* id, JsonObject cfg) {
        moduleId = String(id);
        this->id = moduleId.c_str();
        displayName = "HTTP Fetch";
        defaultRefreshInterval = 300;
        minRefreshInterval = 10;
    }

    bool fetch(String& errorMsg) override {
        JsonObject data = config["modules"][moduleId];
        String url = data["url"] | "";

        if (url.length() == 0) {
            errorMsg = "No URL configured";
            return false;
        }

        String response;
        if (!network.httpGet(url.c_str(), response, errorMsg)) {
            return false;
        }

        String jsonPath = data["jsonPath"] | "";
        float value = extractJsonValue(response, jsonPath);

        data["value"] = value;
        data["lastUpdate"] = millis() / 1000;
        data["lastSuccess"] = true;

        return true;
    }

    String formatDisplay() override {
        JsonObject data = config["modules"][moduleId];
        float value = data["value"] | 0.0;
        String label = data["label"] | "HTTP";
        String unit = data["unit"] | "";

        char buffer[64];
        if (unit.length() > 0) {
            snprintf(buffer, sizeof(buffer), "%.2f %s | %s", value, unit.c_str(), label.c_str());
        } else {
            snprintf(buffer, sizeof(buffer), "%.2f | %s", value, label.c_str());
        }
        return String(buffer);
    }
};
