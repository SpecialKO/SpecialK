# AGENT_RULES.md

## Scope Constraints

- Do not modify injector/bootstrap logic.
- Do not refactor logging unless explicitly instructed.
- Do not modify unrelated subsystems.
- No speculative refactors.
- No placeholder implementations.
- One task per PR.
- Minimal diffs only.
- Must compile cleanly in Visual Studio solution.

## API Focus

Phase 1:
- D3D11 / DXGI only.

GL path:
- Disabled for Phase 1.
- No GL mapping creation.
- No GL composite logic changes.

## Mapping Rules

- Mapping name: Local\\SidecarK_Frame_v1_<target_pid>
- Consumer never creates mapping.
- Producer never creates mapping.
