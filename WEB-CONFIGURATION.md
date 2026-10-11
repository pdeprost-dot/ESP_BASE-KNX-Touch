# Web configuration

Open the ESP_BASE web interface and select **KNX / Touches**.

## General KNX settings

- **Individual address:** must be unique in the KNX installation and validated
  before any future ACTIVE-mode test.
- **Transport:** Routing only in this release.
- **Router address:** optional information in Routing; reserved for future
  Tunneling support.
- **Port:** defaults to 3671.
- **Routing multicast:** defaults to the KNX standard `224.0.23.12`.
- **Feedback timeout:** defaults to 5000 ms.
- **Synchronization:** passive; the ESP sends no `GroupValueRead`.
- **Operating mode:** PASSIVE only.

Network-related changes are loaded on restart.

## Button settings

Each of four buttons has a display name, command group address, status group
address, and enabled flag. Addresses use KNX three-level notation. Duplicate
command groups, duplicate status groups, identical command/status pairs, and
out-of-range values are rejected.

Enabling a button does not enable KNX transmission in this alpha release. The
application and transport authorization guards remain blocked independently of
persistent configuration.

## Persistence

Settings are stored in the application-only `knx-touch` Preferences namespace.
Unchanged submissions do not write flash. Invalid or corrupt data is rejected;
safe generic defaults keep all buttons disabled. ESP_BASE Wi-Fi configuration
uses a separate namespace.
