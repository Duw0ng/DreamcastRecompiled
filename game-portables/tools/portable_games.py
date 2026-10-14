#!/usr/bin/env python3
"""Verified multi-game Windows portable repackager. Requires Python 3.9+ only."""
import argparse
import hashlib
import json
import pathlib
import re
import sys
import zipfile

BASE = pathlib.Path(__file__).resolve().parents[1]
ID_RE = re.compile(r"^[A-Z][A-Z0-9_]{1,59}$")
TAG_RE = re.compile(r"^[a-zA-Z0-9][a-zA-Z0-9._-]{2,99}$")
HASH_RE = re.compile(r"^[0-9a-fA-F]{64}$")
MEDIA = {".cdi", ".gdi", ".iso", ".chd", ".nrg", ".cue", ".mds", ".mdf"}
DIRS = {"_internal", "launcher", "scripts", "tools", "profiles", "licenses"}
FILES = {"DreamcastRecompiled.exe", "ABRIR_LAUNCHER.bat", "VERSION",
         "LICENSE", "THIRD_PARTY_NOTICES.md", "LANGUAGES_README.txt",
         "README.md", "launcher_offline_runtime.json"}
HOST_REQUIRED = {"DreamcastRecompiled.exe", "_internal/python312.dll", "launcher/launch.py"}

def die(msg):
    raise ValueError(msg)

