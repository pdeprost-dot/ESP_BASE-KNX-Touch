# Changelog

All notable changes are documented here. This project follows Semantic
Versioning and is currently pre-release software.

## [Unreleased]

- Added schema 2 general KNX settings and schema 1 migration.
- Added configurable individual address, Routing multicast, port, optional
  router address, and feedback timeout.
- Kept Routing, passive synchronization, and PASSIVE operation mandatory.

## [0.1.0-alpha] - pending publication

- Validated the Waveshare ESP32-S3-Touch-LCD-1.47 LCD and AXS5106L touch input.
- Added a four-button touch interface and independent state models.
- Added passive KNXnet/IP Routing reception and DPT 1.001 status decoding.
- Added guarded cEMI encoding primitives developed through controlled tests.
- Added ESP_BASE 2.4.1 web configuration and versioned application persistence.
- Added multicast lifecycle recovery and diagnostic counters.

[Unreleased]: https://github.com/pdeprost-dot/ESP_BASE-KNX-Touch/compare/v0.1.0-alpha...HEAD
[0.1.0-alpha]: https://github.com/pdeprost-dot/ESP_BASE-KNX-Touch/releases/tag/v0.1.0-alpha
