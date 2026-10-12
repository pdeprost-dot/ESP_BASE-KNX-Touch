# v0.2.0-alpha release notes

## Summary

Operational alpha of the four-button KNX/IP touch panel based on ESP_BASE
2.4.1. It adds persistent ACTIVE/PASSIVE selection and validated successive
ON/OFF control while retaining authoritative KNX status feedback.

## Compatible hardware

- Waveshare ESP32-S3-Touch-LCD-1.47
- ESP32-S3R8 with 16 MB flash and 8 MB PSRAM
- JD9853 LCD and AXS5106L touch controller

## Installation

Build locally with PlatformIO. Inspect the installed partition table and flash
only the generated application image at the confirmed application offset. Keep
bootloader, partition table, NVS, and PHY data unchanged. Configure Wi-Fi
through ESP_BASE, then review the KNX settings on **KNX / Touches**.

No precompiled firmware asset is included in this alpha.

## Highlights

- Four independently enabled and configured touch buttons.
- Persistent Web-selectable ACTIVE mode with immediate PASSIVE fallback.
- DPT 1.001 ON/OFF writes over KNXnet/IP Routing.
- UNKNOWN, ON, OFF, and PENDING states confirmed only by configured status
  telegrams.
- Shared status groups update every matching enabled button.
- Safe restoration after restart and Wi-Fi reconnection without automatic
  commands or retries.

## Important limitations

- Tunneling is not selectable.
- `GroupValueRead` is not emitted; initial synchronization is passive.
- This is not an ETS product database and cannot program KNX participants.
- Multicast delivery depends on the local network's multicast/IGMP behavior.

Review and assign a unique KNX individual address and correct command/status
groups before enabling ACTIVE mode.
