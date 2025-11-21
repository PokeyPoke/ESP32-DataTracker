# Web Flasher Firmware Binaries

These binaries are used by the web-based flasher tool.

## Files

- **bootloader.bin** (offset 0x0000) - ESP32-C3 bootloader
- **partitions.bin** (offset 0x8000) - Partition table
- **boot_app0.bin** (offset 0xe000) - Boot application
- **firmware.bin** (offset 0x10000) - Main application firmware

## Flash Manually

If you want to flash manually using esptool:

```bash
esptool.py --chip esp32c3 --port /dev/ttyUSB0 --baud 460800 \
  --before default_reset --after hard_reset write_flash -z \
  --flash_mode dio --flash_freq 80m --flash_size 4MB \
  0x0 bootloader.bin \
  0x8000 partitions.bin \
  0xe000 boot_app0.bin \
  0x10000 firmware.bin
```

Replace `/dev/ttyUSB0` with your serial port (COM3 on Windows, etc.)

## Web Flasher

Instead of manual flashing, you can use the web-based flasher:
https://yourusername.github.io/DataTracker/flash.html

Just click and flash - no software installation required!
