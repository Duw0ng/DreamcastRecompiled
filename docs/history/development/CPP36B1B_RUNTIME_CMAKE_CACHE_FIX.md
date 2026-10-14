# cpp36b1b — Runtime CMake cache fix

This checkpoint fixes a false failure when moving between extracted DreamcastRecomp folders.

CMake stores the absolute `runtime/` source directory inside the separate runtime build tree. Therefore a tree configured from e.g. `cpp35b1/.../runtime` cannot be configured again from `cpp36b1a/.../runtime`.

cpp36b1b now inspects `CMAKE_HOME_DIRECTORY` before configuring the runtime. If it points to another extraction, only the runtime CMake tree is removed and regenerated.

The following are NOT deleted or reconfigured:

- the game's `generated/` tree;
- the game's main `build/` CMake tree;
- `build/Release/dcr_game_module.dll`;
- the adopted portable-safe guest marker;
- VMU/flash data.

This means the fix can rebuild Runtime/Network/Maple without recompiling the SH-4 guest.
