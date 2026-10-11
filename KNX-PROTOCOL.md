# KNXnet/IP protocol notes

## Supported public mode

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
- intended future command accepted by the transport: PENDING;
- feedback timeout: UNKNOWN, with no retry;
- foreign destination: ignored by the button models.

No `GroupValueRead` is used for initial synchronization.

## Transmission status

The repository includes tested cEMI encoding primitives for future controlled
work. Routing injection uses `L_Data.ind` (`0x29`), while Tunneling encoding is
kept separate. In the alpha application, no UI path calls these functions and
all authorization flags are false. ACTIVE mode and Tunneling selection are not
exposed.

## Address validation

- individual address: `0..15.0..15.1..255`;
- three-level group address: `0..31/0..7/0..255`, excluding `0/0/0`;
- UDP port: 1 through 65535;
- multicast address: IPv4 class-D range (`224.0.0.0/4`);
- feedback timeout: 1000 through 60000 ms.

Command and status addresses must differ, and duplicate command or status
groups across buttons are rejected.
