#!/usr/bin/env python3
"""Make versioned DCR v1.2 direct-portable *test* ZIPs.

Only replace DreamcastRuntime.dll (universal boot and Rev 2 AICA fixes),
relabel the inherited ChuChu GUI *display string in place* without changing
the original executable's size or any Go string length, and update metadata.
Never replace the original launcher with an alternate program.
"""
from __future__ import annotations

import argparse
import hashlib
import pathlib
import re
import sys
import zipfile

GAMES = {
    "CHUCHU_ROCKET": {
        "title": "ChuChu Rocket!", "ui": "ChuChu Rocket!",
        "sha256": "b5ee29e5411fc6ea250969fef1a9ac6b57049352f97c374031abf636be1ab4ab",
    },
    "SGGG": {
        "title": "Segagaga (SGGG)", "ui": "SGGG: Segagaga",
        "sha256": "cff571dce69468a62adff870242253de998b53e169b930f49a7810ec7ed5dca9",
    },
    "SEGA_TETRIS": {
        "title": "Sega Tetris", "ui": "Sega Tetris DC",
        "sha256": "1f31d4cd04853ad833edc7047ae218489638bc92991abdafc0852929025ef6db",
    },
    "MR_DRILLER": {
        "title": "Mr. Driller", "ui": "Mr. Driller DC",
        "sha256": "af2e89c9a145462fbfdad017fc706ac927792359270b7a160a22587df60d57d9",
    },
}
OLD_UI = b"ChuChu Rocket!"
MEDIA = {".cdi", ".gdi", ".chd", ".iso", ".cue", ".nrg", ".mds", ".mdf"}
REQUIRED = {
    "DreamcastRecompiled.exe", "portable_manifest.txt", "bin/dreamcast_program.exe",
    "bin/dcr_game_module.dll", "bin/DreamcastRuntime.dll",
    "bin/dc_disc_probe.exe", "bin/dc_boot_prepare.exe",
}

def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

def read_manifest(data: bytes) -> dict[str, str]:
    rows = {}
    for line in data.decode("utf-8-sig").splitlines():
        if "=" in line:
            k, v = line.split("=", 1)
            rows[k.strip().upper()] = v.strip()
    return rows

def checksums_replace(data: bytes, changes: dict[str, tuple[str, str]]) -> bytes:
    """Adjust sha256 index entries only for binaries actually replaced."""
    out = []
    for line in data.decode("utf-8-sig").splitlines(keepends=True):
        clean = line.rstrip("\r\n")
        m = re.fullmatch(r"([0-9a-fA-F]{64})([ \t]+\*?)(.+)", clean)
        if m:
            path = m.group(3).replace("\\", "/").lstrip("./")
            change = changes.get(path)
            if change:
                old, new = change
                if m.group(1).lower() != old.lower():
                    raise ValueError("Stale checksum entry for " + path)
                line = new + m.group(2) + m.group(3) + line[len(clean):]
        out.append(line)
    return "".join(out).encode("utf-8")

def relabel_launcher(exe: bytes, game: str) -> tuple[bytes, int]:
    """Patch only known, equal-length human-readable Go string constants.

    Preserve binary size and original PE code layout. Do not change the
    internal CHUCHU_ROCKET constant: it may be used as an ABI/cache key.
    """
    if game == "CHUCHU_ROCKET":
        return exe, 0
    new = GAMES[game]["ui"].encode("ascii")
    if len(new) != len(OLD_UI):
        raise ValueError("UI string replacement must be exactly 14 bytes")
    if not exe.startswith(b"MZ"):
        raise ValueError("Expected original Windows PE launcher")
    matches = exe.count(OLD_UI)
    result = exe.replace(OLD_UI, new)
    if len(result) != len(exe):
        raise ValueError("PE size unexpectedly modified")
    if exe[:1024] != result[:1024]:
        raise ValueError("PE executable headers changed")
    return result, matches

