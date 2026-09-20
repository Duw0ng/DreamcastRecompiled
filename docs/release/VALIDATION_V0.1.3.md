# DreamcastRecomp v0.1 Official — validation

Scope: native PS4/DirectInput trigger mapping and controller tester calibration.

Expected checks:
- core regression suite PASS;
- generated runtime contains zero-cross-talk native PS4 logical trigger fallback;
- generated runtime understands `range:<axis>:<rest>:<full>` trigger bindings;
- controller visual tester package includes L2/R2 learning steps and range calibration;
- PowerShell tester source remains ASCII-only for Windows PowerShell 5.1 compatibility.

## Result

- Linux Release build: **PASS (100%)**.
- CTest regression: **54/54 PASS** (the heavy generated program test was rerun independently after the first aggregate invocation reached the execution time limit).
- `controller_visual_tester_package`: PASS.
- PowerShell tester: ASCII-only and delimiter/quote balance check PASS.
- Physical Windows DualShock 4 acceptance remains the user-side confirmation step.
