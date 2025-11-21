# ESP32-C3 Data Tracker

> Display real-time data from public APIs on an OLED screen - no backend server required!

![Version](https://img.shields.io/badge/version-1.0.0-blue)
![Platform](https://img.shields.io/badge/platform-ESP32--C3-green)
![License](https://img.shields.io/badge/license-MIT-orange)

## 📋 Overview

This project turns your ESP32-C3 Super Mini into a standalone data display device that fetches and shows real-time metrics on an SH1106 OLED display. Perfect for tracking cryptocurrency prices, stock quotes, weather, or any custom numeric value.

### Key Features

- 📊 **Bitcoin/Ethereum prices** from CoinGecko (free, no API key)
- 📈 **Stock prices** from Yahoo Finance (free, no API key)
- 🌤️ **Weather data** from Open-Meteo (free, no API key)
- 🔢 **Custom values** (manual entry, no fetching)
- 🔘 **Button control** to cycle through metrics (optional)
- 📡 **Direct API calls** - no backend server needed
- 💾 **Smart caching** with automatic retry and backoff
- ⚙️ **Captive portal** for easy WiFi and module setup
- 🔋 **Low power** - runs continuously on USB power

## 🛠️ Hardware Required

| Component | Specification |
|-----------|--------------|
| Microcontroller | ESP32-C3 Super Mini |
| Display | SH1106 OLED (128×64, I2C) |
| Power | USB-C cable |
| Button (optional) | Push button (normally open) |
| Wires | Jumper wires for connections |

### Cost Estimate
- ESP32-C3 Super Mini: ~$3-5
- SH1106 OLED Display: ~$5-8
- **Total: ~$8-13 USD**

## 🔌 Wiring Diagram

```
ESP32-C3 Super Mini    →    SH1106 OLED Display
─────────────────────────────────────────────────
3.3V                   →    VCC
GND                    →    GND
GPIO8 (SDA)            →    SDA
GPIO9 (SCL)            →    SCL

Optional Button (for cycling metrics):
GPIO2                  →    Button → GND
(Internal pull-up resistor is enabled)
```

**Important Notes:**
- Verify your ESP32-C3 board's I2C pins (some variants use different pins)
- Most SH1106 displays use I2C address 0x3C (changeable in platformio.ini)
- Button is optional - you can use serial commands to switch modules

## 🚀 Quick Start

### ⚡ Option 1: Web Flasher (Recommended for Beginners)

**Flash firmware directly from your browser - no software installation required!**

1. **Visit the web flasher**: [Flash Firmware](https://yourusername.github.io/DataTracker/flash.html)
   *(Replace with your GitHub Pages URL)*
2. **Connect your ESP32-C3** via USB
3. **Click "Install Firmware"** and follow the prompts
4. **Done!** Configure via WiFi captive portal

**Requirements:**
- Chrome 89+ or Edge 89+ browser
- USB data cable (not just charging cable)
- No software installation needed!

📖 **[Full Web Flasher Guide](docs/WEB_FLASHER_GUIDE.md)**

---

### 🔧 Option 2: Build from Source (For Developers)

#### 1. Install PlatformIO

If you haven't already, install PlatformIO:
- **VS Code**: Install the PlatformIO IDE extension
- **CLI**: `pip install platformio`

#### 2. Clone and Build

```bash
# Clone the repository
git clone <your-repo-url>
cd DataTracker

# Build and upload (interactive script)
./quick_start.sh

# Or manually:
pio run --target upload
pio device monitor
```

### 3. Initial Configuration

1. **Power on the device** - It will create a WiFi access point
2. **Connect to WiFi**: Look for network `DataTracker-XXXX` (XXXX = last 4 digits of MAC)
3. **Open browser**: Should auto-redirect to setup page, or go to `http://192.168.4.1`
4. **Configure**:
   - Select your WiFi network
   - Enter WiFi password
   - Choose which metric to display (Bitcoin, Ethereum, Stock, Weather, or Custom)
   - Configure metric-specific settings (e.g., stock ticker, weather location)
   - Set refresh interval (1-30 minutes)
5. **Save** - Device will restart and connect to your WiFi

### 4. View Your Data!

The display will automatically update at your configured interval. If you enabled button support, press the button to cycle through different metrics.

## 📱 Supported Metrics

### Bitcoin Price
- **API**: CoinGecko (free, no key required)
- **Shows**: Current BTC/USD price and 24h change percentage
- **Refresh**: Default 5 minutes (configurable)

### Ethereum Price
- **API**: CoinGecko (free, no key required)
- **Shows**: Current ETH/USD price and 24h change percentage
- **Refresh**: Default 5 minutes (configurable)

### Stock Price
- **API**: Yahoo Finance (free, no key required)
- **Configuration**: Stock ticker symbol (e.g., AAPL, TSLA, GOOGL)
- **Shows**: Current price and daily change percentage
- **Refresh**: Default 5 minutes (configurable)
- **Note**: Only updates during market hours

### Weather
- **API**: Open-Meteo (free, no key required)
- **Configuration**: Latitude, longitude, and optional location name
- **Shows**: Temperature (°C), condition (Clear/Cloudy/Rain/etc.), and location
- **Refresh**: Default 15 minutes (configurable)

### Custom Number
- **API**: None (manual entry)
- **Configuration**: Value, label, and optional unit
- **Shows**: Your custom value with label and unit
- **Refresh**: None (update via config portal or serial console)

## 🎮 Button Controls

If `ENABLE_BUTTON=true` in platformio.ini:

| Action | Function |
|--------|----------|
| **Short press** (< 1s) | Cycle to next module |
| **Long press** (3-10s) | Enter config mode (restart in AP mode) |
| **Very long press** (10s+) | Factory reset (clears all settings) |

## 💻 Serial Console Commands

Connect via serial at 115200 baud and type:

```
help      - Show all commands
config    - Display current configuration (JSON)
wifi      - Show WiFi status
fetch     - Force immediate data fetch
cache     - Show all cached module data
modules   - List available modules
switch    - Switch to next module
reset     - Factory reset (clears all settings)
restart   - Reboot device
```

## ⚙️ Configuration

### Build Flags (platformio.ini)

```ini
build_flags =
    -D ENABLE_BUTTON=true       ; Enable/disable button support
    -D DEBUG_MODE=false         ; Enable debug logging
    -D BUTTON_PIN=2             ; GPIO pin for button
    -D SDA_PIN=8                ; I2C SDA pin
    -D SCL_PIN=9                ; I2C SCL pin
    -D I2C_ADDRESS=0x3C         ; Display I2C address
```

### Storage Structure

Configuration is stored in LittleFS as JSON at `/config.json`:

```json
{
  "wifi": {
    "ssid": "MyNetwork",
    "password": "secret123"
  },
  "device": {
    "activeModule": "bitcoin",
    "enableButton": true,
    "refreshInterval": 300
  },
  "modules": {
    "bitcoin": {
      "value": 43250.00,
      "change24h": 2.3,
      "lastUpdate": 1698765432,
      "lastSuccess": true
    },
    ...
  }
}
```

## 🔧 Troubleshooting

### Display shows "Connecting to WiFi..." indefinitely
**Solution**:
- Hold button for 3 seconds to enter config mode
- Or send `reset` command via serial console
- Check WiFi password is correct

### Display shows "-- " with "!" indicator
**Cause**: Stale or missing data
**Solution**:
- Check WiFi connection (serial command: `wifi`)
- Verify API endpoint is reachable
- Check serial output for specific error messages
- Try forcing a fetch (serial command: `fetch`)

### Button not responding
**Solutions**:
- Verify `ENABLE_BUTTON=true` in platformio.ini
- Check button wiring (GPIO2 to GND when pressed)
- Rebuild and re-flash firmware
- Try factory reset (hold button 10+ seconds)

### Device crashes or reboots randomly
**Possible causes**:
- Memory leak (check serial output for heap info)
- Power supply issue (try different USB cable/adapter)
- WiFi signal too weak (move closer to router)

### API rate limiting errors
**Solution**:
- Increase refresh interval in config
- Check scheduler enforces minimum intervals (serial: `config`)
- CoinGecko free tier: 50 calls/minute
- Yahoo Finance: Generally unlimited
- Open-Meteo: Unlimited

## 📊 Performance & Resources

### Memory Usage
- **Flash**: ~1.2MB firmware + 384KB LittleFS
- **RAM**: ~50KB used, ~270KB free
- **Heap**: Stable at ~200KB free during operation

### Power Consumption
- **Active (fetching)**: ~120mA @ 3.3V
- **Idle (WiFi connected)**: ~80mA @ 3.3V
- **Deep sleep**: Not implemented (future feature)

### Network Usage
- **Per fetch**: ~1-5KB data transfer
- **Default config**: ~30KB/hour (5min refresh)

## 🔐 Security & Privacy

- **No data logging**: Device doesn't store or transmit personal data
- **Local storage only**: All config stored on device flash
- **Open WiFi AP**: Config portal runs without password (secure your network)
- **No HTTPS cert validation**: Disabled for simplicity (acceptable for public APIs)

## 📚 Project Structure

```
DataTracker/
├── platformio.ini              # Build configuration
├── README.md                   # This file
├── LICENSE                     # MIT License
├── src/
│   ├── main.cpp                # Application entry point
│   ├── config.cpp              # Configuration management
│   ├── display.cpp             # Display driver
│   ├── network.cpp             # WiFi & HTTP client
│   ├── scheduler.cpp           # Rate limiting & fetch scheduler
│   ├── button.cpp              # Button handler
│   └── modules/
│       ├── module_interface.h  # Module interface definition
│       ├── bitcoin_module.cpp  # Bitcoin price module
│       ├── ethereum_module.cpp # Ethereum price module
│       ├── stock_module.cpp    # Stock price module
│       ├── weather_module.cpp  # Weather module
│       └── custom_module.cpp   # Custom value module
├── include/
│   ├── config.h
│   ├── display.h
│   ├── network.h
│   ├── scheduler.h
│   └── button.h
└── data/
    └── example_config.json     # Example configuration
```

## 🎯 Future Enhancements

- [ ] **Deep sleep mode** for battery-powered operation
- [ ] **BLE configuration** alternative to WiFi AP
- [ ] **Multi-language support** for display text
- [ ] **Sound alerts** via buzzer for threshold crossing
- [ ] **MQTT integration** for home automation
- [ ] **OTA firmware updates** via web interface
- [ ] **Historical graphs** on display (if RAM permits)
- [ ] **Additional APIs**: GitHub stars, RSS feeds, custom webhooks

## 🤝 Contributing

Contributions welcome! Please:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **CoinGecko** - Free cryptocurrency API
- **Yahoo Finance** - Free stock price data
- **Open-Meteo** - Free weather API
- **U8g2 Library** - Excellent OLED display driver
- **ArduinoJson** - JSON parsing library
- **PlatformIO** - Modern embedded development platform

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/yourusername/DataTracker/issues)
- **Discussions**: [GitHub Discussions](https://github.com/yourusername/DataTracker/discussions)
- **Documentation**: See `/docs` folder (coming soon)

---

**Built with ❤️ for the maker community**

*Star ⭐ this repo if you found it useful!*
