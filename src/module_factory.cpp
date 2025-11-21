#include "module_factory.h"
#include "config.h"
#include "network.h"
#include "security.h"
#include <ArduinoJson.h>

// External references
extern NetworkManager network;
extern StaticJsonDocument<8192> config;

// ============================================================================
// Generic Crypto Module (supports any CoinGecko coin)
// ============================================================================
class GenericCryptoModule : public ModuleInterface {
private:
    String moduleId;  // Unique instance ID

public:
    GenericCryptoModule(const char* id, JsonObject cfg) {
        moduleId = String(id);
        this->id = moduleId.c_str();  // Store pointer (safe because moduleId stays in memory)
        displayName = "Crypto";
        defaultRefreshInterval = 300;  // 5 minutes
        minRefreshInterval = 60;       // 1 minute
    }

    bool fetch(String& errorMsg) override {
        // Get configured crypto ID
        JsonObject moduleData = config["modules"][moduleId];
        String cryptoId = moduleData["cryptoId"] | "bitcoin";

        // Get currency (default USD)
        String currency = config["device"]["currency"] | "USD";
        currency.toLowerCase();

        // Build URL with configured crypto and currency
        String url = "https://api.coingecko.com/api/v3/simple/price?ids=" + cryptoId +
                     "&vs_currencies=" + currency + "&include_24hr_change=true";

        Serial.print("Crypto fetch: ");
        Serial.print(moduleId);
        Serial.print(" (");
        Serial.print(cryptoId);
        Serial.print("/");
        String currUpper = currency;
        currUpper.toUpperCase();
        Serial.print(currUpper);
        Serial.println(")");

        String response;
        if (!network.httpGet(url.c_str(), response, errorMsg)) {
            return false;
        }

        return parseResponse(response, errorMsg, cryptoId, currency);
    }

    bool parseResponse(String payload, String& errorMsg, String cryptoId, String currency) {
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
            errorMsg = "JSON parse error: " + String(error.c_str());
            return false;
        }

        if (!doc.containsKey(cryptoId)) {
            errorMsg = "Invalid response structure";
            return false;
        }

        // Get price in requested currency
        if (!doc[cryptoId].containsKey(currency)) {
            errorMsg = "Currency not found in response";
            return false;
        }

        float price = doc[cryptoId][currency];
        String changeKey = currency + "_24h_change";
        float change = doc[cryptoId][changeKey] | 0.0;

        // Update cache
        JsonObject data = config["modules"][moduleId];
        data["value"] = price;
        data["change24h"] = change;
        data["lastUpdate"] = millis() / 1000;
        data["lastSuccess"] = true;

        String cryptoName = data["cryptoName"] | cryptoId;
        Serial.print(cryptoName);
        Serial.print(" price: ");
        Serial.print(getCurrencySymbol(currency.c_str()));
        Serial.print(price, 2);
        Serial.print(" (");
        Serial.print(change, 2);
        Serial.println("%)");

        return true;
    }

    String formatDisplay() override {
        JsonObject data = config["modules"][moduleId];
        float price = data["value"] | 0.0;
        float change = data["change24h"] | 0.0;
        String currency = config["device"]["currency"] | "USD";

        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%s%.2f | %+.1f%%",
                 getCurrencySymbol(currency.c_str()), price, change);
        return String(buffer);
    }

private:
    const char* getCurrencySymbol(const char* currency) {
        String curr = String(currency);
        curr.toUpperCase();

        if (curr == "USD") return "$";
        if (curr == "EUR") return "€";
        if (curr == "GBP") return "£";
        if (curr == "JPY") return "¥";
        if (curr == "CNY") return "¥";
        if (curr == "CHF") return "Fr";
        if (curr == "AUD") return "A$";
        if (curr == "CAD") return "C$";
        if (curr == "INR") return "₹";
        if (curr == "BRL") return "R$";

        // Default: generic currency symbol
        return "$";
    }
};

