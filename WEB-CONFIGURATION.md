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
- **Operating mode:** PASSIVE or ACTIVE, selected explicitly from the page.

Network-related changes are loaded on restart.

## Button settings

Each of four buttons has a display name, command group address, status group
address, and enabled flag. Addresses use KNX three-level notation. Multiple
buttons may share a command group, a status group, or the same command/status
pair. A command address must still differ from the status address on each
button, and malformed or out-of-range values are rejected.

When active buttons share a status group, one valid status telegram updates
all of their independent state models. Disabled buttons do not subscribe to
status updates.

Enabling a button and enabling KNX transmission remain separate actions.
ACTIVE requires a deliberate Web action and is stored; it is restored after
restart only when multicast is operational. The page provides an immediate
return to persistent PASSIVE. Activation is refused unless multicast is
operational and at least one button is enabled.

## Persistence

Settings are stored in the application-only `knx-touch` Preferences namespace.
Unchanged submissions do not write flash. Invalid or corrupt data is rejected;
safe generic defaults keep all buttons disabled. ESP_BASE Wi-Fi configuration
uses a separate namespace.
