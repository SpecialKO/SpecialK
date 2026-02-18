# INTEGRATION_CONTRACT.md

## Canonical Mapping Name

Local\\SidecarK_Frame_v1_<target_pid>

<target_pid> = injected target process PID (swapchain owner).
Not host PID. Not producer PID.

Mapping name is fixed and versioned.
Any change requires explicit version bump.

## Mapping Ownership

Host:
- Creates mapping once per injection session.
- Binds mapping to injected PID.
- Keeps mapping alive for the duration of the injection session.
- May recreate mapping ONLY when starting a new injection session (new target PID).

Host must NOT recreate mapping based on frame staleness.

SidecarK (consumer):
- Opens mapping only.
- Never creates mapping.
- Never recreates mapping.

Producer:
- Opens mapping only.
- Fails fast if mapping missing.

## Frame Format (SKF1 v1)

Header layout (little-endian):

Offset | Field
-------|------
0x00   | 'SKF1' (uint32)
0x04   | version = 1
0x08   | header_bytes = 0x20
0x0C   | data_offset = 0x24
0x10   | pixel_format = 1 (BGRA8)
0x14   | width
0x18   | height
0x1C   | stride (bytes per row)
0x20   | frame_counter (uint32, volatile)
0x24   | pixel data begins

Pixel format: BGRA8 (little-endian).
Stride must be >= width * 4.

## Producer Write Rules

1. Write pixel data first.
2. Issue memory barrier if applicable.
3. Increment frame_counter last (release semantics).

## Consumer Read Rules (Stable Read)

To avoid torn frames:

1. Read frame_counter (c1).
2. Copy pixel data into GPU upload buffer.
3. Read frame_counter again (c2).
4. Accept frame only if c1 == c2.
   Otherwise discard and retry next Present.

Consumer retains last valid frame if no new stable frame is available.

## Staleness Behavior

If producer stops updating:
- SidecarK retains last valid frame.
- SidecarK does NOT recreate mapping.
- SidecarK does NOT blank frame automatically.
