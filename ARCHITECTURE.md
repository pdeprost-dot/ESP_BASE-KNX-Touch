# Architecture

## Components

- `LcdDriver`: JD9853 initialization and primitive rendering.
- `TouchDriver`: AXS5106L reset, I2C reads, and coordinate conversion.
- `FourButtonUi`: rendering, hit testing, press/release handling, and gesture
  capture. It does not know KNX protocol details.
- `SalonKnxModel`: reusable per-button UNKNOWN/OFF/ON/PENDING state machine.
- `KnxTouchConfigStore`: schema validation, migration, checksum, and persistent
  application storage.
- `KnxTouchWeb`: ESP_BASE public-route integration and HTML validation.
- `KNXTransport`: non-blocking Tunneling/ Routing transport internals, cEMI
  decoding, multicast lifecycle, counters, and transmission guards.
- `main.cpp`: composition and policy. The current policy is PASSIVE.

## Configuration

Schema 2 contains a general KNX block followed by four independent button
blocks. The complete binary record has a magic value, schema version, size, and
checksum. Schema 1 records are validated and migrated in RAM; they are not
rewritten until a user explicitly saves the web form.

Application settings use the `knx-touch` Preferences namespace. ESP_BASE owns
its separate system configuration. Fresh defaults are generic, have no router
IP, and keep every button disabled.

## State and synchronization

Each button owns its state. Only its configured status group can confirm ON or
OFF. A network generation change invalidates every confirmation to UNKNOWN.
PENDING begins only after an authorized transport accepts a command and expires
to UNKNOWN after the configured timeout.

## Safety boundary

Configuration and command intent are separate from authorization. The alpha
application constant is false, `applicationTransmitEnabled` is false, and both
read/write authorization flags are false. The web schema cannot change these
guards. No automatic command is emitted at boot or after reconnection.