def load(path):
    data = json.loads(pathlib.Path(path).read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        die("Expected object: " + str(path))
    return data

def validate():
    catalog = load(BASE / "catalog.json")
    if catalog.get("schema_version") != 1: die("Unknown catalog version")
    seen = set()
    for entry in catalog.get("games", []):
        ident = entry.get("id", "")
        if not isinstance(ident, str) or not ID_RE.fullmatch(ident) or ident in seen:
            die("Duplicate/invalid game ID " + str(ident))
        seen.add(ident)
        game = load(BASE / "games" / (ident + ".json"))
        if game.get("id") != ident or game.get("platform") != "windows-x64":
            die("Game identity/platform mismatch " + ident)
        if game.get("disc", {}).get("required") is not True or entry.get("requires_user_game_image") is not True:
            die("Own game disc required: " + ident)
        for k in ("native_module", "runtime_module", "check_file"):
            if not game.get("portable", {}).get(k, "").startswith("games/" + ident + "/"):
                die("Invalid module path: " + ident)
    for path in sorted((BASE / "releases").glob("*.json")):
        s = load(path)
        ident = s.get("game_id", "")
        if s.get("schema_version") != 1 or ident not in seen:
            die("Invalid release spec: " + str(path))
        for k in ("source_release_tag", "target_tag"):
            if not TAG_RE.fullmatch(str(s.get(k, ""))): die("Invalid " + k)
        if s["source_release_tag"] == s["target_tag"]: die("Tags must differ")
        for k in ("source_asset", "target_asset"):
            v = s.get(k, "")
            if not isinstance(v, str) or pathlib.PurePosixPath(v).name != v or not v.endswith(".zip"):
                die("Unsafe filename " + k)
        if not HASH_RE.fullmatch(str(s.get("source_sha256", ""))): die("Invalid SHA-256")
        row = next(x for x in catalog["games"] if x["id"] == ident)
        if row.get("release_tag") != s["target_tag"] or row.get("release_asset") != s["target_asset"]:
            die("Catalog and release spec disagree")
    print("Validated", len(seen), "games and all release specifications")

def file_hash(path):
    h = hashlib.sha256()
    with pathlib.Path(path).open("rb") as f:
        for chunk in iter(lambda: f.read(2 << 20), b""): h.update(chunk)
    return h.hexdigest()

def pack(spec_path, source, out_dir):
    validate()
    s = load(spec_path)
    ident = s["game_id"]
    game = load(BASE / "games" / (ident + ".json"))
    if file_hash(source).lower() != s["source_sha256"].lower():
        die("Source archive SHA-256 does not match")
    prefix = "games/" + ident + "/"
    with zipfile.ZipFile(source) as zin:
        if zin.testzip(): die("Damaged input ZIP")
        files = {i.filename: i for i in zin.infolist() if not i.is_dir()}
        if len(files) != sum(not i.is_dir() for i in zin.infolist()): die("Duplicate input ZIP paths")
        for n in files:
            p = pathlib.PurePosixPath(n)
            if n.startswith("/") or "\\" in n or ":" in n or ".." in p.parts: die("Unsafe ZIP member: " + n)
            if p.suffix.lower() in MEDIA: die("Game disc image detected: " + n)
        required = HOST_REQUIRED | {prefix + x for x in (
            "portable_manifest.txt", "checksums.sha256", "bin/dreamcast_program.exe",
            "bin/dcr_game_module.dll", "bin/DreamcastRuntime.dll")}
        if not required.issubset(files): die("Required files missing: " + str(sorted(required - files.keys())))
        if "GAME=" + ident not in zin.read(prefix + "portable_manifest.txt").decode("utf-8-sig").splitlines():
            die("Game ID does not match module manifest")
        verified = 0
        for line in zin.read(prefix + "checksums.sha256").decode("utf-8-sig").splitlines():
            if not line.strip(): continue
            match = re.fullmatch(r"([a-fA-F0-9]{64})\s+\*?(.+)", line.strip())
            if not match: die("Invalid checksum manifest line")
            path = pathlib.PurePosixPath(match.group(2))
            if path.is_absolute() or ".." in path.parts: die("Unsafe checksum path")
            name = prefix + str(path)
            if name not in files or hashlib.sha256(zin.read(name)).hexdigest().lower() != match.group(1).lower():
                die("DLL/EXE checksum mismatch: " + name)
            verified += 1
        if verified < 3: die("Insufficient verified binaries")
        chosen = sorted(n for n in files if n in FILES or n.startswith(prefix) or
                        n.startswith("safe_guests/" + ident + "/") or n.split("/", 1)[0] in DIRS)
        out_dir = pathlib.Path(out_dir)
        out_dir.mkdir(parents=True, exist_ok=True)
        target = out_dir / s["target_asset"]
        with zipfile.ZipFile(target, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=6, allowZip64=True) as zout:
            for name in chosen:
                zi = zipfile.ZipInfo(name, (2026, 1, 1, 0, 0, 0))
                zi.compress_type = zipfile.ZIP_DEFLATED
                zi.external_attr = 0o100644 << 16
                zout.writestr(zi, zin.read(name), compress_type=zipfile.ZIP_DEFLATED, compresslevel=6)
            note = ("DreamcastRecompiled game portable: " + game["title"] +
                    "\r\nExtract all files on Windows x64 and start DreamcastRecompiled.exe.\r\n" +
                    "Select YOUR OWN compatible CDI using Games/Juegos. No game disc or firmware is included.\r\n" +
                    "VMU: use ABRIR_LAUNCHER.bat --vmu or Aqua's VMU menu.\r\n" +
                    "Offline path tested by maintainer; online is experimental.\r\n")
            zout.writestr("PORTABLE_README.txt", note)
    with zipfile.ZipFile(target) as verify:
        if verify.testzip() or not required.issubset(set(verify.namelist())):
            die("Output failed structural verification")
    digest = file_hash(target)
    (out_dir / (target.name + ".sha256")).write_text(digest + "  " + target.name + "\n", encoding="ascii")
    print("Created", target, "size", target.stat().st_size, "sha256", digest, "checked binaries", verified)

def main():
    p = argparse.ArgumentParser()
    sub = p.add_subparsers(dest="cmd", required=True)
    sub.add_parser("validate")
    q = sub.add_parser("pack")
    q.add_argument("--spec", type=pathlib.Path, required=True)
    q.add_argument("--source", type=pathlib.Path, required=True)
    q.add_argument("--output", type=pathlib.Path, default=pathlib.Path("dist"))
    args = p.parse_args()
    try:
        if args.cmd == "validate": validate()
        else: pack(args.spec, args.source, args.output)
    except (ValueError, OSError, zipfile.BadZipFile) as exc:
        print("PORTABLE ERROR:", exc, file=sys.stderr)
        sys.exit(2)

if __name__ == "__main__":
    main()
