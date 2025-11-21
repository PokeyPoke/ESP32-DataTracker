# Web Flasher Guide - ESP32-C3 Data Tracker

## 🌐 What is the Web Flasher?

The web flasher is a browser-based tool that allows you to install firmware on your ESP32-C3 **without installing any software**. No PlatformIO, no Arduino IDE, no drivers - just your browser!

---

## ✨ Benefits

- ✅ **No Installation Required** - Works directly in Chrome/Edge browser
- ✅ **Beginner Friendly** - Simple point-and-click interface
- ✅ **Factory Reset Option** - Start fresh if you have problems
- ✅ **Fast** - Flash firmware in under 2 minutes
- ✅ **Safe** - Uses official ESP Web Tools library
- ✅ **Cross-Platform** - Works on Windows, macOS, and Linux

---

## 🚀 Quick Start

### Step 1: Open the Web Flasher

Visit: **https://yourusername.github.io/DataTracker/flash.html**

*(Replace with your actual GitHub Pages URL after deployment)*

### Step 2: Connect Your Device

1. Plug your ESP32-C3 into your computer via USB-C cable
2. Make sure it's a **data cable**, not just a charging cable

### Step 3: Flash Firmware

1. Click **"Install Firmware"** button
2. Select the serial port when prompted (usually labeled "USB Serial" or similar)
3. Click **"Connect"**
4. Wait for installation to complete (1-2 minutes)
5. Done!

### Step 4: Configure

1. Disconnect and reconnect USB
2. Look for WiFi network: `DataTracker-XXXX`
3. Connect and configure via web interface

---

## 🔧 Supported Browsers

The web flasher requires a browser with **Web Serial API** support:

| Browser | Version | Supported |
|---------|---------|-----------|
| **Chrome** | 89+ | ✅ Yes |
| **Edge** | 89+ | ✅ Yes |
| **Opera** | 75+ | ✅ Yes |
| **Brave** | 1.24+ | ✅ Yes |
| Firefox | Any | ❌ No |
| Safari | Any | ❌ No |

**Note:** Firefox and Safari do not support Web Serial API and cannot use the web flasher. Use Chrome or Edge instead.

---

## 🆘 Troubleshooting

### Problem: "No port found" or port not showing up

**Possible Causes:**
- USB cable is charge-only (not a data cable)
- USB drivers not installed (Windows)
- Device not detected by computer

