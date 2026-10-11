# Building and flashing

## Requirements

- PlatformIO Core or PlatformIO IDE
- USB data connection to a Waveshare ESP32-S3-Touch-LCD-1.47
- ESP_BASE 2.4.1 (resolved by PlatformIO)

## Build

```sh
pio run -e waveshare_esp32_s3_touch_lcd_1_47
```

The project selects an ESP32-S3 target with 16 MB flash and 8 MB PSRAM. A
successful build produces `firmware.bin` under the matching `.pio/build`
directory. Building does not flash the board.

## Safe application-only flash

1. Back up the complete factory flash before the first write.
2. Read and inspect the installed partition table.
3. Confirm the application partition offset and capacity.
4. Select the serial port explicitly; do not commit it to `platformio.ini`.
5. Write only `firmware.bin` at the confirmed application offset.
6. Verify the written range against the local image.

Do not erase the full flash. Do not write bootloader, partition-table, NVS, or
PHY partitions merely to update this application. Exact `esptool` parameters
depend on the installed partition table and local serial port, so this project
does not provide a copy-paste command with assumed device-specific values.

## Serial verification

Use 115200 baud. A safe boot reports LCD and touch readiness, Routing decoder
and encoder self-tests as PASS, and KNX transmission as BLOCKED.
