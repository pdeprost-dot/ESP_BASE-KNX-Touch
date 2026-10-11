# Public validation summary

## Hardware

- Target identification, 16 MB flash, and 8 MB PSRAM confirmed.
- JD9853 LCD initialized at 320 x 172 landscape.
- AXS5106L detected at `0x63`; reset, interrupt, coordinates, release, and
  repeatability validated.
- Four independent touch zones validated, including long press, debounce, and
  slide cancellation.

## Network and KNX

- ESP_BASE 2.4.1 Wi-Fi and web services remain responsive with the UI running.
- KNXnet/IP Routing multicast membership and passive reception validated.
- `ROUTING_INDICATION` and cEMI group telegram decoding validated.
- DPT 1.001 ON/OFF feedback updates only the matching configured button.
- Network loss/rejoin lifecycle and UNKNOWN invalidation covered by offline
  tests; recovery after a real Wi-Fi connection transition was observed.
- Controlled one-shot transmission was validated during private development,
  but permanent ACTIVE mode remains blocked and is not part of this release.

## Web and persistence

- Four-button form, general KNX form, validation errors, independent enable
  flags, refresh, and schema 1 persistence were validated on hardware.
- Schema 2 parsing, validation, checksum rejection, and schema 1 migration are
  covered by offline self-tests and compile successfully.
- ESP_BASE configuration and the application `knx-touch` namespace remain
  separate.

## Current build

PlatformIO compilation for `waveshare_esp32_s3_touch_lcd_1_47` succeeds. The
Phase 7A build uses 53,960 bytes of RAM and 907,489 bytes of application flash.
Phase 7A was not flashed.

Private packet captures, serial logs, device identifiers, SSIDs, installation
addresses, and local IP addresses are intentionally excluded from the public
repository state.
