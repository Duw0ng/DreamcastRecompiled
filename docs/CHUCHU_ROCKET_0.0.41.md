# ChuChu Rocket! commercial baseline — 0.0.41

The supplied native Dreamcast/Katana CDI remains the project's first commercial target. No game data is included in release archives.

0.0.40.4 proved that the disc can be recognized and that a large raw control-flow graph can be discovered. 0.0.41 takes the next step: it builds a synthetic executable model from 1ST_READ.BIN, discovers function candidates without symbols, lowers the reachable closure through the existing CFG/DCIR pipeline, emits C++, and builds that generated project.

A representative local run produced 727 reachable functions, 50,752 known SH-4 instructions and zero RAW_SH4 operations in that chosen reachable closure. This is a stronger milestone than the earlier 31,540-word raw probe because those words now pass through the actual recompilation pipeline rather than only static discovery.

The first runtime blockers are now pre-main Katana bootstrap assumptions. In particular, early startup reads system/global memory that the Dreamcast boot ROM / loader normally prepares before handing execution to 1ST_READ.BIN. That state must be modeled generically before judging game logic itself.

The startup path also uses P2 (0xA...) aliases for code that is canonically stored in the P1 (0x8...) image. 0.0.41 canonicalizes these code targets when resolving generated functions.

Use `run_commercial_recompile.bat` to generate and build the commercial project locally, and `run_commercial_recompiled.bat` to execute it until the next actionable dependency.