// ============================================================================
// Generic Stock Module (supports any ticker via Yahoo Finance)
// ============================================================================
class GenericStockModule : public ModuleInterface {
private:
    String moduleId;

public:
    GenericStockModule(const char* id, JsonObject cfg) {
        moduleId = String(id);
        this->id = moduleId.c_str();
        displayName = "Stock";
        defaultRefreshInterval = 300;  // 5 minutes
        minRefreshInterval = 60;       // 1 minute
    }

    bool fetch(String& errorMsg) override {
        JsonObject stockData = config["modules"][moduleId];
        String ticker = stockData["ticker"] | "AAPL";

        if (ticker.length() == 0) {
            errorMsg = "Ticker is empty";
            return false;
        }

        String url = "https://query1.finance.yahoo.com/v8/finance/chart/" + ticker + "?interval=1d&range=1d";

        Serial.print("Stock fetch: ");
        Serial.print(moduleId);
        Serial.print(" (");
        Serial.print(ticker);
        Serial.println(")");

        String response;
        if (!network.httpGet(url.c_str(), response, errorMsg)) {
            return false;
        }

        return parseResponse(response, errorMsg);
    }

    bool parseResponse(String payload, String& errorMsg) {
        if (payload.length() == 0) {
            errorMsg = "Empty response";
            return false;
        }

        StaticJsonDocument<2048> doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
            errorMsg = "JSON parse error: " + String(error.c_str());
            return false;
        }

        if (!doc.containsKey("chart")) {
            errorMsg = "Missing chart data";
            return false;
        }

        JsonObject chart = doc["chart"];
        if (!chart.containsKey("result") || chart["result"].size() == 0) {
            errorMsg = "Missing result data";
            return false;
        }

        JsonObject result = chart["result"][0];
        if (!result.containsKey("meta")) {
            errorMsg = "Missing meta data";
            return false;
        }

        JsonObject meta = result["meta"];
        if (!meta.containsKey("regularMarketPrice")) {
            errorMsg = "Missing regularMarketPrice";
            return false;
        }

        float price = meta["regularMarketPrice"] | 0.0;
        float previousClose = meta["chartPreviousClose"] | price;

        float changePercent = 0.0;
        if (previousClose > 0) {
            changePercent = ((price - previousClose) / previousClose) * 100.0;
        }

        String ticker = config["modules"][moduleId]["ticker"] | "???";

        // Update cache
        JsonObject data = config["modules"][moduleId];
        data["value"] = price;
        data["change"] = changePercent;
        data["ticker"] = ticker;
        data["lastUpdate"] = millis() / 1000;
        data["lastSuccess"] = true;

        Serial.print(ticker);
        Serial.print(" price: $");
        Serial.print(price, 2);
        Serial.print(" (");
        Serial.print(changePercent, 2);
        Serial.println("%)");

        return true;
    }

    String formatDisplay() override {
        JsonObject data = config["modules"][moduleId];
        float price = data["value"] | 0.0;
        float change = data["change"] | 0.0;
        String ticker = data["ticker"] | "N/A";

        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%s: $%.2f | %+.1f%%", ticker.c_str(), price, change);
        return String(buffer);
    }
};

// ============================================================================
// Generic Weather Module (supports any location via Open-Meteo)
// ============================================================================
class GenericWeatherModule : public ModuleInterface {
private:
    String moduleId;

public:
    GenericWeatherModule(const char* id, JsonObject cfg) {
        moduleId = String(id);
        this->id = moduleId.c_str();
        displayName = "Weather";
        defaultRefreshInterval = 600;   // 10 minutes
        minRefreshInterval = 300;       // 5 minutes
    }

    bool fetch(String& errorMsg) override {
        JsonObject weatherData = config["modules"][moduleId];
        float lat = weatherData["latitude"] | 37.7749;
        float lon = weatherData["longitude"] | -122.4194;

        Serial.print("Weather fetch: ");
        Serial.print(moduleId);
        Serial.print(" (");
        Serial.print(lat, 4);
        Serial.print(", ");
        Serial.print(lon, 4);
        Serial.println(")");

        // Use Open-Meteo API (free, no auth)
        String url = "https://api.open-meteo.com/v1/forecast?latitude=" +
                     String(lat, 4) + "&longitude=" + String(lon, 4) +
                     "&current_weather=true";

        String response;
        if (!network.httpGet(url.c_str(), response, errorMsg)) {
            return false;
        }

        return parseResponse(response, errorMsg);
    }

