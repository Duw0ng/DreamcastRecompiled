# Dreamcast Recompiled v1.2 — Classic Original Launcher V5

This is a **fully functional interface**, not a mockup or an image used as a button. It uses Tkinter (included in standard Python) with separate transparent PNG artwork. Buttons, visual states, click detection, keyboard input, language, clock, and expandable advanced panel are all rendered and controlled in real time.

## V5 corrections

The original launcher's artwork and colors were restored. Moving the mouse across empty space no longer reactivates the previously selected button. Buttons have matching normal/highlighted assets; only the corresponding image changes rather than redrawing the whole window. Row overlap was removed, and advanced selection rings reset when the panel closes. The clock and every window remain genuine controls.

See `RELEASE_NOTES_CLASSIC_ORIGINAL_V5.md` for tests and limitations.

## Main screen

- **Eight functional primary buttons:** Play, Games, VMU/Files, Music, Settings, Controllers, Display, and Network.
- **Nine advanced buttons:** Audio, Video, Performance, Saves, BIOS/System, Per-game Profiles, Debug, Language, and Themes.
- **Subtle centered glow** on hover or selection; the former misaligned white rectangle was removed.
- **PC date and time**, refreshed every second; the current screen does not force the year 1998.
- **Collapsible advanced controls:** nine icons are hidden, the window physically shrinks, and this preference persists across launches.
- Duplicate bottom buttons for Games and Play were removed.
- English and Spanish native menus support arrow keys, Tab, Enter, and mouse navigation.
- Artwork is pre-rendered for **1152, 960, and 800 px** desktop widths without requiring Pillow on Windows.

## Implemented features and limits

| Feature | Behavior |
| --- | --- |
| Play | Runs `scripts/stable/TEST_CPP38E_FAST.bat` and reuses the SH-4 guest cache |
| Games | CDI/ISO library: add, select, remove, play, access profiles, and explicitly prepare SH-4 |
| VMU / Saves | Opens persistent system files and provides backup options |
| Music | Plays local WAV files with WinMM on Windows; does not modify in-game CDDA audio |
| Settings / Audio / Video / Performance | Changes only options supported by the stable runtime |
| Controllers | Opens the stable `Configurar_Mando.bat` and game profiles |
| Display | Opens DX11/software rendering settings; does not invent VSync/fullscreen support |
| Network | Opens profiles and logs; does not claim operational online connectivity |
| Debug | Log viewer and trace options |
| Per-game Profiles | Opens the real `game.cfg` in the system's editor |
| Language and Themes | Persists selections in launcher settings |

Settings are stored in `%LOCALAPPDATA%\DreamcastRecompiled\launcher_v1_2\settings.json`. **This launcher change does not modify** the cpp38e runtime, the ABI1 SH-4 guest, its cache, or binary modules.

## Launch on Windows

Double-click `INICIAR_DREAMCAST_RECOMPILED_V1_2.bat` or `ABRIR_LAUNCHER.bat` (the source checkout also provides the English alias `OPEN_LAUNCHER.bat`). This original Python/Tkinter interface requires Python 3 with Tkinter but no additional PyPI packages. An alternative console runner is `INICIAR_DREAMCAST_RECOMPILED_V1_2_CLASICO.bat`.

The package contains the complete original project, not just the artwork. The front-end was tested under Linux/Xvfb; at the time of this historical report, integration testing with retail games on the user's Windows system was still pending.

## V7 — Extended settings

Open **Settings**, **Display**, **Audio**, or **Performance** for the native panels. Changes compatible with cpp38e apply on the next launch. Future options are labeled **Engine pending** and stored without enabling unavailable runtime features. **Launcher window size** is a functioning option and requires restarting the interface. Stable-profile restoration and JSON import/export are available in Settings.

Technical reference: `TECHNICAL_VIDEO_INTEGRATION_V7.md`.

## V8 experimental video (no VSync)

Under **Settings → Video**, select an **output resolution** (native, 960p, 1080p, 1440p, desktop) and an aspect ratio of 4:3 or 16:9. The launcher passes `DCR_VIDEO_RESOLUTION` and `DCR_VIDEO_ASPECT` to the runtime **only for the launched game**.

These controls do **not** change PVR's internal resolution or provide genuine widescreen rendering: 16:9 **stretches** the Dreamcast's 4:3 image. DX11 uses a GPU presentation pass; the software backend uses StretchDIBits. Native 4:3 mode bypasses the new pass. VSync was not introduced; `Present(0, 0)` remains unchanged.

The original V8 backend still required additional Windows build and gameplay tests. In case of regression, restore `native / 4:3` without modifying the SH-4 cache.

See `RELEASE_NOTES_PRESENTATION_V8.md`.