**Solutions:**
1. Try a different USB cable (must support data transfer)
2. Try a different USB port on your computer
3. **Windows**: Install CH340 or CP2102 USB drivers
   - [CH340 Driver Download](http://www.wch.cn/downloads/CH341SER_ZIP.html)
   - [CP2102 Driver Download](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)
4. **macOS**: No drivers needed, but grant browser permission to access port
5. **Linux**: Add user to `dialout` group: `sudo usermod -a -G dialout $USER`

---

### Problem: "Failed to connect" or "Timeout"

**Possible Causes:**
- Another program is using the serial port
- Device is in wrong state
- Browser doesn't have permission

**Solutions:**
1. Close Arduino IDE, PlatformIO, serial monitors, etc.
2. Disconnect and reconnect USB cable
3. Try holding **BOOT button** while connecting USB (enter bootloader mode)
4. Refresh the web flasher page and try again
5. Try "Factory Reset & Install" option instead

---

### Problem: Flash succeeds but device doesn't work

**Possible Causes:**
- Old configuration interfering
- Incomplete flash

**Solutions:**
1. Use **"Factory Reset & Install"** option (red button)
2. This erases everything and flashes fresh firmware
3. Wait for complete finish before disconnecting

---

### Problem: "Browser not supported" message

**Cause:** Your browser doesn't support Web Serial API

**Solution:** Use Chrome 89+ or Edge 89+

Download:
- [Chrome](https://www.google.com/chrome/)
- [Edge](https://www.microsoft.com/edge)

---

### Problem: Permission denied (Linux)

**Cause:** User doesn't have permission to access serial ports

**Solution:**
```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER

# Log out and back in for changes to take effect
# Or temporarily:
sudo chmod 666 /dev/ttyUSB0  # Replace with your port
```

---

### Problem: Device creates WiFi but can't configure

**Possible Causes:**
- Browser cache issue
- Captive portal not opening

**Solutions:**
1. Manually go to `http://192.168.4.1` in browser
2. Clear browser cache
3. Try different device (phone instead of computer)
4. Use Incognito/Private mode
5. Disable VPN temporarily

---

## 🏗️ For Developers: Building Web Flasher Binaries

If you've modified the firmware and want to update the web flasher:

### Linux/macOS:
```bash
./scripts/build_web_flasher.sh
```

### Windows:
```batch
scripts\build_web_flasher.bat
```

This will:
1. Build the firmware
2. Extract all necessary binaries
3. Place them in `docs/firmware/` directory
4. Ready for deployment!

---

## 📤 Deploying Your Own Web Flasher

### Option 1: GitHub Pages (Recommended)

1. **Build the binaries:**
   ```bash
   ./scripts/build_web_flasher.sh
   ```

2. **Commit and push:**
   ```bash
   git add docs/
   git commit -m "Add web flasher with latest firmware"
   git push
   ```

3. **Enable GitHub Pages:**
   - Go to your repository on GitHub
   - Settings → Pages
   - Source: **Deploy from a branch**
   - Branch: **main** or **master**
   - Folder: **/docs**
   - Click Save

4. **Access your flasher:**
   - URL will be: `https://yourusername.github.io/DataTracker/flash.html`
   - Wait 1-2 minutes for deployment

### Option 2: Self-Hosted

1. **Build the binaries** (same as above)

2. **Serve the docs folder:**
   ```bash
   # Python 3
   cd docs
   python3 -m http.server 8000

   # Or use any web server (nginx, Apache, etc.)
   ```

3. **Access locally:**
   - `http://localhost:8000/flash.html`

---

## 🔒 Security Considerations

### Is it safe to use the web flasher?

**Yes!** The web flasher:
- Uses official **ESP Web Tools** by Espressif/Nabu Casa
- Runs entirely in your browser (no data sent to servers)
- Only communicates directly with your ESP32-C3 via USB
- Open source and auditable

### What data is transmitted?

**None!** All flashing happens locally:
- Firmware binaries are loaded from your browser
- Data flows: Browser → USB → ESP32-C3
- No internet connection required (after loading the page)

### Why does it need serial port access?

The Web Serial API allows the browser to communicate with your ESP32-C3 through the USB port. This is required to send the firmware to the device.

You grant permission on each connection - the browser can't access the port without your explicit approval.

---

## 📊 Technical Details

### Flash Memory Layout

The web flasher writes the following binaries:

| Binary | Offset | Size | Purpose |
|--------|--------|------|---------|
| bootloader.bin | 0x0000 | ~30KB | ESP32-C3 bootloader |
| partitions.bin | 0x8000 | <1KB | Partition table |
| boot_app0.bin | 0xe000 | <1KB | Boot application |
| firmware.bin | 0x10000 | ~600KB | Main application |

### Erase vs. No Erase

**Install Firmware** (green button):
- Writes firmware binaries only
- Preserves existing LittleFS filesystem (WiFi config, module data)
- Use for updating firmware without losing settings

**Factory Reset & Install** (red button):
- Erases entire flash memory first
- Then writes firmware binaries
- Fresh start, no old data
- Use when troubleshooting or starting fresh

---

## 🎓 Learn More

- **ESP Web Tools**: https://esphome.github.io/esp-web-tools/
- **Web Serial API**: https://developer.mozilla.org/en-US/docs/Web/API/Web_Serial_API
- **ESP32-C3 Datasheet**: https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf

---

## ❓ FAQ

### Can I use this on mobile?

Not yet. Web Serial API is not available on mobile browsers. You need a desktop/laptop with Chrome or Edge.

### Do I need internet to use the web flasher?

You need internet to load the web flasher page initially. After that, flashing works offline (browser has cached the firmware binaries).

### Can I flash multiple devices?

Yes! Just disconnect one device, connect the next, and flash again.

### Will this work with other ESP32 boards?

This web flasher is specifically for ESP32-C3. Other ESP32 variants (ESP32, ESP32-S2, ESP32-S3) require different bootloaders and partition tables.

### Can I customize the firmware before flashing?

The web flasher uses pre-built binaries. To customize:
1. Modify the source code
2. Build with PlatformIO
3. Run the build script to generate new binaries
4. Update your GitHub Pages deployment

---

## 💬 Need Help?

- **Troubleshooting Guide**: [TROUBLESHOOTING.md](../TROUBLESHOOTING.md)
- **GitHub Issues**: [Open an issue](https://github.com/yourusername/DataTracker/issues)
- **Documentation**: [Full documentation](../README.md)

---

**Happy Flashing! 🚀**
