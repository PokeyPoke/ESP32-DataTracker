# ESP32-DataTracker Enhanced Firmware v1.0

**Branch:** `feature/code-quality-and-enhancements`
**Build Date:** November 25, 2025
**Base Version:** 3.3.0-alpha

## What's New

This enhanced firmware includes significant code quality improvements and four major new features:

### New Features
- ✅ **OTA Updates** - Update firmware remotely via web interface
- ✅ **Alert/Threshold System** - Configure alerts for module values
- ✅ **Screensaver** - Automatic display dimming after 5 minutes
- ✅ **Data Export** - Export config and data as JSON/CSV

### Code Quality Improvements
- ✅ Centralized constants file with documentation
- ✅ Removed debug code from production builds
- ✅ Conditional compilation for debug features
- ✅ Better code organization

## Files

- `bootloader.bin` - ESP32-C3 bootloader (13 KB)
- `partitions.bin` - Partition table (3 KB)
- `firmware.bin` - Main application firmware (1.4 MB)

## Flash Instructions

### Using Web Flasher

1. Visit the web flasher: [flash-v2.html](../../flash-v2.html)
2. Select these files:
   - Bootloader at 0x0
   - Partitions at 0x8000
   - Firmware at 0x10000
3. Click "Flash"

### Using esptool

```bash
esptool.py --chip esp32c3 --baud 921600 write_flash \
  0x0 bootloader.bin \
  0x8000 partitions.bin \
  0x10000 firmware.bin
```

### Using PlatformIO

```bash
pio run -t upload
```

## New API Endpoints

```
GET  /api/ota/status          - Get firmware version info
POST /api/ota/upload          - Upload new firmware
GET  /api/alerts              - List all alerts
POST /api/alerts              - Create new alert
DELETE /api/alerts?id=X       - Delete alert
POST /api/alerts/toggle       - Enable/disable alert
GET  /api/export/config       - Export configuration
GET  /api/export/data         - Export module data
```

## Compatibility

- ✅ **Backward Compatible** - Existing configurations will work
- ✅ **No Breaking Changes** - All original features preserved
- ✅ **Same Hardware** - ESP32-C3, SH1106 OLED display

## Upgrade Notes

1. Backup your current configuration via web interface
2. Flash the new firmware
3. Device will reboot and resume normal operation
4. All settings and modules preserved

## Known Issues

- Alert system currently reads from config only (simplified implementation)
- BUTTON_PIN redefinition warning (harmless, can be ignored)

## Support

For issues or questions:
- GitHub: https://github.com/PokeyPoke/ESP32-DataTracker/issues
- Branch: feature/code-quality-and-enhancements

## Changelog

See [IMPROVEMENTS_SUMMARY.md](../../../IMPROVEMENTS_SUMMARY.md) for detailed changes.
