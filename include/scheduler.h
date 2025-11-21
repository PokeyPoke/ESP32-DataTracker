#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Arduino.h>
#include <map>

// Forward declaration
class ModuleInterface;

// Scheduler states
enum SchedulerState {
    IDLE,         // Waiting for next scheduled fetch
    FETCHING,     // HTTP request in progress
    COOLDOWN,     // Mandatory gap between requests
    RETRY_WAIT    // Backing off after failure
};

// Scheduler context
struct SchedulerContext {
    SchedulerState state;
    unsigned long lastFetchTime;
    unsigned long nextAllowedFetch;
    uint8_t retryCount;
    uint16_t retryDelay;
    String currentModule;
};

class Scheduler {
private:
    std::map<String, ModuleInterface*> modules;
    SchedulerContext context;
    unsigned long lastGlobalFetch;

    static const uint16_t GLOBAL_MIN_INTERVAL = 10;  // 10 seconds between any fetches

    uint16_t calculateBackoff(uint8_t retryCount);
    void executeFetch();

public:
    Scheduler();
    ~Scheduler();

    void init();
    void registerModule(ModuleInterface* module);
    void unregisterModule(const char* moduleId);
    void loadModulesFromConfig();  // Load all modules from config using factory
    void tick();
    void requestFetch(const char* moduleId, bool forced = false);

    SchedulerState getState() { return context.state; }
    String getCurrentModule() { return context.currentModule; }
    int getModuleCount() { return modules.size(); }
    bool hasModule(const char* moduleId) { return modules.find(String(moduleId)) != modules.end(); }
};

#endif // SCHEDULER_H
