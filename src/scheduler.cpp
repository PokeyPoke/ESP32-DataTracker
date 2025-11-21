#include "scheduler.h"
#include "modules/module_interface.h"
#include "module_factory.h"
#include "config.h"

Scheduler::Scheduler() {
    context.state = IDLE;
    context.lastFetchTime = 0;
    context.nextAllowedFetch = 0;
    context.retryCount = 0;
    context.retryDelay = 0;
    lastGlobalFetch = 0;
}

Scheduler::~Scheduler() {
    // Clean up registered modules
    for (auto& pair : modules) {
        delete pair.second;
    }
    modules.clear();
}

void Scheduler::init() {
    Serial.println("Scheduler initialized");
}

void Scheduler::registerModule(ModuleInterface* module) {
    if (module && module->id) {
        modules[String(module->id)] = module;
        Serial.print("Registered module: ");
        Serial.println(module->id);
    }
}

void Scheduler::unregisterModule(const char* moduleId) {
    String id = String(moduleId);
    auto it = modules.find(id);
    if (it != modules.end()) {
        delete it->second;  // Free module memory
        modules.erase(it);
        Serial.print("Unregistered module: ");
        Serial.println(moduleId);
    }
}

void Scheduler::loadModulesFromConfig() {
    Serial.println("\n=== Loading Modules from Config ===");

    // Get module order array
    JsonArray moduleOrder = config["device"]["moduleOrder"];
    if (moduleOrder.size() == 0) {
        Serial.println("ERROR: moduleOrder is empty!");
        return;
    }

    Serial.print("Found ");
    Serial.print(moduleOrder.size());
    Serial.println(" modules in config");

    // Iterate through module order and create instances
    for (JsonVariant v : moduleOrder) {
        String moduleId = v.as<String>();

        // Get module config
        JsonObject moduleConfig = config["modules"][moduleId];
        if (moduleConfig.isNull()) {
            Serial.print("WARNING: Module '");
            Serial.print(moduleId);
            Serial.println("' not found in config, skipping");
            continue;
        }

        // Get module type
        String moduleType = moduleConfig["type"] | "unknown";
        if (moduleType == "unknown") {
            Serial.print("WARNING: Module '");
            Serial.print(moduleId);
            Serial.println("' has no type, skipping");
            continue;
        }

        // Check if module already registered (avoid duplicates)
        if (hasModule(moduleId.c_str())) {
            Serial.print("Module '");
            Serial.print(moduleId);
            Serial.println("' already registered, skipping");
            continue;
        }

        // Create module using factory
        ModuleInterface* module = ModuleFactory::createModule(
            moduleType.c_str(),
            moduleId.c_str(),
            moduleConfig
        );

        if (module) {
            registerModule(module);
            Serial.print("  ✓ Created ");
            Serial.print(moduleType);
            Serial.print(" module: ");
            Serial.println(moduleId);
        } else {
            Serial.print("  ✗ Failed to create module: ");
            Serial.println(moduleId);
        }
    }

    Serial.print("\nTotal modules registered: ");
    Serial.println(modules.size());
    Serial.println("===================================\n");
}

void Scheduler::tick() {
    unsigned long now = millis() / 1000;

    // If currently fetching, let it complete
    if (context.state == FETCHING) {
        return;
    }

    // Check if it's time to auto-refresh the active module
    if (context.state == IDLE) {
        String activeModule = config["device"]["activeModule"] | "bitcoin";

        JsonObject moduleData = config["modules"][activeModule];
        unsigned long lastUpdate = moduleData["lastUpdate"] | 0;

        // Get module-specific refresh interval, fall back to global default
        uint16_t moduleRefreshInterval = moduleData["refreshInterval"] | 0;
        if (moduleRefreshInterval == 0) {
            // Fall back to global default if module doesn't specify
            moduleRefreshInterval = config["device"]["refreshInterval"] | 300;
        }

        // Enforce min/max limits (10s - 3600s)
        if (moduleRefreshInterval < 10) moduleRefreshInterval = 10;
        if (moduleRefreshInterval > 3600) moduleRefreshInterval = 3600;

        // Debug logging
        static unsigned long lastDebugTime = 0;
        if (now - lastDebugTime > 60) {  // Log every 60 seconds
            Serial.print("DEBUG Scheduler: activeModule=");
            Serial.print(activeModule);
            Serial.print(" lastUpdate=");
            Serial.print(lastUpdate);
            Serial.print(" now=");
            Serial.print(now);
            Serial.print(" moduleRefreshInterval=");
            Serial.print(moduleRefreshInterval);
            Serial.print(" timeSinceLastUpdate=");
            Serial.println(now - lastUpdate);
            lastDebugTime = now;
        }

        if (lastUpdate == 0 || (now - lastUpdate) >= moduleRefreshInterval) {
            // Time to refresh
            Serial.print("Scheduler: Triggering fetch for ");
            Serial.print(activeModule);
            Serial.print(" (interval: ");
            Serial.print(moduleRefreshInterval);
            Serial.println("s)");
            requestFetch(activeModule.c_str(), false);
        }
    }
}

