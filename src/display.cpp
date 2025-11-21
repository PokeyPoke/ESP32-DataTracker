#include "display.h"
#include "config.h"
#include <WiFi.h>

DisplayManager::DisplayManager()
    : u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE), currentState(SPLASH),
      currentBrightness(255), brightnessIncreasing(false) {
}

void DisplayManager::init() {
    // Initialize I2C with custom pins if defined
    #ifdef SDA_PIN
    Wire.begin(SDA_PIN, SCL_PIN);
    #endif

    u8g2.begin();
    u8g2.enableUTF8Print();
    Serial.println("Display initialized");
}

void DisplayManager::clear() {
    u8g2.clearBuffer();
}

// Helper to convert UTF8 characters to ASCII (strip accents)
String removeAccents(const char* input) {
    String result = "";
    const char* p = input;

    while (*p) {
        unsigned char c = *p;

        // Check for UTF-8 multi-byte sequences
        if ((c & 0x80) == 0) {
            // Single-byte ASCII character
            result += (char)c;
            p++;
        } else if ((c & 0xE0) == 0xC0) {
            // 2-byte UTF-8 sequence
            unsigned char c1 = *p++;
            unsigned char c2 = *p++;

            // Czech Ř (U+0158) = C5 98, ř (U+0159) = C5 99
            if (c1 == 0xC5 && c2 == 0x98) {
                result += 'R';  // Convert Ř to R
            } else if (c1 == 0xC5 && c2 == 0x99) {
                result += 'r';  // Convert ř to r
            }
            // Czech Č (U+010C) = C4 8C, č (U+010D) = C4 8D
            else if (c1 == 0xC4 && c2 == 0x8C) {
                result += 'C';  // Convert Č to C
            } else if (c1 == 0xC4 && c2 == 0x8D) {
                result += 'c';  // Convert č to c
            }
            // Add more mappings as needed
            else {
                // Unknown accent, skip it
            }
        } else {
            // Skip other multi-byte sequences
            if ((c & 0xF0) == 0xE0) p += 3;      // 3-byte
            else if ((c & 0xF8) == 0xF0) p += 4; // 4-byte
            else p++;
        }
    }

    return result;
}

void DisplayManager::drawCenteredText(const char* text, int y, const uint8_t* font) {
    u8g2.setFont(font);
    int width = u8g2.getStrWidth(text);
    u8g2.drawStr((128 - width) / 2, y, text);
}

void DisplayManager::drawCenteredValue(const char* value, int y) {
    u8g2.setFont(u8g2_font_logisoso24_tr);  // Large font with symbols (tr = transparent)
    int width = u8g2.getStrWidth(value);
    u8g2.drawStr((128 - width) / 2, y, value);
}

// Helper to add thousand separators to a number string
void addThousandSeparators(char* dest, const char* src, char separator) {
    int len = strlen(src);
    int dotPos = -1;

    // Find decimal point
    for (int i = 0; i < len; i++) {
        if (src[i] == '.') {
            dotPos = i;
            break;
        }
    }

    int wholeLen = (dotPos >= 0) ? dotPos : len;
    int destIdx = 0;

    // Copy whole number part with separators
    for (int i = 0; i < wholeLen; i++) {
        if (i > 0 && (wholeLen - i) % 3 == 0 && separator != '\0') {
            dest[destIdx++] = separator;
        }
        dest[destIdx++] = src[i];
    }

    // Copy decimal part if exists
    if (dotPos >= 0) {
        for (int i = dotPos; i < len; i++) {
            dest[destIdx++] = src[i];
        }
    }

    dest[destIdx] = '\0';
}

// Helper to format price with smart decimals or user override
void formatPrice(char* buffer, size_t bufSize, float price, int userDecimals) {
    char tempBuf[32];
    const char* sepStr = config["device"]["thousandSep"] | ",";
    char separator = sepStr[0];

    // Format number without thousand separators first
    if (userDecimals >= 0) {
        snprintf(tempBuf, sizeof(tempBuf), "%.*f", userDecimals, price);
    } else {
        // Auto decimals based on price magnitude
        if (price >= 10000) {
            snprintf(tempBuf, sizeof(tempBuf), "%.0f", price);
        } else if (price >= 1000) {
            snprintf(tempBuf, sizeof(tempBuf), "%.0f", price);
        } else if (price >= 100) {
            snprintf(tempBuf, sizeof(tempBuf), "%.0f", price);
        } else if (price >= 1) {
            snprintf(tempBuf, sizeof(tempBuf), "%.2f", price);
        } else if (price >= 0.001) {
            snprintf(tempBuf, sizeof(tempBuf), "%.4f", price);
        } else {
            snprintf(tempBuf, sizeof(tempBuf), "%.6f", price);
        }
    }

    // Add thousand separators
    char formattedNum[32];
    addThousandSeparators(formattedNum, tempBuf, separator);

    // Add $ prefix
    snprintf(buffer, bufSize, "$%s", formattedNum);
}

