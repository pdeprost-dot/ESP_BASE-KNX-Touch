# ESP_BASE-KNX-Touch

Experimental four-button KNX/IP touch panel for the Waveshare
ESP32-S3-Touch-LCD-1.47, built on [ESP_BASE](https://github.com/pdeprost-dot/ESP_BASE)
2.4.1.

> **Alpha safety notice:** permanent KNX command mode is not enabled or
> validated. The public build is receive-only: application transmission,
> `GroupValueWrite`, and `GroupValueRead` are blocked.

## Features

- JD9853 LCD support at 320 x 172 in landscape orientation.
- AXS5106L capacitive touch support with press, release, long-press, and
  slide-cancellation handling.
- Four independently configured buttons with UNKNOWN, OFF, ON, and PENDING
  states.
- Passive KNXnet/IP Routing reception on the standard multicast group.
- DPT 1.001 status decoding for configured group addresses.
- ESP_BASE web page for general KNX settings and the four buttons.
- Versioned application configuration in a dedicated `knx-touch` Preferences
  namespace, including migration from schema 1 to schema 2.
- Multicast lifecycle recovery after verified Wi-Fi/IP changes.

## Supported hardware

- Waveshare ESP32-S3-Touch-LCD-1.47
- ESP32-S3R8, 16 MB flash, 8 MB PSRAM
- JD9853 LCD, 172 x 320
- AXS5106L touch controller at I2C address `0x63`

See [HARDWARE.md](HARDWARE.md) for the verified wiring.

## Build and flash

Install [PlatformIO Core](https://docs.platformio.org/) and run:

```sh
pio run
```

The default environment is `waveshare_esp32_s3_touch_lcd_1_47`. Select the
serial port explicitly for your machine. To preserve the existing bootloader,
partition table, and NVS, write only `.pio/build/waveshare_esp32_s3_touch_lcd_1_47/firmware.bin`
at the application offset declared by the installed partition table. Do not
assume `0x10000` on a board whose partition table has not been inspected.

No precompiled firmware is published because device and installation settings
must be reviewed locally. See [BUILDING.md](BUILDING.md).

## Web configuration

ESP_BASE exposes **KNX / Touches** in its navigation. The page configures:

- individual KNX address;
- Routing multicast address and UDP port;
- optional KNX/IP router address for future use;
- feedback timeout;
- name, command group, status group, and enabled flag for each button.

Only Routing, passive synchronization, and PASSIVE operating mode are
available. Network-related changes take effect after restart. All four buttons
are disabled in a fresh configuration. See
[WEB-CONFIGURATION.md](WEB-CONFIGURATION.md).

## KNX behavior

The firmware joins a configured KNXnet/IP Routing multicast group and accepts
valid `ROUTING_INDICATION` cEMI telegrams. A button changes to ON or OFF only
after a DPT 1.001 write or response is received on its configured status group.
Foreign group addresses do not modify button state.

At startup and after loss of network synchronization, state is UNKNOWN. The
code contains guarded transmission primitives developed during controlled
testing, but the application never calls them in this release and all transport
authorization flags remain false. See [KNX-PROTOCOL.md](KNX-PROTOCOL.md).

## Known limitations

- Permanent ACTIVE mode is neither exposed nor validated.
- Tunneling is retained as an internal experimental transport but is not
  selectable in the web UI.
- UDP multicast delivery has no end-to-end acknowledgement.
- Configuration schema migration is compiled and tested offline; schema 2 has
  not yet been flashed as part of Phase 7A.
- The application is not an ETS product database and does not program KNX
  participants.

## Dependencies and licensing

Project code is licensed under MIT. Runtime dependencies are fetched by
PlatformIO and are not vendored:

- ESP_BASE 2.4.1 — MIT
- PubSubClient 2.8 — MIT
- Arduino_GFX 1.5.9 — BSD-compatible license
- Arduino-ESP32 framework — upstream component licenses apply

See [LICENSE](LICENSE) and [CHANGELOG.md](CHANGELOG.md).