    bool parseResponse(String payload, String& errorMsg) {
        StaticJsonDocument<1024> doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
            errorMsg = "JSON parse error";
            return false;
        }

        if (!doc.containsKey("current_weather")) {
            errorMsg = "Missing weather data";
            return false;
        }

        JsonObject current = doc["current_weather"];
        float temp = current["temperature"];
        int weatherCode = current["weathercode"];

        // Update cache
        JsonObject data = config["modules"][moduleId];
        data["temperature"] = temp;
        data["condition"] = getWeatherCondition(weatherCode);
        data["lastUpdate"] = millis() / 1000;
        data["lastSuccess"] = true;

        String location = data["location"] | "Unknown";
        Serial.print(location);
        Serial.print(" weather: ");
        Serial.print(temp, 1);
        Serial.print("°C, ");
        Serial.println(data["condition"].as<const char*>());

        return true;
    }

    String formatDisplay() override {
        JsonObject data = config["modules"][moduleId];
        float temp = data["temperature"] | 0.0;
        String condition = data["condition"] | "Unknown";

        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%.1f°C | %s", temp, condition.c_str());
        return String(buffer);
    }

private:
    const char* getWeatherCondition(int code) {
        if (code == 0) return "Clear";
        if (code <= 3) return "Cloudy";
        if (code <= 49) return "Fog";
        if (code <= 59) return "Drizzle";
        if (code <= 69) return "Rain";
        if (code <= 79) return "Snow";
        if (code <= 99) return "Thunderstorm";
        return "Unknown";
    }
};

// ============================================================================
// Generic Custom Module (user-defined values)
// ============================================================================
// Helper function to extract value from JSON using simple path notation
// Supports: "field", "parent.child", "array[0]", "parent.array[0].field"
float extractJsonValue(const String& json, const String& path) {
    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, json);

    if (error) {
        Serial.print("JSON parse error: ");
        Serial.println(error.c_str());
        return 0.0;
    }

    // If path is empty, try to parse entire response as number
    if (path.length() == 0) {
        return json.toFloat();
    }

    // Navigate JSON tree using path
    JsonVariant current = doc.as<JsonVariant>();
    int pathStart = 0;

    while (pathStart < path.length()) {
        int dotPos = path.indexOf('.', pathStart);
        int bracketPos = path.indexOf('[', pathStart);

        String segment;
        int arrayIndex = -1;

        // Determine segment boundaries
        if (bracketPos != -1 && (dotPos == -1 || bracketPos < dotPos)) {
            // Array access: "field[0]" or "[0]"
            if (bracketPos > pathStart) {
                segment = path.substring(pathStart, bracketPos);
            }
            int closeBracket = path.indexOf(']', bracketPos);
            if (closeBracket != -1) {
                arrayIndex = path.substring(bracketPos + 1, closeBracket).toInt();
                pathStart = closeBracket + 1;
                if (pathStart < path.length() && path.charAt(pathStart) == '.') {
                    pathStart++;  // Skip dot after bracket
                }
            }
        } else if (dotPos != -1) {
            // Object field with more path remaining
            segment = path.substring(pathStart, dotPos);
            pathStart = dotPos + 1;
        } else {
            // Last segment
            segment = path.substring(pathStart);
            pathStart = path.length();
        }

        // Navigate to field if segment exists
        if (segment.length() > 0) {
            if (current.is<JsonObject>()) {
                current = current[segment];
            } else {
                Serial.println("Path navigation failed: not an object");
                return 0.0;
            }
        }

        // Navigate to array index if specified
        if (arrayIndex != -1) {
            if (current.is<JsonArray>()) {
                current = current[arrayIndex];
            } else {
                Serial.println("Path navigation failed: not an array");
                return 0.0;
            }
        }

        if (current.isNull()) {
            Serial.println("Path not found in JSON");
            return 0.0;
        }
    }

    // Convert final value to float
    if (current.is<float>()) return current.as<float>();
    if (current.is<int>()) return (float)current.as<int>();
    if (current.is<const char*>()) return String(current.as<const char*>()).toFloat();

    Serial.println("Could not convert value to float");
    return 0.0;
}

