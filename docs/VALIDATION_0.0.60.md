# DreamcastRecomp 0.0.60 validation

- 47/47 CTest regressions pass.
- ChuChu Rocket commercial closure: 2,294 functions, 171,568 known SH-4
  instructions, 0 unknown, `RAW_SH4=0`.
- Generated commercial C++ compiles and links with Clang 17 Debug.
- Portable commercial run with the supplied CDI creates/loads A1 and reaches:
  `vmu=A1/1/1/1/0/0`.
- Same run reports real descriptor telemetry such as `maple-desc=27/15/0`
  instead of treating NOP descriptors as fake transfer frames.
- Early enumeration reports `maple-recv-fix=0`.
- No PVR/AICA/timing/rasterizer changes in this release.