void DisplayManager::drawHeader(const char* title) {
    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.drawStr(2, 10, title);
}

void DisplayManager::drawStatusBar(bool wifiConnected, unsigned long lastUpdate, bool isStale, const char* label) {
    u8g2.setFont(u8g2_font_6x10_tr);

    // WiFi indicator centered at bottom
    if (isStale) {
        // Draw WiFi symbol crossed out (centered)
        const char* wifiIcon = "⊘";
        int wifiWidth = u8g2.getStrWidth(wifiIcon);
        u8g2.drawStr((128 - wifiWidth) / 2, 62, wifiIcon);
    }

    // Label in bottom right corner
    if (label && strlen(label) > 0) {
        int labelWidth = u8g2.getStrWidth(label);
        u8g2.drawStr(128 - labelWidth - 2, 62, label);
    }
}

void DisplayManager::showSplash() {
    u8g2.clearBuffer();

    drawCenteredText("DATA TRACKER", 28, u8g2_font_helvB10_tr);
    drawCenteredText("v2.6.4", 42, u8g2_font_6x10_tr);
    drawCenteredText("Auto-Fetch", 54, u8g2_font_5x7_tr);

    u8g2.sendBuffer();
    currentState = SPLASH;
}

void DisplayManager::showConnecting(const char* ssid) {
    u8g2.clearBuffer();

    drawCenteredText("CONNECTING", 25, u8g2_font_helvB08_tr);
    drawCenteredText(ssid, 40, u8g2_font_6x10_tr);
    drawCenteredText("Please wait...", 55, u8g2_font_6x10_tr);

    u8g2.sendBuffer();
    currentState = CONNECTING;
}

void DisplayManager::showConfigMode(const char* apName) {
    u8g2.clearBuffer();

    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.drawStr(25, 15, "SETUP MODE");

    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(5, 30, "1. Connect to WiFi:");
    u8g2.drawStr(10, 42, apName);

    u8g2.drawStr(5, 57, "2. Open browser");

    u8g2.sendBuffer();
    currentState = CONFIG_MODE;
}

void DisplayManager::showError(const char* message) {
    u8g2.clearBuffer();

    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.drawStr(40, 20, "ERROR");

    u8g2.setFont(u8g2_font_6x10_tr);
    int width = u8g2.getStrWidth(message);
    u8g2.drawStr((128 - width) / 2, 40, message);

    u8g2.sendBuffer();
    currentState = ERROR_STATE;
}

// Generic crypto display function that works for any crypto module
void DisplayManager::showCrypto(const char* moduleId, float price, float change24h, unsigned long lastUpdate, bool stale) {
    // Get crypto name and decimals from config using the actual module ID
    JsonObject module = config["modules"][moduleId];
    String cryptoNameStr = module["cryptoName"] | "Crypto";
    int decimals = module["decimals"] | -1;  // -1 = auto

    u8g2.clearBuffer();

    // Format price with thousand separators (includes $)
    char priceStr[20];
    formatPrice(priceStr, sizeof(priceStr), price, decimals);

    // Remove $ from string (we'll draw it separately)
    const char* numOnly = priceStr + 1;  // Skip first char ($)

    // Large number font - use _tr (proportional) so narrow digits don't have extra space
    u8g2.setFont(u8g2_font_logisoso38_tr);
    int numWidth = u8g2.getStrWidth(numOnly);
    bool useLargeFont = true;

    if (numWidth > 115) {
        u8g2.setFont(u8g2_font_logisoso32_tr);
        numWidth = u8g2.getStrWidth(numOnly);
        useLargeFont = false;
    }

    // Medium-sized $ symbol (bigger than 6x10, but still smaller than main number)
    u8g2.setFont(u8g2_font_helvB10_tr);
    int dollarWidth = u8g2.getStrWidth("$");

    // Center the whole thing ($ + number)
    int totalWidth = dollarWidth + 1 + numWidth;
    int startX = (128 - totalWidth) / 2;

    // Draw medium $ aligned to top of numbers
    u8g2.drawStr(startX, useLargeFont ? 22 : 24, "$");

    // Draw large number (proportional for tight spacing)
    u8g2.setFont(useLargeFont ? u8g2_font_logisoso38_tr : u8g2_font_logisoso32_tr);
    u8g2.drawStr(startX + dollarWidth + 2, 40, numOnly);

    // Thin divider line
    u8g2.drawHLine(0, 52, 128);

    // Change percentage - bottom left
    u8g2.setFont(u8g2_font_6x10_tr);
    char changeStr[16];
    const char* sign = (change24h >= 0) ? "+" : "-";
    snprintf(changeStr, sizeof(changeStr), "%s%.1f%%", sign, fabs(change24h));
    u8g2.drawStr(2, 62, changeStr);

    // Label in bottom right
    drawStatusBar(WiFi.isConnected(), lastUpdate, stale, cryptoNameStr.c_str());

    u8g2.sendBuffer();
    currentState = NORMAL;
}

