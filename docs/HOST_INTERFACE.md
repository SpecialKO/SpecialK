# HOST_INTERFACE.md

## Responsibilities

Host is sole mapping owner.

Host must:
- Create mapping: Local\\SidecarK_Frame_v1_<target_pid>
- Use CreateFileMappingW with PAGE_READWRITE.
- Bind mapping to injected target PID.
- Keep mapping alive for duration of injection session.

Host must NOT:
- Modify frame protocol.
- Write pixel data.
- Recreate mapping due to frame staleness.