void Scheduler::requestFetch(const char* moduleId, bool forced) {
    unsigned long now = millis() / 1000;

    Serial.print("DEBUG requestFetch: moduleId=");
    Serial.print(moduleId);
    Serial.print(" forced=");
    Serial.println(forced);

    // Debug: print all registered modules
    Serial.print("DEBUG: Total modules in map: ");
    Serial.println(modules.size());
    Serial.println("DEBUG: Registered modules:");
    for (auto& pair : modules) {
        Serial.print("  - ");
        Serial.println(pair.first);
    }

    // Check if module exists
    if (modules.find(String(moduleId)) == modules.end()) {
        Serial.print("ERROR: Module not found in map: ");
        Serial.println(moduleId);
        Serial.println("Module lookup FAILED!");
        return;
    }
    Serial.println("Module found in map!");

    ModuleInterface* module = modules[String(moduleId)];

    // Check global cooldown
    if (!forced && (now - lastGlobalFetch) < GLOBAL_MIN_INTERVAL) {
        Serial.print("Fetch denied: global cooldown active (");
        Serial.print(now - lastGlobalFetch);
        Serial.print("s < ");
        Serial.print(GLOBAL_MIN_INTERVAL);
        Serial.println("s)");
        return;
    }

    // Check module-specific cooldown
    JsonObject moduleData = config["modules"][moduleId];
    unsigned long lastUpdate = moduleData["lastUpdate"] | 0;
    if (!forced && (now - lastUpdate) < module->minRefreshInterval) {
        Serial.print("Fetch denied: module cooldown (last update ");
        Serial.print(now - lastUpdate);
        Serial.print("s ago, min interval ");
        Serial.print(module->minRefreshInterval);
        Serial.println("s)");
        return;
    }

    // Check retry backoff (unless forced)
    if (!forced && context.retryDelay > 0 && (now - context.lastFetchTime) < context.retryDelay) {
        Serial.print("Fetch denied: retry backoff (");
        Serial.print(context.retryDelay - (now - context.lastFetchTime));
        Serial.println("s remaining)");
        return;
    }

    // Approve fetch
    if (forced) {
        Serial.println("FORCED FETCH - bypassing all cooldowns");
    } else {
        Serial.println("FETCH APPROVED - passed all cooldown checks");
    }
    context.currentModule = String(moduleId);
    context.state = FETCHING;
    executeFetch();
}

void Scheduler::executeFetch() {
    Serial.println("\n=== executeFetch() called ===");
    Serial.print("Current module: ");
    Serial.println(context.currentModule);

    if (modules.find(context.currentModule) == modules.end()) {
        Serial.print("ERROR: Module not found in map: ");
        Serial.println(context.currentModule);
        context.state = IDLE;
        return;
    }

    ModuleInterface* module = modules[context.currentModule];
    Serial.print("Module found at address: ");
    Serial.println((unsigned long)module, HEX);
    Serial.print("Module ID: ");
    Serial.println(module->id);

    String errorMsg;
    Serial.println("About to call module->fetch()...");
    bool success = false;

    if (context.currentModule == "stock") {
        Serial.println("DEBUG: Calling StockModule fetch specifically");
    }

    success = module->fetch(errorMsg);

    Serial.print("module->fetch() returned: ");
    Serial.println(success ? "true" : "false");
    if (!success) {
        Serial.print("Error message: ");
        Serial.println(errorMsg);
    }

    unsigned long now = millis() / 1000;
    context.lastFetchTime = now;
    lastGlobalFetch = now;

    if (success) {
        Serial.println("Fetch successful");
        context.retryCount = 0;
        context.retryDelay = 0;

        JsonObject moduleData = config["modules"][context.currentModule];
        moduleData["lastSuccess"] = true;
        moduleData["lastError"] = "";
    } else {
        Serial.print("Fetch failed: ");
        Serial.println(errorMsg);

        context.retryCount++;
        context.retryDelay = calculateBackoff(context.retryCount);

        JsonObject moduleData = config["modules"][context.currentModule];
        moduleData["lastSuccess"] = false;
        moduleData["lastError"] = errorMsg;

        Serial.print("Retry count: ");
        Serial.print(context.retryCount);
        Serial.print(", next retry in ");
        Serial.print(context.retryDelay);
        Serial.println(" seconds");
    }

    context.state = IDLE;
}

uint16_t Scheduler::calculateBackoff(uint8_t retryCount) {
    // Exponential backoff: min(2^n × 60s, 3600s)
    uint16_t delay = 60 * (1 << retryCount);  // 2^n × 60
    return min(delay, (uint16_t)3600);
}