void DisplayManager::showStock(const char* ticker, float price, float change, unsigned long lastUpdate, bool stale) {
    // Get decimals from config
    JsonObject module = config["modules"]["stock"];
    int decimals = module["decimals"] | -1;  // -1 = auto

    u8g2.clearBuffer();

    // Format price with thousand separators (includes $)
    char priceStr[20];
    formatPrice(priceStr, sizeof(priceStr), price, decimals);

    // Remove $ from string (we'll draw it separately)
    const char* numOnly = priceStr + 1;  // Skip first char ($)

    // Large number font - use _tr (proportional) so narrow digits don't have extra space
    u8g2.setFont(u8g2_font_logisoso38_tr);
    int numWidth = u8g2.getStrWidth(numOnly);
    bool useLargeFont = true;

    if (numWidth > 115) {
        u8g2.setFont(u8g2_font_logisoso32_tr);
        numWidth = u8g2.getStrWidth(numOnly);
        useLargeFont = false;
    }

    // Medium-sized $ symbol (bigger than 6x10, but still smaller than main number)
    u8g2.setFont(u8g2_font_helvB10_tr);
    int dollarWidth = u8g2.getStrWidth("$");

    // Center the whole thing ($ + number)
    int totalWidth = dollarWidth + 1 + numWidth;
    int startX = (128 - totalWidth) / 2;

    // Draw medium $ aligned to top of numbers
    u8g2.drawStr(startX, useLargeFont ? 22 : 24, "$");

    // Draw large number (proportional for tight spacing)
    u8g2.setFont(useLargeFont ? u8g2_font_logisoso38_tr : u8g2_font_logisoso32_tr);
    u8g2.drawStr(startX + dollarWidth + 2, 40, numOnly);

    // Thin divider line between value and bottom info
    u8g2.drawHLine(0, 52, 128);

    // Change percentage - bottom left corner
    u8g2.setFont(u8g2_font_6x10_tr);
    char changeStr[16];
    const char* sign = (change >= 0) ? "+" : "-";
    snprintf(changeStr, sizeof(changeStr), "%s%.1f%%", sign, fabs(change));
    u8g2.drawStr(2, 62, changeStr);

    // Status bar with ticker in bottom right
    drawStatusBar(WiFi.isConnected(), lastUpdate, stale, ticker);

    u8g2.sendBuffer();
    currentState = NORMAL;
}

void DisplayManager::showWeather(float temp, const char* condition, const char* location, unsigned long lastUpdate, bool stale) {
    u8g2.clearBuffer();

    // Temperature - use proportional font for tight spacing
    char tempStr[16];
    snprintf(tempStr, sizeof(tempStr), "%.1f", temp);

    u8g2.setFont(u8g2_font_logisoso38_tr);
    int tempWidth = u8g2.getStrWidth(tempStr);

    // Check if too wide, use smaller font
    if (tempWidth + 35 > 120) {
        u8g2.setFont(u8g2_font_logisoso32_tr);
        tempWidth = u8g2.getStrWidth(tempStr);
    }

    u8g2.drawStr((128 - tempWidth - 32) / 2, 40, tempStr);

    // Degree symbol and C - same font as temp
    u8g2.drawStr((128 - tempWidth - 32) / 2 + tempWidth + 4, 40, "°C");

    // Thin divider line
    u8g2.drawHLine(0, 52, 128);

    // Condition - bottom left corner
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(2, 62, condition);

    // Status bar with location in bottom right
    String cleanLocation = removeAccents(location);
    drawStatusBar(WiFi.isConnected(), lastUpdate, stale, cleanLocation.c_str());

    u8g2.sendBuffer();
    currentState = NORMAL;
}

void DisplayManager::showCustomValue(const char* valueStr, const char* label, const char* unit, unsigned long lastUpdate) {
    u8g2.clearBuffer();

    // Value - use proportional font for tight spacing
    u8g2.setFont(u8g2_font_logisoso38_tr);
    int valueWidth = u8g2.getStrWidth(valueStr);

    // Check if too wide, use smaller font
    if (valueWidth > 120) {
        u8g2.setFont(u8g2_font_logisoso32_tr);
        valueWidth = u8g2.getStrWidth(valueStr);
    }

    u8g2.drawStr((128 - valueWidth) / 2, 40, valueStr);

    // Thin divider line
    u8g2.drawHLine(0, 52, 128);

    // Unit - bottom left corner
    if (strlen(unit) > 0) {
        u8g2.setFont(u8g2_font_6x10_tr);
        u8g2.drawStr(2, 62, unit);
    }

    // Status bar with label in bottom right (never stale for manual entry)
    drawStatusBar(WiFi.isConnected(), lastUpdate, false, label);

    u8g2.sendBuffer();
    currentState = NORMAL;
}

