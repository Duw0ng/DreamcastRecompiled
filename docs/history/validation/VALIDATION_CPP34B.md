# Validation — cpp34b MODULAR_FULL_ISP_ABI1

Date: 2026-10-03

- Full CMake/Ninja project build: PASS.
- Core CTest suite: 54/54 PASS (the long run reached 53/54 before tool timeout; `generated_program_build` was rerun alone and passed).
- Generated modular fixture: configured and built successfully with Guest ABI v1.
- Network-only rebuild after touching only `runtime/plugins/dcr_network.cpp`: exactly 2 build steps, plugin object + shared library; generated SH-4 compile count 0.
- Persistent flash migration checks the complete PlanetWeb/DreamKey/Katana layout and reseeds only partition 2 when upgrading, preserving factory/system data and console ID.
