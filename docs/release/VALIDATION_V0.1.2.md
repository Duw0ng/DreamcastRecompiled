# DreamcastRecomp v0.1.2 Official — validation

v0.1.2 adds a Windows visual controller tester without changing SH-4/PVR/AICA execution semantics.

Validation targets:

- full CMake Release build;
- full CTest suite including generated-program build tests;
- package-level tester check ensuring launcher, XInput, DirectInput/WinMM, mapped-state, raw diagnostics and learning UI are present;
- archive integrity test after packaging.

The Linux validation container cannot physically connect a Windows DualShock 4 or display WinForms. Hardware acceptance must therefore be performed on Windows with `run_controller_test.bat`; the tool exposes both raw and mapped values specifically so that any remaining DS4 axis-layout difference can be identified unambiguously.

## Result

- Release build: **PASS (100%)**
- CTest suite: **54/54 PASS** (the final generated-program test was rerun independently after the first combined invocation reached the container call timeout)
- `controller_visual_tester_package`: **PASS**
- Existing generated runtime/controller mapping regressions: **PASS**

The tester itself is Windows-only because it uses WinForms, XInput and WinMM; physical input acceptance therefore remains a Windows hardware test.