// Legacy function for backward compatibility
void DisplayManager::showCustom(float value, const char* label, const char* unit, unsigned long lastUpdate) {
    char valueStr[16];
    snprintf(valueStr, sizeof(valueStr), "%.2f", value);
    showCustomValue(valueStr, label, unit, lastUpdate);
}

void DisplayManager::showTransit(const char* routeName, float nextArrival, const char* stopName, unsigned long lastUpdate, bool stale) {
    u8g2.clearBuffer();

    // Route name at top
    u8g2.setFont(u8g2_font_helvB10_tr);
    String cleanRouteName = removeAccents(routeName);
    u8g2.drawStr(2, 12, cleanRouteName.c_str());

    // Arrival time - large centered
    char arrivalStr[16];
    if (nextArrival < 1.0) {
        snprintf(arrivalStr, sizeof(arrivalStr), "NOW");
    } else {
        snprintf(arrivalStr, sizeof(arrivalStr), "%.0f", nextArrival);
    }

    u8g2.setFont(u8g2_font_logisoso38_tr);
    int arrivalWidth = u8g2.getStrWidth(arrivalStr);

    // Check if too wide, use smaller font
    if (arrivalWidth > 120) {
        u8g2.setFont(u8g2_font_logisoso32_tr);
        arrivalWidth = u8g2.getStrWidth(arrivalStr);
    }

    u8g2.drawStr((128 - arrivalWidth) / 2, 40, arrivalStr);

    // "min" label if not NOW
    if (nextArrival >= 1.0) {
        u8g2.setFont(u8g2_font_helvB10_tr);
        u8g2.drawStr((128 - arrivalWidth) / 2 + arrivalWidth + 4, 40, "min");
    }

    // Thin divider line
    u8g2.drawHLine(0, 52, 128);

    // Stop name - bottom left corner
    if (strlen(stopName) > 0) {
        u8g2.setFont(u8g2_font_6x10_tr);
        String cleanStopName = removeAccents(stopName);
        // Truncate if too long (max ~20 chars for 6x10 font)
        if (cleanStopName.length() > 20) {
            cleanStopName = cleanStopName.substring(0, 17) + "...";
        }
        u8g2.drawStr(2, 62, cleanStopName.c_str());
    }

    // Status bar (minimal - just update time)
    drawStatusBar(WiFi.isConnected(), lastUpdate, stale);

    u8g2.sendBuffer();
    currentState = NORMAL;
}

