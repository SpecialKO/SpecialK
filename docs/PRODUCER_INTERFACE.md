# PRODUCER_INTERFACE.md

## Responsibilities

Producer writes frames into mapping created by Host.

Producer must:
- Open mapping using canonical name.
- Validate SKF1 header.
- Write pixel data before incrementing frame_counter.
- Increment frame_counter last.

Producer must NOT:
- Create mapping.
- Recreate mapping.
- Modify header fields after initialization.

## Failure Behavior

If mapping does not exist:
- Fail fast.
- Do not create mapping.