def build(source: pathlib.Path, runtime: pathlib.Path, out_dir: pathlib.Path, game: str) -> pathlib.Path:
    if game not in GAMES:
        raise ValueError("Unknown game")
    info = GAMES[game]
    actual = sha(source.read_bytes())
    if actual.lower() != info["sha256"]:
        raise ValueError(f"Unexpected original {game} ZIP: {actual}")
    new_runtime = runtime.read_bytes()
    if not new_runtime.startswith(b"MZ"):
        raise ValueError("Expected rebuilt Windows DLL")
    with zipfile.ZipFile(source) as zin:
        if zin.testzip() is not None:
            raise ValueError("Original ZIP CRC failed")
        members = [item for item in zin.infolist() if not item.is_dir()]
        names = [item.filename for item in members]
        if len(names) != len(set(names)):
            raise ValueError("Duplicate members")
        if not REQUIRED.issubset(names):
            raise ValueError("Missing direct-portable files: " + str(REQUIRED-set(names)))
        for name in names:
            p = pathlib.PurePosixPath(name)
            if p.is_absolute() or ".." in p.parts or "\\" in name or ":" in name:
                raise ValueError("Unsafe path: " + name)
            if p.suffix.lower() in MEDIA:
                raise ValueError("A disc image must not be redistributed: " + name)
        data = {name: zin.read(name) for name in names}

    manifest = read_manifest(data["portable_manifest.txt"])
    if manifest.get("GAME") != game or manifest.get("GUEST_ABI") != "1":
        raise ValueError(
            f"Wrong module identity or guest ABI: GAME={manifest.get('GAME')!r}; "
            f"GUEST_ABI={manifest.get('GUEST_ABI')!r}; expected={game!r}/'1'"
        )

    old_runtime = data["bin/DreamcastRuntime.dll"]
    old_launcher = data["DreamcastRecompiled.exe"]
    data["bin/DreamcastRuntime.dll"] = new_runtime
    new_launcher, patch_count = relabel_launcher(old_launcher, game)
    data["DreamcastRecompiled.exe"] = new_launcher

    # Manifest is only descriptive. Keep game identity, ABI, and runtime
    # compatibility tags intact (the precompiled guest remains untouched).
    if "TITLE" in manifest and manifest["TITLE"] != info["title"]:
        lines = data["portable_manifest.txt"].decode("utf-8-sig").splitlines()
        data["portable_manifest.txt"] = ("\r\n".join(
            "TITLE=" + info["title"] if x.startswith("TITLE=") else x for x in lines
        ) + "\r\n").encode("utf-8")

    if "README_TESTER.txt" in data and game != "CHUCHU_ROCKET":
        readme = data["README_TESTER.txt"].decode("utf-8-sig")
        readme = readme.replace("ChuChu Rocket!", info["title"])
        data["README_TESTER.txt"] = readme.encode("utf-8")

    # Extra integrity sidecars found in e.g. Sega Tetris must be regenerated
    # or the altered runtime/branding would trigger their mismatch guard.
    changes = {
        "bin/DreamcastRuntime.dll": (sha(old_runtime), sha(new_runtime)),
        "DreamcastRecompiled.exe": (sha(old_launcher), sha(new_launcher)),
    }
    for name in names:
        if name.endswith(".sha256") and name not in changes:
            data[name] = checksums_replace(data[name], changes)

    data["DCRV1.2_UNIVERSAL_PORTABLE_FIX.txt"] = (
        "DreamcastRecompiled v1.2 - universal portable boot test\r\n"
        f"Game: {info['title']} ({game})\r\n"
        "Same original standalone GUI, game-specific SH-4 module and plugins.\r\n"
        "Runtime now selects scrambled for a raw CDI track when the old\r\n"
        "launcher requests plain, while preserving 32768-byte IP.BIN.\r\n"
        "Native Rev 2 AICA one-shot replay fix included.\r\n"
        "No game image or console firmware included.\r\n"
        "Experimental: does not replace the Stable v1.2 release.\r\n"
        "Internal legacy ChuChu constants and compatibility paths are intact.\r\n"
    ).encode("utf-8")

    out_dir.mkdir(parents=True, exist_ok=True)
    target = out_dir / f"DCRV1.2_{game}_UNIVERSAL_BOOT_TEST_WIN64.zip"
    with zipfile.ZipFile(target, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=6) as zout:
        for name in names:
            zout.writestr(name, data[name])
        zout.writestr("DCRV1.2_UNIVERSAL_PORTABLE_FIX.txt",
                      data["DCRV1.2_UNIVERSAL_PORTABLE_FIX.txt"])
    with zipfile.ZipFile(target) as verify:
        if verify.testzip() is not None:
            raise ValueError("Output ZIP CRC failed")
        if sha(verify.read("bin/dcr_game_module.dll")) != sha(data["bin/dcr_game_module.dll"]):
            raise ValueError("Game module changed")
        if verify.read("DreamcastRecompiled.exe") != new_launcher:
            raise ValueError("Original GUI mutated")
        if read_manifest(verify.read("portable_manifest.txt"))["GAME"] != game:
            raise ValueError("Game identity changed")
    digest = sha(target.read_bytes())
    target.with_suffix(target.suffix + ".sha256").write_text(
        digest + "  " + target.name + "\n", encoding="ascii")
    print(f"[PASS] {game}: original GUI size={len(old_launcher)}; branding occurrences={patch_count}")
    print(f"[PASS] game guest unchanged, universal runtime={sha(new_runtime)}, original ZIP={actual}")
    print(f"[PASS] ZIP={target} bytes={target.stat().st_size} sha256={digest}")
    return target

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--game", choices=sorted(GAMES), required=True)
    ap.add_argument("--source", type=pathlib.Path, required=True)
    ap.add_argument("--runtime", type=pathlib.Path, required=True)
    ap.add_argument("--output", type=pathlib.Path, required=True)
    args = ap.parse_args()
    try:
        build(args.source, args.runtime, args.output, args.game)
        return 0
    except (OSError, ValueError, zipfile.BadZipFile) as exc:
        print("PACK ERROR:", exc, file=sys.stderr)
        return 1

if __name__ == "__main__":
    raise SystemExit(main())
