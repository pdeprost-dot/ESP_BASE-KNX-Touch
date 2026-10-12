# KNXnet/IP protocol notes

## Supported public transport

This alpha exposes KNXnet/IP Routing reception only. It joins the configurable
IPv4 multicast address (standard default `224.0.23.12`) on UDP port 3671 by
default and validates KNXnet/IP header length, protocol version, service type,
total length, and cEMI bounds.

`ROUTING_INDICATION` (`0x0530`) frames containing group-addressed cEMI data are
decoded into source individual address, destination group address, APCI, and an
inline DPT 1 value when present. DPT 1.001 interpretation is limited to status
groups explicitly configured by the user.

## State policy

- startup or network resynchronization: UNKNOWN;
- valid status write/response: confirmed ON or OFF;
- touch command accepted by the transport: PENDING;
- feedback timeout: UNKNOWN, with no retry;
- foreign destination: ignored by the button models.

A valid status write/response is dispatched to every active button configured
for its destination group. Sharing an address does not merge the button state
models; each button continues to own its UNKNOWN/OFF/ON/PENDING state.

No `GroupValueRead` is used for initial synchronization.

## Transmission

Routing injection uses `L_Data.ind` (`0x29`), while Tunneling encoding remains
separate and is not selectable. PASSIVE blocks writes. ACTIVE permits one
GroupValueWrite per valid touch to the configured command group of an enabled
button.

## Address validation

## v0.2.0-alpha ACTIVE policy

ACTIVE is explicit and persistent. Routing writes are accepted only while Wi-Fi,
multicast membership and the UDP socket are operational, and only for command
groups of enabled buttons. There is no startup/reconnection send,
GroupValueRead, or timeout retry. ACTIVE restoration after boot or reconnection
only restores authorization; it sends no telegram. Shared command groups remain
one-send-per-touch; status fan-out is a separate receive action.

- individual address: `0..15.0..15.1..255`;
- three-level group address: `0..31/0..7/0..255`, excluding `0/0/0`;
- UDP port: 1 through 65535;
- multicast address: IPv4 class-D range (`224.0.0.0/4`);
- feedback timeout: 1000 through 60000 ms.

Command and status addresses must differ within one button. Command groups,
status groups, and complete command/status pairs may be shared across buttons.