class GenericCustomModule : public ModuleInterface {
private:
    String moduleId;

public:
    GenericCustomModule(const char* id, JsonObject cfg) {
        moduleId = String(id);
        this->id = moduleId.c_str();
        displayName = "Custom";
        defaultRefreshInterval = 300;  // 5 minutes (can fetch from HTTP)
        minRefreshInterval = 10;       // 10 seconds minimum
    }

    bool fetch(String& errorMsg) override {
        JsonObject data = config["modules"][moduleId];
        String url = data["url"] | "";

        // If no URL configured, this is manual-entry mode (current behavior)
        if (url.length() == 0) {
            return true;  // No fetch needed, data set manually via API
        }

        // HTTP fetch mode
        Serial.print("Custom module fetching from: ");
        Serial.println(url);

        extern NetworkManager network;
        String response;
        if (!network.httpGet(url.c_str(), response, errorMsg)) {
            Serial.print("HTTP fetch failed: ");
            Serial.println(errorMsg);
            return false;
        }

        Serial.print("Response received (");
        Serial.print(response.length());
        Serial.println(" bytes)");

        // Extract value using JSON path (if specified)
        String jsonPath = data["jsonPath"] | "";
        float extractedValue = extractJsonValue(response, jsonPath);

        Serial.print("Extracted value: ");
        Serial.println(extractedValue);

        // Update module data
        data["value"] = extractedValue;

        return true;
    }

    String formatDisplay() override {
        JsonObject data = config["modules"][moduleId];
        float value = data["value"] | 0.0;
        String label = data["label"] | "Value";
        String unit = data["unit"] | "";

        char buffer[64];
        if (unit.length() > 0) {
            snprintf(buffer, sizeof(buffer), "%s: %.2f %s", label.c_str(), value, unit.c_str());
        } else {
            snprintf(buffer, sizeof(buffer), "%s: %.2f", label.c_str(), value);
        }
        return String(buffer);
    }
};

// ============================================================================
// Transit Module (shows next bus/train arrival times)
// ============================================================================
class GenericTransitModule : public ModuleInterface {
private:
    String moduleId;

public:
    GenericTransitModule(const char* id, JsonObject cfg) {
        moduleId = String(id);
        this->id = moduleId.c_str();
        displayName = "Transit";
        defaultRefreshInterval = 30;   // 30 seconds (good for real-time transit)
        minRefreshInterval = 10;       // 10 seconds minimum
    }

    bool fetch(String& errorMsg) override {
        JsonObject data = config["modules"][moduleId];
        String apiUrl = data["apiUrl"] | "";

        if (apiUrl.length() == 0) {
            errorMsg = "No API URL configured";
            return false;
        }

        Serial.print("Transit module fetching from: ");
        Serial.println(apiUrl);

        extern NetworkManager network;
        String response;
        if (!network.httpGet(apiUrl.c_str(), response, errorMsg)) {
            Serial.print("HTTP fetch failed: ");
            Serial.println(errorMsg);
            return false;
        }

        Serial.print("Response received (");
        Serial.print(response.length());
        Serial.println(" bytes)");

        // Parse JSON response to extract arrival time
        return parseTransitResponse(response, errorMsg);
    }

    bool parseTransitResponse(String payload, String& errorMsg) {
        StaticJsonDocument<2048> doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
            errorMsg = "JSON parse error: " + String(error.c_str());
            return false;
        }

        JsonObject data = config["modules"][moduleId];
        String jsonPath = data["jsonPath"] | "";

