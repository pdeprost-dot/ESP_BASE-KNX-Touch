# Changelog

All notable changes are documented here. This project follows Semantic
Versioning and is currently pre-release software.

## [Unreleased]

## [0.2.0-alpha] - 2026-10-12

- Added persistent Web-selectable ACTIVE/PASSIVE operation, command-group
  whitelisting, and multicast/socket gating.
- Enabled successive touch-triggered DPT 1.001 ON/OFF commands on all four
  configured buttons, without automatic retry or GroupValueRead.
- Kept KNX status feedback authoritative, including fan-out to buttons sharing
  a status group and PENDING timeout to UNKNOWN.
- Restored the selected ACTIVE mode safely after restart or Wi-Fi reconnection,
  without sending a command automatically.
- Compact the KNX web form, highlight enabled buttons and invalid fields, and
  preserve submitted values after validation errors.
- Allow command groups, status groups, and complete command/status pairs to be
  shared across buttons, with fan-out of status updates to all active matches.
- Added schema 2 general KNX settings and schema 1 migration.
- Added configurable individual address, Routing multicast, port, optional
  router address, and feedback timeout.
- Kept Routing and passive synchronization mandatory.

## [0.1.0-alpha] - 2026-10-11

- Validated the Waveshare ESP32-S3-Touch-LCD-1.47 LCD and AXS5106L touch input.
- Added a four-button touch interface and independent state models.
- Added passive KNXnet/IP Routing reception and DPT 1.001 status decoding.
- Added guarded cEMI encoding primitives developed through controlled tests.
- Added ESP_BASE 2.4.1 web configuration and versioned application persistence.
- Added multicast lifecycle recovery and diagnostic counters.

[Unreleased]: https://github.com/pdeprost-dot/ESP_BASE-KNX-Touch/compare/v0.2.0-alpha...HEAD
[0.2.0-alpha]: https://github.com/pdeprost-dot/ESP_BASE-KNX-Touch/compare/v0.1.0-alpha...v0.2.0-alpha
[0.1.0-alpha]: https://github.com/pdeprost-dot/ESP_BASE-KNX-Touch/releases/tag/v0.1.0-alpha
