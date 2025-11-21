#include "module_interface.h"
#include "config.h"
#include "network.h"
#include <ArduinoJson.h>

extern NetworkManager network;

class EthereumModule : public ModuleInterface {
public:
    EthereumModule() {
        id = "ethereum";
        displayName = "Crypto 2";  // Generic name, actual symbol shown in formatDisplay()
        defaultRefreshInterval = 300;  // 5 minutes
        minRefreshInterval = 60;       // 1 minute
    }

    bool fetch(String& errorMsg) override {
        // Get configured crypto ID (default to ethereum)
        JsonObject moduleData = config["modules"]["ethereum"];
        String cryptoId = moduleData["cryptoId"] | "ethereum";

        // Build URL with configured crypto
        String url = "https://api.coingecko.com/api/v3/simple/price?ids=" + cryptoId +
                     "&vs_currencies=usd&include_24hr_change=true";

        String response;
        if (!network.httpGet(url.c_str(), response, errorMsg)) {
            return false;
        }

        return parseResponse(response, errorMsg, cryptoId);
    }

    bool parseResponse(String payload, String& errorMsg, String cryptoId) {
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

        float price = doc[cryptoId]["usd"];
        float change = doc[cryptoId]["usd_24h_change"];

        // Update cache (preserve existing fields like cryptoId/cryptoSymbol/cryptoName)
        JsonObject data = config["modules"]["ethereum"];
        data["value"] = price;
        data["change24h"] = change;
        data["lastUpdate"] = millis() / 1000;
        data["lastSuccess"] = true;

        String cryptoName = data["cryptoName"] | "Ethereum";
        Serial.print(cryptoName);
        Serial.print(" price: $");
        Serial.print(price, 2);
        Serial.print(" (");
        Serial.print(change, 2);
        Serial.println("%)");

        return true;
    }

    String formatDisplay() override {
        JsonObject data = config["modules"]["ethereum"];
        float price = data["value"] | 0.0;
        float change = data["change24h"] | 0.0;

        char buffer[64];
        snprintf(buffer, sizeof(buffer), "$%.2f | %+.1f%%", price, change);
        return String(buffer);
    }
};
