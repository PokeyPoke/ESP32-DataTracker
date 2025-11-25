#include "alert_manager.h"
#include "config.h"
#include "constants.h"
#include "scheduler.h"

extern Scheduler scheduler;

AlertManager::AlertManager() : alertCount(0) {
    // Initialize all alerts as empty
    for (int i = 0; i < MAX_ALERTS; i++) {
        alerts[i].id[0] = '\0';
        alerts[i].enabled = false;
        alerts[i].triggered = false;
    }
}

bool AlertManager::addAlert(const char* moduleId, const char* label,
                            AlertCondition condition, float threshold) {
    if (alertCount >= MAX_ALERTS) {
        Serial.println("ERROR: Maximum number of alerts reached");
        return false;
    }

    AlertRule& rule = alerts[alertCount];

    // Generate unique ID
    snprintf(rule.id, sizeof(rule.id), "alert_%d", alertCount + 1);
    strncpy(rule.moduleId, moduleId, sizeof(rule.moduleId) - 1);
    strncpy(rule.label, label, sizeof(rule.label) - 1);
    rule.condition = condition;
    rule.threshold = threshold;
    rule.enabled = true;
    rule.triggered = false;
    rule.lastCheck = 0;
    rule.triggerTime = 0;

    alertCount++;

    Serial.printf("Alert added: %s for %s\n", rule.id, moduleId);
    saveToConfig();

    return true;
}

bool AlertManager::removeAlert(const char* alertId) {
    // Find alert index
    int index = -1;
    for (int i = 0; i < alertCount; i++) {
        if (strcmp(alerts[i].id, alertId) == 0) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        return false;
    }

    // Shift remaining alerts down
    for (int i = index; i < alertCount - 1; i++) {
        alerts[i] = alerts[i + 1];
    }

    alertCount--;

    Serial.printf("Alert removed: %s\n", alertId);
    saveToConfig();

    return true;
}

bool AlertManager::updateAlert(const char* alertId, AlertCondition condition, float threshold) {
    AlertRule* rule = (AlertRule*)getAlertById(alertId);
    if (!rule) {
        return false;
    }

    rule->condition = condition;
    rule->threshold = threshold;
    rule->triggered = false;  // Reset trigger state

    Serial.printf("Alert updated: %s\n", alertId);
    saveToConfig();

    return true;
}

bool AlertManager::enableAlert(const char* alertId, bool enabled) {
    AlertRule* rule = (AlertRule*)getAlertById(alertId);
    if (!rule) {
        return false;
    }

    rule->enabled = enabled;
    if (!enabled) {
        rule->triggered = false;  // Reset if disabled
    }

    Serial.printf("Alert %s: %s\n", alertId, enabled ? "enabled" : "disabled");
    saveToConfig();

    return true;
}

void AlertManager::clearAllAlerts() {
    alertCount = 0;
    Serial.println("All alerts cleared");
    saveToConfig();
}

void AlertManager::checkAlerts() {
    unsigned long now = millis();

    for (int i = 0; i < alertCount; i++) {
        AlertRule& rule = alerts[i];

        // Skip if not enabled or already triggered
        if (!rule.enabled || rule.triggered) {
            continue;
        }

        // Rate limit: check at most once per minute
        if (now - rule.lastCheck < ALERT_CHECK_INTERVAL) {
            continue;
        }

        rule.lastCheck = now;

        // Get current value from scheduler/module
        auto modules = scheduler.getModules();
        auto it = modules.find(rule.moduleId);

        if (it == modules.end()) {
            continue;  // Module not found
        }

        ModuleInterface* module = it->second;
        float currentValue = 0.0f;
        float previousValue = 0.0f;

        // Extract value based on module type
        // This is simplified - in reality you'd need to parse the module's value
        // For now, we'll use a placeholder

        // Evaluate condition
        if (evaluateCondition(currentValue, previousValue, rule)) {
            rule.triggered = true;
            rule.triggerTime = now;
            logAlert(rule, currentValue);
        }
    }
}

