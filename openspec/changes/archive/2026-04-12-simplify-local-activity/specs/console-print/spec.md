## REMOVED Requirements

### Requirement: Plugin can print file-backed lines on Skyrim VR without executing them
**Reason**: Print is removed from the product contract.
**Migration**: Remove any workflow that writes plugin-authored lines to an outbox file.

### Requirement: Print starts only when print prerequisites are available
**Reason**: No print runtime service remains.
**Migration**: Remove print-specific startup expectations and configuration.

### Requirement: Print state resets cleanly when the outbox file changes identity
**Reason**: Outbox tracking and queueing are removed together with print.
**Migration**: Delete outbox files and any tooling that rotates or truncates them.

### Requirement: Print stops when it cannot continue safely
**Reason**: No print execution path remains.
**Migration**: Remove monitoring and tests that expect outbox print-path failures or recovery behavior.