        // If jsonPath is specified, use it to extract the minutes value
        float minutes = 0.0;
        if (jsonPath.length() > 0) {
            minutes = extractJsonValue(payload, jsonPath);
        } else {
            // Try common GTFS/transit API patterns
            // Pattern 1: {arrivals: [{minutes: 5}]}
            if (doc.containsKey("arrivals") && doc["arrivals"].is<JsonArray>()) {
                JsonArray arrivals = doc["arrivals"];
                if (arrivals.size() > 0) {
                    minutes = arrivals[0]["minutes"] | arrivals[0]["min"] | 0.0;
                }
            }
            // Pattern 2: {minutes: 5}
            else if (doc.containsKey("minutes")) {
                minutes = doc["minutes"];
            }
            // Pattern 3: {min: 5}
            else if (doc.containsKey("min")) {
                minutes = doc["min"];
            }
            // Pattern 4: Calculate from timestamp
            else if (doc.containsKey("arrivalTime")) {
                unsigned long arrivalTime = doc["arrivalTime"];
                unsigned long now = millis() / 1000;
                if (arrivalTime > now) {
                    minutes = (arrivalTime - now) / 60.0;
                }
            }
        }

        // Update cache
        data["nextArrival"] = minutes;
        data["lastUpdate"] = millis() / 1000;
        data["lastSuccess"] = true;

        String routeName = data["routeName"] | "Transit";
        Serial.print(routeName);
        Serial.print(" next arrival: ");
        Serial.print(minutes, 1);
        Serial.println(" min");

        return true;
    }

    String formatDisplay() override {
        JsonObject data = config["modules"][moduleId];
        float minutes = data["nextArrival"] | 0.0;
        String routeName = data["routeName"] | "Transit";

        char buffer[64];
        if (minutes < 1.0) {
            snprintf(buffer, sizeof(buffer), "%s: NOW", routeName.c_str());
        } else {
            snprintf(buffer, sizeof(buffer), "%s: %.0f min", routeName.c_str(), minutes);
        }
        return String(buffer);
    }
};

// ============================================================================
// Settings Module (shows security code for web access)
// ============================================================================
class GenericSettingsModule : public ModuleInterface {
public:
    GenericSettingsModule(const char* id, JsonObject cfg) {
        this->id = "settings";
        displayName = "Settings";
        defaultRefreshInterval = 30;   // 30 seconds
        minRefreshInterval = 10;       // 10 seconds
    }

    bool fetch(String& errorMsg) override {
        // Use SecurityManager to generate code (keeps display and validation in sync)
        extern SecurityManager security;
        uint32_t code = security.generateNewCode();

        // Calculate time remaining until expiration
        unsigned long now = millis();
        unsigned long generatedAt = now;
        unsigned long expiresAt = generatedAt + 300000;  // 5 minutes
        unsigned long timeRemaining = expiresAt - now;

        // Store in config for display
        JsonObject data = config["modules"]["settings"];
        data["securityCode"] = code;
        data["codeTimeRemaining"] = timeRemaining;
        data["lastUpdate"] = millis() / 1000;
        data["lastSuccess"] = true;

        Serial.print("Settings: Generated code via SecurityManager: ");
        Serial.println(code);

        return true;
    }

    String formatDisplay() override {
        JsonObject data = config["modules"]["settings"];
        uint32_t code = data["securityCode"] | 0;

        char buffer[16];
        snprintf(buffer, sizeof(buffer), "Code: %06u", code);
        return String(buffer);
    }
};

// ============================================================================
// Quad Screen Module (displays 4 values in 2x2 grid)
// ============================================================================
class QuadScreenModule : public ModuleInterface {
private:
    String moduleId;

public:
    QuadScreenModule(const char* id, JsonObject cfg) {
        moduleId = String(id);
        this->id = moduleId.c_str();
        displayName = "Quad Screen";
        defaultRefreshInterval = 60;   // 1 minute
        minRefreshInterval = 30;       // 30 seconds
    }

