#ifndef ALERT_MANAGER_H
#define ALERT_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>

/**
 * Alert conditions
 */
enum AlertCondition {
    GREATER_THAN,      // value > threshold
    LESS_THAN,         // value < threshold
    EQUALS,            // value == threshold
    CHANGE_UP,         // value increased by X%
    CHANGE_DOWN        // value decreased by X%
};

/**
 * Alert rule structure
 */
struct AlertRule {
    char id[16];                  // Unique alert ID
    char moduleId[32];            // Module to monitor (e.g., "bitcoin", "stock")
    char label[64];               // Human-readable description
    AlertCondition condition;     // Comparison type
    float threshold;              // Threshold value
    bool enabled;                 // Is this alert active?
    bool triggered;               // Has this alert been triggered?
    unsigned long lastCheck;      // Last time this alert was checked
    unsigned long triggerTime;    // When the alert was triggered
};

/**
 * Alert Manager - Monitors modules and triggers alerts based on user-defined rules
 */
class AlertManager {
private:
    AlertRule alerts[MAX_ALERTS];
    int alertCount;

    // Helper methods
    bool evaluateCondition(float currentValue, float previousValue, const AlertRule& rule);
    void logAlert(const AlertRule& rule, float value);

public:
    AlertManager();

    // Alert management
    bool addAlert(const char* moduleId, const char* label, AlertCondition condition, float threshold);
    bool removeAlert(const char* alertId);
    bool updateAlert(const char* alertId, AlertCondition condition, float threshold);
    bool enableAlert(const char* alertId, bool enabled);
    void clearAllAlerts();

    // Monitoring
    void checkAlerts();  // Call periodically to check all active alerts
    bool hasTriggeredAlerts();
    int getTriggeredCount();
    void resetAlert(const char* alertId);  // Reset triggered state
    void resetAllAlerts();

    // Getters
    int getAlertCount() { return alertCount; }
    const AlertRule* getAlert(int index);
    const AlertRule* getAlertById(const char* alertId);

    // JSON serialization
    void toJson(JsonArray& array);
    bool fromJson(const JsonArray& array);

    // Save/Load from config
    bool saveToConfig();
    bool loadFromConfig();
};

#endif // ALERT_MANAGER_H
