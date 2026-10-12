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
- `KnxRuntimeControl`: PASSIVE/ACTIVE policy gate. It requires an
  enabled button, an operational multicast socket, and a non-PENDING model.
- `main.cpp`: composition, persistent operating-mode restoration, and policy.

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
OFF. The status dispatcher fans one telegram out to every active matching
button, including buttons that share the same status address, while keeping
their models independent. Disabled buttons and foreign groups are ignored. A
network generation change invalidates every confirmation to UNKNOWN. PENDING
begins only after an authorized transport accepts a command and expires to
UNKNOWN after the configured timeout.

## Safety boundary

Configuration and command intent remain separate from transport authorization.
PASSIVE blocks application writes. ACTIVE authorizes only command groups of
enabled buttons while the multicast socket is operational; GroupValueRead is
never authorized. The selected mode is persisted in the application namespace.
After boot or network regeneration, ACTIVE is restored only after the socket is
operational and never emits a command by itself. A successful UDP send changes
only the originating model to PENDING; confirmation still comes exclusively
from its configured status group.