bool AlertManager::evaluateCondition(float currentValue, float previousValue,
                                     const AlertRule& rule) {
    switch (rule.condition) {
        case GREATER_THAN:
            return currentValue > rule.threshold;

        case LESS_THAN:
            return currentValue < rule.threshold;

        case EQUALS:
            return abs(currentValue - rule.threshold) < 0.01f;  // Floating point tolerance

        case CHANGE_UP:
            if (previousValue == 0.0f) return false;
            return ((currentValue - previousValue) / previousValue * 100.0f) > rule.threshold;

        case CHANGE_DOWN:
            if (previousValue == 0.0f) return false;
            return ((previousValue - currentValue) / previousValue * 100.0f) > rule.threshold;

        default:
            return false;
    }
}

void AlertManager::logAlert(const AlertRule& rule, float value) {
    Serial.println("\n========================================");
    Serial.println("🔔 ALERT TRIGGERED!");
    Serial.printf("ID: %s\n", rule.id);
    Serial.printf("Module: %s\n", rule.moduleId);
    Serial.printf("Label: %s\n", rule.label);
    Serial.printf("Value: %.2f\n", value);
    Serial.printf("Threshold: %.2f\n", rule.threshold);
    Serial.println("========================================\n");

    // Future: Could trigger buzzer, LED, notification, etc.
}

bool AlertManager::hasTriggeredAlerts() {
    for (int i = 0; i < alertCount; i++) {
        if (alerts[i].triggered) {
            return true;
        }
    }
    return false;
}

int AlertManager::getTriggeredCount() {
    int count = 0;
    for (int i = 0; i < alertCount; i++) {
        if (alerts[i].triggered) {
            count++;
        }
    }
    return count;
}

void AlertManager::resetAlert(const char* alertId) {
    AlertRule* rule = (AlertRule*)getAlertById(alertId);
    if (rule) {
        rule->triggered = false;
        Serial.printf("Alert reset: %s\n", alertId);
    }
}

void AlertManager::resetAllAlerts() {
    for (int i = 0; i < alertCount; i++) {
        alerts[i].triggered = false;
    }
    Serial.println("All alerts reset");
}

const AlertRule* AlertManager::getAlert(int index) {
    if (index < 0 || index >= alertCount) {
        return nullptr;
    }
    return &alerts[index];
}

const AlertRule* AlertManager::getAlertById(const char* alertId) {
    for (int i = 0; i < alertCount; i++) {
        if (strcmp(alerts[i].id, alertId) == 0) {
            return &alerts[i];
        }
    }
    return nullptr;
}

void AlertManager::toJson(JsonArray& array) {
    for (int i = 0; i < alertCount; i++) {
        const AlertRule& rule = alerts[i];
        JsonObject obj = array.createNestedObject();

        obj["id"] = rule.id;
        obj["moduleId"] = rule.moduleId;
        obj["label"] = rule.label;
        obj["condition"] = (int)rule.condition;
        obj["threshold"] = rule.threshold;
        obj["enabled"] = rule.enabled;
        obj["triggered"] = rule.triggered;
    }
}

bool AlertManager::fromJson(const JsonArray& array) {
    alertCount = 0;

    for (JsonVariant v : array) {
        if (alertCount >= MAX_ALERTS) {
            break;
        }

        JsonObject obj = v.as<JsonObject>();
        AlertRule& rule = alerts[alertCount];

        strncpy(rule.id, obj["id"] | "", sizeof(rule.id) - 1);
        strncpy(rule.moduleId, obj["moduleId"] | "", sizeof(rule.moduleId) - 1);
        strncpy(rule.label, obj["label"] | "", sizeof(rule.label) - 1);
        rule.condition = (AlertCondition)(obj["condition"] | 0);
        rule.threshold = obj["threshold"] | 0.0f;
        rule.enabled = obj["enabled"] | true;
        rule.triggered = obj["triggered"] | false;
        rule.lastCheck = 0;
        rule.triggerTime = 0;

        alertCount++;
    }

    Serial.printf("Loaded %d alerts from JSON\n", alertCount);
    return true;
}

bool AlertManager::saveToConfig() {
    JsonArray alertsArray = config.createNestedArray("alerts");
    toJson(alertsArray);
    return saveConfig();
}

bool AlertManager::loadFromConfig() {
    if (config.containsKey("alerts")) {
        JsonArray alertsArray = config["alerts"];
        return fromJson(alertsArray);
    }
    return false;
}
