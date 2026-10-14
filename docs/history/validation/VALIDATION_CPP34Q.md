# Validation cpp34q — Late-AOT Target Overlay

- Stable `runtime/include/dc_runtime.hpp` SHA-256 remains `ca75e79316dc78ae74021a0853312f4c3f378a0f0524be0371c0c3db263e9739`.
- No guest ABI field changes.
- Synthetic raw SH-4 target was emitted with `RAW_SH4=0` and `Unknown SH-4=0`.
- Overlay linked as a standalone shared plugin against the stable DreamcastRuntime.
- Plugin host loaded kind 100 and startup registered the generated target in `DCRuntime::targets`.
- Persistent seed accumulation tested with two sequential Late-AOT targets; regenerated overlay contained both targets.
- The original guest generated directory/build was not modified by the overlay generator.
- Real target `0x8C036102` was not locally executed because the user's cached ChuChu `BOOTSTRAP.BIN` is not available in this environment. Its runtime dump begins with eight instructions that all decode as known SH-4 operations, so it is suitable for the targeted generator to validate on the user's cache.