void DisplayManager::showModule(const char* moduleId) {
    JsonObject module = config["modules"][moduleId];
    unsigned long lastUpdate = module["lastUpdate"] | 0;
    bool stale = isCacheStale(moduleId);

    // Get module type to support dynamic module IDs
    String moduleType = module["type"] | "unknown";

    if (moduleType == "crypto") {
        // Support both old hardcoded IDs and new dynamic IDs
        float price = module["value"] | 0.0;
        float change = module["change24h"] | 0.0;
        const char* cryptoName = module["cryptoName"] | "Crypto";
        const char* cryptoSymbol = module["cryptoSymbol"] | "???";

        Serial.print("Crypto module (");
        Serial.print(moduleId);
        Serial.print(") - Name: ");
        Serial.print(cryptoName);
        Serial.print(", Price: $");
        Serial.println(price);

        // All crypto modules use the same display format
        showCrypto(moduleId, price, change, lastUpdate, stale);
    }
    else if (moduleType == "stock") {
        const char* ticker = module["ticker"] | "STOCK";
        float price = module["value"] | 0.0;
        float change = module["change"] | 0.0;
        showStock(ticker, price, change, lastUpdate, stale);
    }
    else if (moduleType == "weather") {
        float temp = module["temperature"] | 0.0;
        const char* condition = module["condition"] | "Unknown";
        const char* location = module["location"] | "Unknown";
        showWeather(temp, condition, location, lastUpdate, stale);
    }
    else if (moduleType == "custom") {
        float value = module["value"] | 0.0;
        const char* label = module["label"] | "CUSTOM";
        const char* unit = module["unit"] | "";
        int decimals = module["decimals"] | -1;

        // Format value based on decimals setting
        char valueStr[16];
        if (decimals == -1) {
            // Auto mode
            snprintf(valueStr, sizeof(valueStr), "%.2f", value);
        } else {
            // Use specified decimals
            char formatStr[8];
            snprintf(formatStr, sizeof(formatStr), "%%.%df", decimals);
            snprintf(valueStr, sizeof(valueStr), formatStr, value);
        }

        showCustomValue(valueStr, label, unit, lastUpdate);
    }
    else if (moduleType == "transit") {
        const char* routeName = module["routeName"] | "Transit";
        float nextArrival = module["nextArrival"] | 0.0;
        const char* stopName = module["stopName"] | "";
        showTransit(routeName, nextArrival, stopName, lastUpdate, stale);
    }
    else if (moduleType == "settings") {
        uint32_t code = module["securityCode"] | 0;
        unsigned long timeRemaining = module["codeTimeRemaining"] | 0;
        unsigned long lastUpdate = module["lastUpdate"] | 0;
        String ip = WiFi.localIP().toString();

        Serial.print("Showing settings module - Code: ");
        Serial.print(code);
        Serial.print(", Time remaining: ");
        Serial.print(timeRemaining / 1000);
        Serial.print("s, Last update: ");
        Serial.print(lastUpdate);
        Serial.print(", IP: ");
        Serial.println(ip);

        // If code is 0 or expired, trigger a fetch
        if (code == 0 || timeRemaining == 0) {
            Serial.println("WARNING: Security code not generated or expired! Code should be generated by scheduler.");
        }

        showSettings(code, ip.c_str(), timeRemaining);
    }
    else if (moduleType == "countdown") {
        float value = module["value"] | 0.0;
        const char* label = module["label"] | "Event";
        const char* countdownType = module["countdownType"] | "date";
        int decimals = module["decimals"] | -1;

        // Format countdown value with decimal control
        char valueStr[16];
        char unitStr[16] = "";

        if (String(countdownType) == "datetime") {
            // For datetime countdown, format as hours
            if (decimals == 0) {
                int hours = (int)value;
                snprintf(valueStr, sizeof(valueStr), "%d", hours);
                snprintf(unitStr, sizeof(unitStr), "h");
            } else if (decimals == -1) {
                // Auto: show integer hours
                snprintf(valueStr, sizeof(valueStr), "%d", (int)value);
                snprintf(unitStr, sizeof(unitStr), "h");
            } else {
                // Show with decimals
                char formatStr[8];
                snprintf(formatStr, sizeof(formatStr), "%%.%df", decimals);
                snprintf(valueStr, sizeof(valueStr), formatStr, value);
                snprintf(unitStr, sizeof(unitStr), "h");
            }
        } else {
            // For date countdown, format as days
            if (decimals == 0) {
                int days = (int)value;
                snprintf(valueStr, sizeof(valueStr), "%d", days);
                if (days == 1 || days == -1) {
                    snprintf(unitStr, sizeof(unitStr), "day");
                } else {
                    snprintf(unitStr, sizeof(unitStr), "days");
                }
            } else if (decimals == -1) {
                // Auto: show integer days
                snprintf(valueStr, sizeof(valueStr), "%d", (int)value);
                int days = (int)value;
                if (days == 1 || days == -1) {
                    snprintf(unitStr, sizeof(unitStr), "day");
                } else {
                    snprintf(unitStr, sizeof(unitStr), "days");
                }
            } else {
                // Show with decimals
                char formatStr[8];
                snprintf(formatStr, sizeof(formatStr), "%%.%df", decimals);
                snprintf(valueStr, sizeof(valueStr), formatStr, value);
                snprintf(unitStr, sizeof(unitStr), "days");
            }
        }

        showCustomValue(valueStr, label, unitStr, lastUpdate);
    }
    else if (moduleType == "http") {
        float value = module["value"] | 0.0;
        const char* label = module["label"] | "API Data";
        const char* unit = module["unit"] | "";
        showCustom(value, label, unit, lastUpdate);
    }
    else if (moduleType == "quad") {
        // Quad screen - show 4 modules in 2x2 grid
        String slot1 = module["slot1"] | "";
        String slot2 = module["slot2"] | "";
        String slot3 = module["slot3"] | "";
        String slot4 = module["slot4"] | "";

        Serial.print("Showing quad module - Slots: ");
        Serial.print(slot1);
        Serial.print(", ");
        Serial.print(slot2);
        Serial.print(", ");
        Serial.print(slot3);
        Serial.print(", ");
        Serial.println(slot4);

        showQuadScreen(slot1.c_str(), slot2.c_str(), slot3.c_str(), slot4.c_str(), lastUpdate, stale);
    }
    else {
        Serial.print("ERROR: Unknown module type '");
        Serial.print(moduleType);
        Serial.print("' for module ID '");
        Serial.print(moduleId);
        Serial.println("'");
        showError("Unknown module");
    }
}

void DisplayManager::showButtonStatus(bool isPressed, int digitalValue, int analogValue) {
    u8g2.clearBuffer();

    // Title
    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.drawStr(20, 10, "BUTTON DEBUG");

    // Button status indicator
    u8g2.setFont(u8g2_font_logisoso24_tn);
    if (isPressed) {
        u8g2.drawStr(40, 35, "ON");
    } else {
        u8g2.drawStr(30, 35, "OFF");
    }

    // Digital value
    u8g2.setFont(u8g2_font_6x10_tr);
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Digital: %d (%s)", digitalValue, digitalValue ? "HIGH" : "LOW");
    u8g2.drawStr(2, 48, buffer);

    // Analog value
    snprintf(buffer, sizeof(buffer), "Analog: %d", analogValue);
    u8g2.drawStr(2, 60, buffer);

    u8g2.sendBuffer();
}

