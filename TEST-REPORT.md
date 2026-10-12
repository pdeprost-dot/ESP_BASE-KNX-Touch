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

## Shared group-address validation (post v0.1.0-alpha)

- Offline configuration tests accept two or four buttons with an identical
  command/status pair and validate an in-memory persistence round trip.
- Dispatch tests confirm that a shared status updates every active matching
  model, while a disabled matching button remains UNKNOWN.
- A shared command with different status groups does not couple the models.
- Existing parser tests continue to reject malformed, out-of-range, trailing,
  and zero group addresses.
- ESP32-S3 PlatformIO compilation passed with 53,960 bytes RAM and 907,573
  bytes application flash.
- KNX transmission remains blocked; this change was not flashed or tested on
  the KNX installation.

## Phase 7B web-interface checkpoint

- Removed the duplicate page heading and compacted general/button fields into
  responsive grids.
- Enabled and disabled buttons have distinct visual treatment; checkboxes use
  explicit flex alignment.
- Validation identifies and highlights the failing field and re-renders every
  submitted value, including invalid text and checkbox state.
- Shared group addresses remain accepted and invalid KNX syntax remains
  rejected by the existing parser.
- ESP32-S3 compilation passed with 53,960 bytes RAM and 909,233 bytes
  application flash. Transmission remains blocked in this checkpoint.

## Phase 7B passive hardware validation

- The passive checkpoint was flashed only at application offset `0x10000`;
  esptool verified the written data hash.
- Existing NVS configuration reloaded after restart; LCD, touch, ESP_BASE Wi-Fi
  and multicast membership (`join=1/1`) initialized successfully.
- Two enabled buttons sharing command `2/0/0` and status `2/1/0` both changed
  to ON and then OFF from single real status telegrams.
- The ESP remained `outbound_application=blocked` throughout validation.

## Phase 7B ACTIVE offline checkpoint

- Runtime assertions cover PASSIVE boot, explicit ACTIVE, disabled buttons,
  PENDING, unavailable network/socket, and immediate PASSIVE return.
- Existing state, timeout, anti-retrigger, shared-address and cEMI tests remain
  compiled into the build.
- Configuration and multicast-generation changes force PASSIVE; no automatic
  send or GroupValueRead exists.
- ESP32-S3 compilation passed with 53,984 bytes RAM and 912,161 bytes flash.
  This ACTIVE-capable binary has not been flashed.

## Phase 7B first controlled ACTIVE test

- Commit `5b5c361` was rebuilt and flashed only to application offset
  `0x10000`; esptool verified the written image digest. Bootloader, partition
  table and NVS were preserved.
- Boot was PASSIVE, encoder/decoder self-tests passed, and multicast joined
  successfully with no socket error. Salon alone was enabled with source
  `1.1.250`, command `2/0/0`, status `2/1/0`, DPT 1.001.
- A passive Salon press was logged as `blocked=PASSIVE` and emitted nothing.
  A real OFF status on `2/1/0` established the initial confirmed state.
- After explicit authorization, the Web action enabled ACTIVE with one
  whitelisted command group. One Salon gesture produced exactly one
  `GroupValueWrite ON` to `2/0/0`.
- Salon entered PENDING, received `GroupValueWrite ON` on `2/1/0`, and changed
  to confirmed ON. Physical illumination was confirmed by the user.
- The application returned immediately to PASSIVE and logged the transition.
  No GroupValueRead, retry, second command, or OFF command was emitted.
- The stale general-section wording was corrected after the test; that text-only
  follow-up was compiled but not flashed.

## Phase 7B controlled OFF test

- Commit `9f91946` was rebuilt and flashed only at application offset
  `0x10000`. Image SHA-256 was
  `1B542BDBB15AA08F0EE3D5F5A0BD7BAB0809A64D3BDD23E87AC3466F44D77C5C`,
  and esptool verified the written data.
- Boot was PASSIVE and encoder/decoder self-tests passed. After one initial
  Wi-Fi timeout, multicast recovered (`join=2/2`) with no socket error.
- A real `2/1/0 = ON` status established the required confirmed ON state.
  Salon alone was enabled; ACTIVE was explicitly enabled with only `2/0/0`
  whitelisted.
- One touch gesture produced one logged `GroupValueWrite OFF` from `1.1.250`
  to `2/0/0`, followed by PENDING. No status arrived on `2/1/0` within five
  seconds, so the model correctly returned to UNKNOWN without retry.
- The user confirmed that the light did not switch off. No late status was
  observed. The application returned immediately to PASSIVE; no second write
  and no GroupValueRead were emitted.
- The local TX log proves that the UDP send call completed, but does not prove
  that the routing indication reached the Siemens router or TP1. A passive
  Wireshark/bus-monitor comparison is required before any new controlled test.
