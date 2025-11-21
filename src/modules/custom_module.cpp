#include "module_interface.h"
#include "config.h"
#include <ArduinoJson.h>

class CustomModule : public ModuleInterface {
public:
    CustomModule() {
        id = "custom";
        displayName = "Custom Metric";
        defaultRefreshInterval = 0;
        minRefreshInterval = 0;
    }

    bool fetch(String& errorMsg) override {
        JsonObject data = config["modules"]["custom"];
        data["lastUpdate"] = millis() / 1000;
        data["lastSuccess"] = true;
        return true;
    }

    String formatDisplay() override {
        JsonObject data = config["modules"]["custom"];
        float value = data["value"] | 0.0;
        String label = data["label"] | "Custom";
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