void DisplayManager::drawQRCode(const char* data, int x, int y, int scale) {
    // Create QR code
    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(3)];  // Version 3
    qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, data);

    // Draw each module as a filled box
    for (uint8_t qy = 0; qy < qrcode.size; qy++) {
        for (uint8_t qx = 0; qx < qrcode.size; qx++) {
            if (qrcode_getModule(&qrcode, qx, qy)) {
                u8g2.drawBox(x + (qx * scale), y + (qy * scale), scale, scale);
            }
        }
    }
}

void DisplayManager::showWiFiQR(const char* ssid, const char* password) {
    u8g2.clearBuffer();

    // Create WiFi QR code data
    String qrData = "WIFI:T:WPA;S:" + String(ssid) + ";P:" + String(password) + ";;";

    // Draw QR code on RIGHT side
    // QR Version 3 = 29x29 modules, scale 2 = 58x58 pixels
    // Position: x=70 (right side), y=3 (vertically centered)
    drawQRCode(qrData.c_str(), 70, 3, 2);

    // Draw info on LEFT side with plenty of space
    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.drawStr(2, 10, "Step 1/3");

    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(2, 24, "Scan QR");
    u8g2.drawStr(2, 34, "to join");

    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(2, 48, String(ssid).c_str());
    u8g2.drawStr(2, 58, String(password).c_str());

    u8g2.sendBuffer();
}

void DisplayManager::showURLQR() {
    u8g2.clearBuffer();

    // Create URL QR code - shorter URL
    String qrData = "http://dt.local";

    // Draw QR code on RIGHT side - same size as Step 1
    // QR Version 3 = 29x29 modules, scale 2 = 58x58 pixels (same as WiFi QR)
    // Position: x=70 (right side, same as WiFi QR), y=3 (vertically centered)
    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(3)];  // Version 3 (same as WiFi QR)
    qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, qrData.c_str());

    // Draw QR on right side - same position and scale as WiFi QR
    int qrX = 70;  // Right side (same as Step 1)
    int qrY = 3;   // Vertically centered (same as Step 1)
    int scale = 2; // Same scale as Step 1
    for (uint8_t qy = 0; qy < qrcode.size; qy++) {
        for (uint8_t qx = 0; qx < qrcode.size; qx++) {
            if (qrcode_getModule(&qrcode, qx, qy)) {
                u8g2.drawBox(qrX + (qx * scale), qrY + (qy * scale), scale, scale);
            }
        }
    }

    // Draw info on LEFT side with plenty of space
    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.drawStr(2, 10, "Step 2/3");

    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(2, 24, "Scan QR");
    u8g2.drawStr(2, 34, "to open");
    u8g2.drawStr(2, 44, "setup");

    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(2, 58, "dt.local");

    u8g2.sendBuffer();
}

void DisplayManager::showSettings(uint32_t securityCode, const char* deviceIP, unsigned long timeRemaining) {
    u8g2.clearBuffer();

    // Create URL QR code with device IP
    String qrData = "http://" + String(deviceIP);

    // Draw QR code on RIGHT side
    // QR Version 3 = 29x29 modules, scale 2 = 58x58 pixels
    // Position: x=70 (right side), y=3 (vertically centered)
    drawQRCode(qrData.c_str(), 70, 3, 2);

    // Draw settings info on LEFT side
    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.drawStr(2, 10, "SETTINGS");

    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(2, 22, "Scan QR");
    u8g2.drawStr(2, 32, "to open");

    // Show security code prominently
    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.drawStr(2, 44, "Code:");
    u8g2.setFont(u8g2_font_helvB10_tr);
    char codeStr[8];
    snprintf(codeStr, sizeof(codeStr), "%06u", securityCode);
    u8g2.drawStr(2, 56, codeStr);

    // Note: Removed time countdown to prevent display flickering
    // QR codes need to be completely static for scanning

    u8g2.sendBuffer();
}

