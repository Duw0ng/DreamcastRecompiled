# Per-game SH-4 function limits — Dreamcast Recompiled v1.2

This revision is based on `TESTER_DEBUG_MENU_WINSOCK_FIXED` and does not change the SH-4 runtime, plugins, Winsock, or rendering implementations.

## Observed issue

The historical SGGG reference contains **6,393 reachable SH-4 functions**. The historical profile value **6,144** was **not the maximum final function count**; it was `max_closure_entries`, the discovery-entry budget. The actual `max_functions` setting was 8,192.

A closure budget of 6,144 does not necessarily prevent 6,393 functions from being observed at the end, since additional functions may appear during final analysis. Raising the budget permits more candidate entries to be explored, but does not guarantee more correct functions or better startup. Monitor `Unknown SH-4`, `RAW_SH4`, rejected targets, and CFG quality.

## New defaults

| Game | max_functions | max_closure_entries | closure_passes |
|---|---:|---:|---:|
| SGGG | 10240 | 7680 | 64 |
| ChuChu Rocket! | 8192 | 6144 | 64 |
| Crazy Taxi 2 | 8192 | 6144 | 64 |
| Daytona USA | 8192 | 6144 | 64 |
| Record of Lodoss War | 10000 | 9000 | 64 |
| Unknown titles | 8192 | 6144 | 64 |

**Important:** persistent profiles are independent for every game. SGGG's automatic migration applies only to the old v1 profile with the exact 8192/6144 values. Custom profiles are not overwritten.

## Configure limits from the launcher

1. Open `ABRIR_LAUNCHER.bat` (or the English alias `OPEN_LAUNCHER.bat`) and select **Per-game profiles** in the advanced panel.
2. Choose the game to configure.
3. Adjust **Maximum final functions**, **Closure entries**, or **Analysis passes**.
4. For SGGG, choose **Recommended SGGG values** and then **Save limits**.
5. Choose **Games → Prepare SH-4**, or run `run_game.bat "game.cdi"` without `--reuse`, to rebuild that title's guest. The normal **Play** button uses `--reuse` and stops if limits have changed; it does **not** rebuild automatically. Other games' caches are not touched.

You can also edit `%LOCALAPPDATA%\DreamcastRecompiled\games\<GAME>\game.cfg` manually. After an external edit, run `run_game.bat "path\game.cdi" --recompile-game` to apply the changes, since manual edits do not create the launcher's pending-change marker.

`--reuse` must never recompile. Pending limit changes cause the old module to be rejected with exit code **14**, indicating that a normal preparation run is required.

## Telemetry to inspect

- `Closure entry budget: 7680 / 10240` for SGGG.
- `Synthetic entries` and `Reachable functions` count **different concepts**; they do not need to match.
- `Unknown SH-4: 0` and `RAW_SH4: 0` are quality targets for included instructions.
- `Closure cap hit` means discovery may have been limited by the configured budget.

At the time of this original revision, SGGG startup still required Windows compilation and gameplay verification.

## Verification performed before packaging

- All 118 Python-launcher tests passed under Linux/Xvfb.
- A GUI test successfully saved 10240/7680/64 to a simulated SGGG profile.
- Unrelated settings were preserved, and only the selected game profile changed.
- **Windows PowerShell, an actual MSVC rebuild, and SGGG CDI gameplay were not executed during that original test.**