    bool fetch(String& errorMsg) override {
        // Quad module doesn't fetch - it aggregates data from other modules
        // Update timestamp to show it's active
        JsonObject data = config["modules"][moduleId];
        data["lastUpdate"] = millis() / 1000;
        data["lastSuccess"] = true;
        return true;
    }

    String formatDisplay() override {
        JsonObject data = config["modules"][moduleId];

        // Get the 4 module IDs to display
        String slot1 = data["slot1"] | "";
        String slot2 = data["slot2"] | "";
        String slot3 = data["slot3"] | "";
        String slot4 = data["slot4"] | "";

        // Get values from referenced modules
        String val1 = getModuleValue(slot1);
        String val2 = getModuleValue(slot2);
        String val3 = getModuleValue(slot3);
        String val4 = getModuleValue(slot4);

        // Format for 2x2 grid display
        // This will be rendered specially by the display manager
        return "QUAD:" + val1 + "|" + val2 + "|" + val3 + "|" + val4;
    }

private:
    String getModuleValue(String moduleId) {
        if (moduleId.length() == 0) return "---";

        JsonObject module = config["modules"][moduleId];
        if (module.isNull()) return "N/A";

        String type = module["type"] | "";

        if (type == "crypto") {
            String symbol = module["cryptoSymbol"] | "?";
            float value = module["value"] | 0.0;
            return symbol + ":" + String(value, 0);
        } else if (type == "stock") {
            String ticker = module["ticker"] | "?";
            float value = module["value"] | 0.0;
            return ticker + ":" + String(value, 0);
        } else if (type == "weather") {
            float temp = module["temperature"] | 0.0;
            return String(temp, 0) + "°C";
        } else if (type == "custom") {
            float value = module["value"] | 0.0;
            String unit = module["unit"] | "";
            return String(value, 1) + unit;
        }

        return "---";
    }
};

// ============================================================================
// Module Factory Implementation
// ============================================================================

// ============================================================================
// Include standalone modules
// ============================================================================
#include "modules/countdown_module.cpp"
#include "modules/custom_module.cpp"
#include "modules/http_module.cpp"

ModuleInterface* ModuleFactory::createModule(const char* type, const char* id, JsonObject config) {
    String typeStr = String(type);

    if (typeStr == "crypto") {
        return new GenericCryptoModule(id, config);
    } else if (typeStr == "stock") {
        return new GenericStockModule(id, config);
    } else if (typeStr == "weather") {
        return new GenericWeatherModule(id, config);
    } else if (typeStr == "custom") {
        return new CustomModule();
    } else if (typeStr == "countdown") {
        return new CountdownModule(id, config);
    } else if (typeStr == "http") {
        return new HttpModule(id, config);
    } else if (typeStr == "transit") {
        return new GenericTransitModule(id, config);
    } else if (typeStr == "quad") {
        return new QuadScreenModule(id, config);
    } else if (typeStr == "settings") {
        return new GenericSettingsModule(id, config);
    }

    // Unknown type
    Serial.print("ERROR: Unknown module type: ");
    Serial.println(type);
    return nullptr;
}

const char* ModuleFactory::getModuleTypeName(const char* type) {
    String typeStr = String(type);

    if (typeStr == "crypto") return "Cryptocurrency";
    if (typeStr == "stock") return "Stock Price";
    if (typeStr == "weather") return "Weather";
    if (typeStr == "custom") return "Custom Value";
    if (typeStr == "countdown") return "Countdown";
    if (typeStr == "http") return "HTTP Fetch";
    if (typeStr == "quad") return "Quad Screen";
    if (typeStr == "settings") return "Settings";
    if (typeStr == "transit") return "Transit";

    return "Unknown";
}

bool ModuleFactory::isValidModuleType(const char* type) {
    String typeStr = String(type);
    return (typeStr == "crypto" || typeStr == "stock" || typeStr == "weather" ||
            typeStr == "custom" || typeStr == "countdown" || typeStr == "http" ||
            typeStr == "quad" || typeStr == "settings" || typeStr == "transit");
}
