#include "module_interface.h"
#include "config.h"
#include <ArduinoJson.h>
#include <time.h>

extern StaticJsonDocument<8192> config;

class CountdownModule : public ModuleInterface {
private:
    String moduleId;

public:
    CountdownModule(const char* id, JsonObject cfg) {
        moduleId = String(id);
        this->id = moduleId.c_str();
        displayName = "Countdown";
        defaultRefreshInterval = 3600;  // Refresh hourly
        minRefreshInterval = 60;        // Min 1 minute
    }

    bool fetch(String& errorMsg) override {
        JsonObject data = config["modules"][moduleId];

        String targetDate = data["targetDate"] | "";
        if (targetDate.length() == 0) {
            errorMsg = "No target date set";
            return false;
        }

        // Parse target date (YYYY-MM-DD)
        int year, month, day;
        if (sscanf(targetDate.c_str(), "%d-%d-%d", &year, &month, &day) != 3) {
            errorMsg = "Invalid date format";
            return false;
        }

        // Get current time
        time_t now;
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo)) {
            errorMsg = "Time not synced";
            return false;
        }

        // Create target time
        struct tm target = {0};
        target.tm_year = year - 1900;
        target.tm_mon = month - 1;
        target.tm_mday = day;

        String countdownType = data["countdownType"] | "date";
        if (countdownType == "datetime") {
            String targetTime = data["targetTime"] | "00:00";
            int hour, minute;
            if (sscanf(targetTime.c_str(), "%d:%d", &hour, &minute) == 2) {
                target.tm_hour = hour;
                target.tm_min = minute;
                target.tm_sec = 0;
            }
        } else {
            target.tm_hour = 0;
            target.tm_min = 0;
            target.tm_sec = 0;
        }

        time(&now);
        time_t targetTimestamp = mktime(&target);
        int secondsDiff = targetTimestamp - now;

        // Store as days or hours
        if (countdownType == "datetime") {
            data["value"] = secondsDiff / 3600.0;  // Hours
        } else {
            data["value"] = (float)(secondsDiff / 86400);  // Days
        }

        data["lastUpdate"] = millis() / 1000;
        data["lastSuccess"] = true;

        return true;
    }

    String formatDisplay() override {
        JsonObject data = config["modules"][moduleId];
        float value = data["value"] | 0.0;
        String label = data["label"] | "Event";
        String countdownType = data["countdownType"] | "date";

        char buffer[64];

        if (countdownType == "datetime") {
            int hours = (int)value;
            int days = hours / 24;
            int remainingHours = hours % 24;

            if (hours < 0) {
                snprintf(buffer, sizeof(buffer), "Past | %s", label.c_str());
            } else if (hours < 1) {
                snprintf(buffer, sizeof(buffer), "< 1hr | %s", label.c_str());
            } else if (hours < 24) {
                snprintf(buffer, sizeof(buffer), "%dh | %s", hours, label.c_str());
            } else if (remainingHours == 0) {
                snprintf(buffer, sizeof(buffer), "%dd | %s", days, label.c_str());
            } else {
                snprintf(buffer, sizeof(buffer), "%dd %dh | %s", days, remainingHours, label.c_str());
            }
        } else {
            int days = (int)value;
            if (days == 0) {
                snprintf(buffer, sizeof(buffer), "Today! | %s", label.c_str());
            } else if (days == 1) {
                snprintf(buffer, sizeof(buffer), "1 day | %s", label.c_str());
            } else if (days < 0) {
                snprintf(buffer, sizeof(buffer), "%d days ago | %s", -days, label.c_str());
            } else {
                snprintf(buffer, sizeof(buffer), "%d days | %s", days, label.c_str());
            }
        }

        return String(buffer);
    }
};
