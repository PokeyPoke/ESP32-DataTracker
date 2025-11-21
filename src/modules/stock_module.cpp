#include "module_interface.h"
#include "config.h"
#include "network.h"
#include <ArduinoJson.h>

extern NetworkManager network;

class StockModule : public ModuleInterface {
public:
    StockModule() {
        id = "stock";
        displayName = "Stock Price";
        defaultRefreshInterval = 300;  // 5 minutes
        minRefreshInterval = 60;       // 1 minute
    }

    bool fetch(String& errorMsg) override {
        Serial.println("\n*** StockModule::fetch() CALLED ***");

        // Get ticker from config
        JsonObject stockData = config["modules"]["stock"];
        String ticker = stockData["ticker"] | "AAPL";

        Serial.print("Stock: Fetching price for ");
        Serial.println(ticker);
        Serial.print("Stock: Ticker from config: '");
        Serial.print(ticker);
        Serial.println("'");

        if (ticker.length() == 0) {
            errorMsg = "Ticker is empty";
            Serial.println("Stock: ERROR - ticker is empty");
            return false;
        }

        // Use Yahoo Finance v8 query API (free, no auth required)
        String url = "https://query1.finance.yahoo.com/v8/finance/chart/" + ticker + "?interval=1d&range=1d";
        Serial.print("Stock: URL = ");
        Serial.println(url);

        String response;
        if (!network.httpGet(url.c_str(), response, errorMsg)) {
            Serial.print("Stock: HTTP request failed - ");
            Serial.println(errorMsg);
            return false;
        }

        Serial.print("Stock: Response length = ");
        Serial.println(response.length());
        Serial.print("Stock: Response received, parsing... ");
        bool success = parseResponse(response, errorMsg);
        if (!success) {
            Serial.print("Stock: Parse failed - ");
            Serial.println(errorMsg);
        } else {
            Serial.println("Stock: Parse successful");
        }
        return success;
    }

    bool parseResponse(String payload, String& errorMsg) {
        Serial.print("Stock: parseResponse called with payload length: ");
        Serial.println(payload.length());

        if (payload.length() == 0) {
            errorMsg = "Empty response";
            Serial.println("Stock: ERROR - Empty response payload");
            return false;
        }

        // Print first 100 chars of response for debugging
        Serial.print("Stock: Response preview: ");
        Serial.println(payload.substring(0, min(100, (int)payload.length())));

        StaticJsonDocument<2048> doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
            errorMsg = "JSON parse error: " + String(error.c_str());
            Serial.print("Stock: JSON parse error: ");
            Serial.println(error.c_str());
            return false;
        }

        Serial.println("Stock: JSON parsed successfully");

        // Yahoo Finance v8 response format:
        // chart.result[0].meta.regularMarketPrice = current price
        // chart.result[0].meta.chartPreviousClose = previous close

        if (!doc.containsKey("chart")) {
            errorMsg = "Missing chart data";
            Serial.println("Stock: ERROR - Missing 'chart' in response");
            return false;
        }

        JsonObject chart = doc["chart"];
        if (!chart.containsKey("result") || chart["result"].size() == 0) {
            errorMsg = "Missing result data";
            Serial.println("Stock: ERROR - Missing 'result' array in chart");
            return false;
        }

        JsonObject result = chart["result"][0];
        if (!result.containsKey("meta")) {
            errorMsg = "Missing meta data";
            Serial.println("Stock: ERROR - Missing 'meta' in result");
            return false;
        }

        JsonObject meta = result["meta"];
        if (!meta.containsKey("regularMarketPrice")) {
            errorMsg = "Missing regularMarketPrice";
            Serial.println("Stock: ERROR - Missing 'regularMarketPrice' in meta");
            return false;
        }

        float price = meta["regularMarketPrice"] | 0.0;
        float previousClose = meta["chartPreviousClose"] | price;  // fallback to current if missing

        // Calculate percentage change
        float changePercent = 0.0;
        if (previousClose > 0) {
            changePercent = ((price - previousClose) / previousClose) * 100.0;
        }

        // Get ticker from config to use as symbol
        String ticker = config["modules"]["stock"]["ticker"] | "AAPL";

        Serial.print("Stock: Parsed - Symbol: ");
        Serial.print(ticker);
        Serial.print(", Price: ");
        Serial.print(price, 2);
        Serial.print(", Change: ");
        Serial.println(changePercent, 2);

        // Update cache
        JsonObject data = config["modules"]["stock"].to<JsonObject>();
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
        JsonObject data = config["modules"]["stock"];
        float price = data["value"] | 0.0;
        float change = data["change"] | 0.0;
        String ticker = data["ticker"] | "N/A";

        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%s: $%.2f | %+.1f%%", ticker.c_str(), price, change);
        return String(buffer);
    }
};
