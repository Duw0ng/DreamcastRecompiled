# Validation — 0.0.158

- Root cause: 0.0.157 function-wide GPR cache omitted preload for write-only destinations, but generated functions support entry at any basic block. A later-block entry could therefore flush a zero-initialized local into architectural state.
- Fix: every potentially written cached GPR/special register is preloaded on entry/reload.
- Added multi-entry write-only regression probe.
- Linux Release host build: PASS.
- CTest: 50/50 PASS before final version bump; repeated after bump.
- Existing generated-code tests and TA/PVR tests retained.
- Immediate rollback remains 0.0.156; stable rollback remains 0.0.131.
