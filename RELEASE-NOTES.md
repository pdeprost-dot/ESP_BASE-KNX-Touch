# v0.1.0-alpha release notes (draft)

## Summary

First public alpha of a four-button KNX/IP touch panel based on ESP_BASE 2.4.1.
It combines a validated LCD/touch interface, passive KNXnet/IP Routing status
reception, and persistent web configuration.

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

## Important limitations

- Permanent ACTIVE command mode is not enabled or validated.
- The public firmware is receive-only and keeps application transmission
  blocked.
- Tunneling is not selectable.
- This is not an ETS product database and cannot program KNX participants.
- Multicast delivery depends on the local network's multicast/IGMP behavior.

Review and assign a unique KNX individual address and group addresses before
any future authorized ACTIVE-mode testing.
