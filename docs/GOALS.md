# GOALS.md

## Project Identity

SidecarK is a SpecialK fork consisting of:
- A host component (injection + lifecycle management)
- An injected rendering component (DXGI Present path)

Phase 1 concerns the injected D3D11/DXGI render path only.

OpenGL (GL) path is explicitly disabled for Phase 1.

## Architectural Lock-In

1. Host is the sole mapping owner.
2. Consumer (injected SidecarK) must never create or recreate the shared memory mapping.
3. Producer must fail fast if mapping is absent.
4. Injector/bootstrap logic is infrastructure and must not be refactored.
5. One PR per task. Minimal diffs only.

## Phase 1 Visual Milestone

Working =

- Producer writes static BGRA test pattern.
- SidecarK reads it from shared memory.
- Overlay is visibly composited in DXGI Present.
- Control plane toggles overlay visibility deterministically.
- No flicker.
- No mapping recreation churn.
- No injector crash.
