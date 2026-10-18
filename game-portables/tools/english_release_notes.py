#!/usr/bin/env python3
"""Generate English-only GitHub release descriptions for DCR v1.2 Rev 2."""
import argparse
from pathlib import Path

GAMES = {
    "CHUCHU_ROCKET": ("ChuChu Rocket!", "portable-chuchu-rocket-direct-v1.2",
      "Offline gameplay has been confirmed. Online and DCNet remain experimental."),
    "SGGG": ("Segagaga (SGGG)", "portable-sggg-direct-v1.2",
      "Boot and menu navigation have been tested successfully. Some other compatibility issues may remain."),
    "SEGA_TETRIS": ("Sega Tetris", "portable-sega-tetris-direct-v1.2",
      "Gameplay, graphics and audio were observed; occasional crashes may still occur."),
    "MR_DRILLER": ("Mr. Driller", "portable-mr-driller-direct-v1.2",
      "Gameplay has been reached. Graphical defects in menus may remain."),
}

def release_text(game: str) -> str:
    title, tag, status = GAMES[game]
    audio = (
        "### Confirmed Segagaga audio fix\n\n"
        "**The repeated navigation/menu sound effects have been fixed.** When an AICA "
        "one-shot voice finishes, the runtime now clears its KYONB latch so a "
        "later global KYONEX event for another sound cannot accidentally replay it. "
        "The maintainer confirmed in gameplay that the unwanted repetition "
        "disappeared and that SGGG is more playable as a result. This does not mean "
        "that every other audio or compatibility issue is resolved.\n\n"
    ) if game == "SGGG" else (
        "### Audio\n\nThe package includes the Rev 2 AICA one-shot voice fix. "
        "Its improvement to repeated menu sounds was specifically confirmed with SGGG; "
        "no separate audio-fix claim is made for this game.\n\n"
    )
    return (
        f"# {title} — DreamcastRecompiled v1.2 Rev 2\n\n"
        "**Windows x64 · Precompiled game-specific portable · Updated October 17, 2026**\n\n"
        "The maintainer tested the updated game portables with compatible disc images "
        "and confirmed that they start. The original GUI and game-specific native SH-4 "
        "module are retained; repeated native compilation is not needed.\n\n"
        "## Changes in this release\n\n"
        "- **Universal plain/scrambled boot:** raw CDI tracks requiring scrambled staging "
        "are automatically corrected when an older frontend requests plain.\n"
        "- **IP.BIN protection:** the original 32 KiB bootstrap header is preserved.\n"
        "- **Original launcher preserved:** controller settings and VMU handling remain. "
        "Incorrect inherited ChuChu Rocket! display labels were adapted to each game, "
        "while internal module ABI identifiers remain untouched.\n"
        "- **AICA Rev 2:** completed non-looping voices no longer restart inadvertently "
        "when a different sound is triggered.\n"
        "- **Title-specific compiled SH-4 module and hardware plugins remain unchanged.**\n\n"
        + audio +
        "## Known status and limitations\n\n" + status + "\n\n"
        "## Installation\n\n"
        "1. Download and fully extract the attached portable ZIP.\n"
        "2. Run DreamcastRecompiled.exe using the original launcher interface.\n"
        f"3. Select your own compatible {title} CDI/GDI image.\n\n"
        "No commercial game image or console BIOS is included. Compatibility is "
        "specific to tested game revisions and host configurations.\n\n"
        f"**Earlier portable:** [original v1.2 archive](https://github.com/Duw0ng/DreamcastRecompiled/releases/tag/{tag}-initial-archive). "
        "The general v1.2 Stable release was not replaced.\n"
    )

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--game", choices=sorted(GAMES), required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(release_text(args.game), encoding="utf-8")

if __name__ == "__main__":
    main()