void DisplayManager::showQuadScreen(const char* slot1, const char* slot2, const char* slot3, const char* slot4, unsigned long lastUpdate, bool stale) {
    u8g2.clearBuffer();

    // Helper struct to hold label, value, and whether it has a currency symbol
    struct ModuleData {
        String label;
        String value;
        bool hasCurrency;  // true if value should be prefixed with small $
    };

    // Helper to apply common abbreviations
    auto abbreviate = [](String text) -> String {
        // City abbreviations
        if (text == "San Francisco") return "SF";
        if (text == "Los Angeles") return "LA";
        if (text == "New York") return "NY";
        if (text == "Washington") return "DC";
        if (text == "Las Vegas") return "Vegas";
        if (text == "Saint Petersburg") return "St Pete";
        if (text == "Salt Lake City") return "SLC";

        // Common label abbreviations
        if (text == "Temperature") return "Temp";
        if (text == "Humidity") return "Humid";
        if (text == "Pressure") return "Press";
        if (text == "Precipitation") return "Precip";
        if (text == "Visibility") return "Vis";
        if (text == "Wind Speed") return "Wind";
        if (text == "Battery") return "Batt";
        if (text == "Voltage") return "Volt";
        if (text == "Current") return "Curr";
        if (text == "Power") return "Pwr";

        // If no abbreviation found and text is too long, truncate
        if (text.length() > 10) {
            return text.substring(0, 10);
        }

        return text;
    };

    auto getModuleData = [&abbreviate](const char* moduleId) -> ModuleData {
        if (strlen(moduleId) == 0) return {"", "---", false};

        JsonObject module = config["modules"][moduleId];
        if (module.isNull()) return {"", "N/A", false};

        String type = module["type"] | "";

        if (type == "crypto") {
            String symbol = module["cryptoSymbol"] | "?";
            float value = module["value"] | 0.0;
            int decimals = module["decimals"] | -1;

            String valueStr;
            if (decimals >= 0) {
                // User specified decimals - NO $ prefix
                char buf[16];
                snprintf(buf, sizeof(buf), "%.*f", decimals, value);
                valueStr = String(buf);
            } else {
                // Auto formatting - avoid K suffix, just show numbers - NO $ prefix
                if (value >= 10000) {
                    valueStr = String((int)value);
                } else if (value >= 100) {
                    valueStr = String((int)value);
                } else if (value >= 1) {
                    valueStr = String(value, 2);
                } else if (value >= 0.001) {
                    valueStr = String(value, 4);
                } else {
                    valueStr = String(value, 6);
                }
            }
            return {symbol, valueStr, true};  // hasCurrency = true
        } else if (type == "stock") {
            String ticker = module["ticker"] | "?";
            float value = module["value"] | 0.0;
            int decimals = module["decimals"] | -1;

            String valueStr;
            if (decimals >= 0) {
                // User specified decimals - NO $ prefix
                char buf[16];
                snprintf(buf, sizeof(buf), "%.*f", decimals, value);
                valueStr = String(buf);
            } else {
                // Auto formatting - avoid K suffix - NO $ prefix
                if (value >= 10000) {
                    valueStr = String((int)value);
                } else if (value >= 100) {
                    valueStr = String((int)value);
                } else if (value >= 1) {
                    valueStr = String(value, 2);
                } else if (value >= 0.01) {
                    valueStr = String(value, 3);
                } else {
                    valueStr = String(value, 5);
                }
            }
            return {ticker, valueStr, true};  // hasCurrency = true
        } else if (type == "weather") {
            float temp = module["temperature"] | 0.0;
            String location = module["location"] | "";
            String unit = module["unit"] | "C";
            // Apply abbreviations (e.g., "San Francisco" -> "SF", or truncate if too long)
            location = abbreviate(location);
            return {location, String(temp, 1) + "°" + unit, false};
        } else if (type == "custom") {
            float value = module["value"] | 0.0;
            String label = module["label"] | "";
            String unit = module["unit"] | "";
            // Apply abbreviations (e.g., "Temperature" -> "Temp", or truncate if too long)
            label = abbreviate(label);
            return {label, String(value, 1) + unit, false};
        }

        return {"", "---", false};
    };

    // Get data for all 4 slots
    ModuleData data1 = getModuleData(slot1);
    ModuleData data2 = getModuleData(slot2);
    ModuleData data3 = getModuleData(slot3);
    ModuleData data4 = getModuleData(slot4);

    // Dividing lines
    u8g2.drawHLine(0, 32, 128);  // Horizontal middle
    u8g2.drawVLine(64, 0, 64);    // Vertical middle

    // Helper to draw one quadrant
    auto drawQuadrant = [&](int x, int y, ModuleData data) {
        // Small font for label at top
        u8g2.setFont(u8g2_font_5x7_tr);
        if (data.label.length() > 0) {
            int labelWidth = u8g2.getStrWidth(data.label.c_str());
            u8g2.drawStr(x + (64 - labelWidth) / 2, y + 8, data.label.c_str());
        }

        // Draw value with optional medium $ symbol
        if (data.hasCurrency) {
            // Medium $ font (bigger but still smaller than number)
            u8g2.setFont(u8g2_font_helvB08_tr);
            int dollarWidth = u8g2.getStrWidth("$");

            // Large font for number
            u8g2.setFont(u8g2_font_helvB14_tr);
            int valueWidth = u8g2.getStrWidth(data.value.c_str());

            // Center both together
            int totalWidth = dollarWidth + 1 + valueWidth;
            int startX = x + (64 - totalWidth) / 2;

            // Draw medium $ at top-left of number
            u8g2.setFont(u8g2_font_helvB08_tr);
            u8g2.drawStr(startX, y + 22, "$");

            // Draw large number
            u8g2.setFont(u8g2_font_helvB14_tr);
            u8g2.drawStr(startX + dollarWidth + 1, y + 26, data.value.c_str());
        } else {
            // No currency symbol - just draw value centered
            u8g2.setFont(u8g2_font_helvB14_tr);
            int valueWidth = u8g2.getStrWidth(data.value.c_str());
            u8g2.drawStr(x + (64 - valueWidth) / 2, y + 26, data.value.c_str());
        }
    };

    // Draw all 4 quadrants
    drawQuadrant(0, 0, data1);    // Top-left
    drawQuadrant(64, 0, data2);   // Top-right
    drawQuadrant(0, 32, data3);   // Bottom-left
    drawQuadrant(64, 32, data4);  // Bottom-right

    u8g2.sendBuffer();
}

