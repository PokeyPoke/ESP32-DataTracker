#ifndef CONSTANTS_H
#define CONSTANTS_H

// ============================================================================
// TIMING CONSTANTS
// ============================================================================

// Display update intervals
#define DISPLAY_UPDATE_INTERVAL 1000    // Update display every 1 second (user-facing updates)
#define QR_UPDATE_INTERVAL 500          // Check for client connection every 500ms (responsiveness vs CPU)

// Serial communication
#define SERIAL_CHECK_INTERVAL 100       // Check serial input every 100ms (balance responsiveness/CPU)

// Configuration persistence
#define MIN_SAVE_INTERVAL 30000         // Minimum 30s between config saves (prevent flash wear)

// Debug features
#define BUTTON_DEBUG_DURATION 30000     // Auto-disable debug mode after 30 seconds (safety timeout)

// Settings security
#define SETTINGS_CODE_REFRESH 30000     // Refresh security code display every 30s (keep user informed)

// ============================================================================
// SECURITY CONSTANTS
// ============================================================================

// Security code timing
#define CODE_EXPIRATION_MS 300000       // 5 minutes (300000ms) - Security code validity period
#define SESSION_EXPIRATION_MS 1800000   // 30 minutes (1800000ms) - Session token validity
#define LOCKOUT_DURATION_MS 60000       // 1 minute (60000ms) - Lockout after failed attempts

// Security limits
#define MAX_FAILED_ATTEMPTS 3           // Maximum failed login attempts before lockout
#define SECURITY_CODE_MIN 100000        // 6-digit code minimum value
#define SECURITY_CODE_MAX 999999        // 6-digit code maximum value
#define SESSION_TOKEN_LENGTH 32         // Length of session token string

// ============================================================================
// BUTTON CONSTANTS
// ============================================================================

#define DEBOUNCE_DELAY 50               // 50ms debounce delay (typical mechanical button bounce)
#define SHORT_PRESS_MAX 1000            // Max 1s for short press (UX standard)
#define LONG_PRESS_MIN 4200             // Min 4.2s for long press (enter/exit brightness mode, prevent accidental)

// Touch sensor
#define TOUCH_THRESHOLD_RATIO 0.7       // 70% of baseline triggers touch (empirically tuned)

// ============================================================================
// NETWORK CONSTANTS
// ============================================================================

// WiFi connection
#define WIFI_CONNECT_TIMEOUT 15000      // 15 second WiFi connection timeout
#define WIFI_RECONNECT_INTERVAL 30000   // 30 seconds between reconnection attempts

// WiFi scanning
#define WIFI_SCAN_CACHE_DURATION 10000  // Cache scan results for 10 seconds
#define WIFI_SCAN_MAX_RETRIES 3         // Maximum scan retry attempts

// HTTP timeouts
#define HTTP_TIMEOUT 10000              // 10 second HTTP request timeout
#define HTTP_MAX_REDIRECTS 5            // Maximum number of HTTP redirects to follow

// Web server
#define WEB_SERVER_PORT 80              // Standard HTTP port
#define WEB_SERVER_TIMEOUT 3000         // 3 second timeout for web server requests

// ============================================================================
// DISPLAY CONSTANTS
// ============================================================================

// Display dimensions (SH1106 OLED)
#define DISPLAY_WIDTH 128               // Display width in pixels
#define DISPLAY_HEIGHT 64               // Display height in pixels

// Display I2C
#define DISPLAY_I2C_ADDRESS 0x3C        // Standard SH1106 I2C address
#define DISPLAY_I2C_SDA 21              // I2C SDA pin (ESP32-C3)
#define DISPLAY_I2C_SCL 20              // I2C SCL pin (ESP32-C3)

// Display brightness
#define DISPLAY_BRIGHTNESS_MIN 0        // Minimum brightness level
#define DISPLAY_BRIGHTNESS_MAX 255      // Maximum brightness level
#define DISPLAY_BRIGHTNESS_DEFAULT 128  // Default brightness (50%)
#define DISPLAY_BRIGHTNESS_STEP 32      // Brightness adjustment step size

// Screensaver
#define SCREENSAVER_TIMEOUT 300000      // 5 minutes before screensaver activates
#define SCREENSAVER_BRIGHTNESS 32       // Low brightness during screensaver

// ============================================================================
// DATA/MEMORY CONSTANTS
// ============================================================================

// JSON document sizes
#define CONFIG_JSON_SIZE 8192           // Configuration document size (must accommodate all modules)
#define MODULE_JSON_SIZE 2048           // Per-module configuration size
#define HTTP_RESPONSE_SIZE 4096         // Maximum HTTP response buffer

// String buffer sizes
#define URL_BUFFER_SIZE 256             // Maximum URL length
#define ERROR_MSG_SIZE 128              // Error message buffer size
#define SSID_BUFFER_SIZE 33             // WiFi SSID max length (32 + null terminator)
#define PASSWORD_BUFFER_SIZE 64         // WiFi password max length

// ============================================================================
// MODULE/SCHEDULER CONSTANTS
// ============================================================================

// Scheduler timing
#define SCHEDULER_MIN_INTERVAL 10       // Minimum 10 seconds between any fetch operations (API rate limiting)
#define SCHEDULER_BACKOFF_MULTIPLIER 2  // Exponential backoff multiplier
#define SCHEDULER_MAX_BACKOFF 300       // Maximum backoff time (5 minutes)

// Module refresh rates
#define MODULE_UPDATE_FAST 60           // Fast update: 1 minute (time, countdown)
#define MODULE_UPDATE_NORMAL 300        // Normal update: 5 minutes (crypto, stock, weather)
#define MODULE_UPDATE_SLOW 3600         // Slow update: 1 hour (rarely changing data)

// ============================================================================
// HARDWARE PIN DEFINITIONS
// ============================================================================

// Button pin (if using physical button)
#define BUTTON_PIN 9                    // GPIO9 for button input

// Touch sensor pin
#define TOUCH_PIN T0                    // Touch0 pin for capacitive touch

// LED indicators (future enhancement)
#define STATUS_LED_PIN 2                // GPIO2 for status LED

// ============================================================================
// FILESYSTEM CONSTANTS
// ============================================================================

#define CONFIG_FILE_PATH "/config.json" // LittleFS config file path
#define LITTLEFS_SIZE (384 * 1024)      // 384KB filesystem size

// ============================================================================
// MISCELLANEOUS CONSTANTS
// ============================================================================

// Serial baud rate
#define SERIAL_BAUD_RATE 115200         // Standard ESP32 serial speed

// Animal names for device identification
#define ANIMAL_NAMES_COUNT 64           // Number of animal names in the array

// Factory reset
#define FACTORY_RESET_HOLD_TIME 10000   // Hold button for 10 seconds to factory reset

// OTA update
#define OTA_BUFFER_SIZE 4096            // Buffer size for OTA updates
#define OTA_PARTITION_SIZE (1536 * 1024) // 1.5MB OTA partition size

// Alerts/Thresholds
#define MAX_ALERTS 10                   // Maximum number of configured alerts
#define ALERT_CHECK_INTERVAL 60000      // Check alerts every 60 seconds

#endif // CONSTANTS_H
