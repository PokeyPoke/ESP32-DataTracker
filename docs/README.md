# ESP32-C3 Data Tracker - Web Resources

This directory contains web-based tools for the ESP32-C3 Data Tracker project.

## 🌐 Web Flasher

Flash firmware directly from your browser - no software installation required!

**[Open Web Flasher →](flash.html)**

### Features
- ⚡ Flash firmware in under 2 minutes
- 🔄 Factory reset option
- 🌐 No software installation needed
- 🔒 Secure - uses official ESP Web Tools
- ✅ Works in Chrome 89+ and Edge 89+

## 📚 Documentation

- **[Web Flasher Guide](WEB_FLASHER_GUIDE.md)** - Complete guide to using the web flasher
- **[Main Project Repository](https://github.com/yourusername/DataTracker)** - Source code and full documentation

## 📦 Firmware Binaries

Pre-built firmware binaries are available in the `firmware/` directory for manual flashing or web flashing.

## 🔧 For Developers

### Building Binaries

To build and update the web flasher binaries:

```bash
# Linux/macOS
./scripts/build_web_flasher.sh

# Windows
scripts\build_web_flasher.bat
```

### Testing Locally

```bash
cd docs
python3 -m http.server 8000
# Open: http://localhost:8000/flash.html
```

## ❓ Need Help?

- **Troubleshooting**: Check the [Troubleshooting Guide](../TROUBLESHOOTING.md)
- **Issues**: [Open an issue on GitHub](https://github.com/yourusername/DataTracker/issues)
- **Documentation**: [Full project documentation](../README.md)

---

**Built with ❤️ for makers and IoT enthusiasts**