void DisplayManager::showModuleLoading(const char* moduleName, int progress) {
    u8g2.clearBuffer();

    // Module name at top
    drawHeader(moduleName);

    // "Loading..." text
    u8g2.setFont(u8g2_font_helvB08_tr);
    int textWidth = u8g2.getStrWidth("Loading...");
    u8g2.drawStr((128 - textWidth) / 2, 30, "Loading...");

    // Progress bar
    int barWidth = 100;
    int barHeight = 12;
    int barX = (128 - barWidth) / 2;
    int barY = 38;

    // Draw progress bar outline
    u8g2.drawFrame(barX, barY, barWidth, barHeight);

    // Fill progress bar
    int fillWidth = (barWidth - 4) * progress / 100;
    if (fillWidth > 0) {
        u8g2.drawBox(barX + 2, barY + 2, fillWidth, barHeight - 4);
    }

    // Progress percentage
    u8g2.setFont(u8g2_font_6x10_tr);
    char percentStr[8];
    snprintf(percentStr, sizeof(percentStr), "%d%%", progress);
    int percentWidth = u8g2.getStrWidth(percentStr);
    u8g2.drawStr((128 - percentWidth) / 2, 60, percentStr);

    u8g2.sendBuffer();
}

void DisplayManager::setBrightness(uint8_t level) {
    currentBrightness = level;
    u8g2.setContrast(level);
    Serial.print("Display brightness set to: ");
    Serial.println(level);
}

void DisplayManager::cycleBrightness() {
    // Define 9 brightness levels: 1%, 3%, 5%, 10%, 20%, 40%, 60%, 80%, 100%
    const uint8_t levels[] = {3, 8, 13, 26, 51, 102, 153, 204, 255};
    const int levelCount = 9;

    // Find current level index
    int currentIndex = -1;
    for (int i = 0; i < levelCount; i++) {
        if (currentBrightness == levels[i]) {
            currentIndex = i;
            break;
        }
    }

    // If not at a predefined level, find closest
    if (currentIndex == -1) {
        int minDiff = 9999;
        for (int i = 0; i < levelCount; i++) {
            int diff = abs(currentBrightness - levels[i]);
            if (diff < minDiff) {
                minDiff = diff;
                currentIndex = i;
            }
        }
    }

    // Ping-pong logic: go high to low, then low to high
    if (brightnessIncreasing) {
        if (currentIndex >= levelCount - 1) {
            // At max, reverse direction
            brightnessIncreasing = false;
            currentIndex--;
        } else {
            currentIndex++;
        }
    } else {
        if (currentIndex <= 0) {
            // At min, reverse direction
            brightnessIncreasing = true;
            currentIndex++;
        } else {
            currentIndex--;
        }
    }

    currentBrightness = levels[currentIndex];
    setBrightness(currentBrightness);

    // Show brightness level on screen briefly
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.drawStr(30, 20, "BRIGHTNESS");

    // Draw brightness bar
    int barWidth = 100;
    int barHeight = 20;
    int barX = 14;
    int barY = 30;

    u8g2.drawFrame(barX, barY, barWidth, barHeight);
    int fillWidth = (barWidth - 4) * currentBrightness / 255;
    u8g2.drawBox(barX + 2, barY + 2, fillWidth, barHeight - 4);

    // Show percentage
    u8g2.setFont(u8g2_font_6x10_tr);
    char percentStr[8];
    snprintf(percentStr, sizeof(percentStr), "%d%%", (currentBrightness * 100) / 255);
    int percentWidth = u8g2.getStrWidth(percentStr);
    u8g2.drawStr((128 - percentWidth) / 2, 62, percentStr);

    u8g2.sendBuffer();
}

uint8_t DisplayManager::getBrightness() {
    return currentBrightness;
}
