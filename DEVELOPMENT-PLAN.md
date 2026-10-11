# Development plan

## Completed for the public alpha

- ESP32-S3 hardware identification and application-only flash workflow.
- LCD and AXS5106L touch validation.
- Four-button UI and independent state models.
- Passive KNXnet/IP Routing reception and DPT 1.001 feedback.
- Multicast lifecycle recovery.
- Versioned web configuration and application-only persistence.

## Before ACTIVE mode

- Complete a dedicated security review of authorization and rate limiting.
- Validate unique individual and group addresses for each installation.
- Add explicit, non-persistent commissioning controls.
- Repeat packet-level and physical effect tests for every command path.
- Define recovery behavior for missing or contradictory feedback.

ACTIVE mode must not be enabled merely by changing persistent configuration.
