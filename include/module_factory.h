#ifndef MODULE_FACTORY_H
#define MODULE_FACTORY_H

#include "modules/module_interface.h"
#include <ArduinoJson.h>

/**
 * Module Factory Pattern
 *
 * Creates module instances dynamically based on type and configuration.
 * Supports multiple instances of the same module type with unique IDs.
 *
 * Example:
 *   ModuleInterface* btcModule = ModuleFactory::createModule("crypto", "crypto_btc", btcConfig);
 *   ModuleInterface* ethModule = ModuleFactory::createModule("crypto", "crypto_eth", ethConfig);
 */
class ModuleFactory {
public:
    /**
     * Create a module instance
     *
     * @param type Module type ("crypto", "stock", "weather", "custom", "settings")
     * @param id Unique module ID (e.g., "crypto_btc", "stock_aapl")
     * @param config Configuration object from config["modules"][id]
     * @return Pointer to new module instance, or nullptr if type unknown
     */
    static ModuleInterface* createModule(const char* type, const char* id, JsonObject config);

    /**
     * Get display name for a module type
     *
     * @param type Module type
     * @return Human-readable name
     */
    static const char* getModuleTypeName(const char* type);

    /**
     * Check if a module type is valid
     *
     * @param type Module type
     * @return true if valid
     */
    static bool isValidModuleType(const char* type);
};

#endif // MODULE_FACTORY_H
