#!/usr/bin/env python3
"""Reassemble exact user-provided ZIPs from private GitHub Git-blob chunks.

GitHub Actions uses GH_TOKEN to download blobs already registered in binary-sources.
Only creates the target if its exact size, SHA-256, ZIP integrity, and guest ID match.
"""
import argparse
import base64
import hashlib
import json
import pathlib
import re
import subprocess
import sys
import zipfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
GAME_RE = re.compile(r"^[A-Z0-9][A-Z0-9_]{1,50}$")
TAG_RE = re.compile(r"^[A-Za-z0-9._-]{3,100}$")
SHA_RE = re.compile(r"^[0-9a-f]{40}$")
SHA256_RE = re.compile(r"^[0-9a-f]{64}$")
NO_MEDIA = {".cdi", ".gdi", ".chd", ".iso", ".bin", ".cue", ".nrg", ".mdf", ".mds"}
REQUIRED = {"DreamcastRecompiled.exe","bin/DreamcastRuntime.dll",
            "bin/dcr_game_module.dll","bin/dreamcast_program.exe",
            "portable_manifest.txt","README_TESTER.txt"}

def verify_manifest(m):
    if m.get("schema_version") != 1 or not GAME_RE.fullmatch(m.get("game_id", "")):
        raise ValueError("Unsupported schema or invalid game identifier")
    for field in ("release_tag", "asset"):
        if not TAG_RE.fullmatch(m.get(field, "")):
            raise ValueError("Bad " + field)
    if not m["asset"].endswith(".zip") or "/" in m["asset"]:
        raise ValueError("Only root ZIP filenames allowed")
    if not SHA256_RE.fullmatch(m.get("sha256", "")) or not isinstance(m.get("size_bytes"),int):
        raise ValueError("Missing expected SHA-256 or byte count")
    shas = m.get("chunks", [])
    if not isinstance(shas,list) or not 1 <= len(shas) <= 100 or any(not SHA_RE.fullmatch(s) for s in shas):
        raise ValueError("Malformed Git blob SHA list")

def check_zip(filename, m):
    with zipfile.ZipFile(filename) as z:
        infos = [info for info in z.infolist() if not info.is_dir()]
        paths = [info.filename for info in infos]
        if len(paths) != len(set(paths)):
            raise ValueError("Duplicate ZIP member")
        for n in paths:
            p = pathlib.PurePosixPath(n)
            if p.is_absolute() or ".." in p.parts or "\\" in n or ":" in n:
                raise ValueError("Unsafe ZIP path " + n)
            if p.suffix.lower() in NO_MEDIA:
                raise ValueError("Possible disc/firmware file included: " + n)
        if not REQUIRED.issubset(paths):
            raise ValueError("Missing expected portable files " + str(REQUIRED - set(paths)))
        if z.testzip() is not None:
            raise ValueError("ZIP CRC verification failed")
        text = z.read("portable_manifest.txt").decode("utf-8-sig")
        guest = next((ln.split("=", 1)[1].strip()
                      for ln in text.splitlines() if ln.startswith("GAME=")), "")
        if guest != m["game_id"]:
            raise ValueError("Guest module identity mismatch: " + guest)
        print("Verified ZIP:", len(infos), "files, guest", guest)

def assemble(manifest, output, repo):
    m = json.loads(pathlib.Path(manifest).read_text(encoding="utf-8"))
    verify_manifest(m)
    output = pathlib.Path(output)
    output.mkdir(parents=True, exist_ok=True)
    target = output / m["asset"]
    h = hashlib.sha256()
    with target.open("wb") as out:
        for idx, sha in enumerate(m["chunks"]):
            api = f"repos/{repo}/git/blobs/{sha}"
            raw = subprocess.run(["gh","api",api, "-H","Accept: application/vnd.github+json"],
                                 check=True,stdout=subprocess.PIPE).stdout
            blob = json.loads(raw)
            if blob.get("sha") != sha or blob.get("encoding") != "base64":
                raise ValueError("Unexpected blob response " + sha)
            data = base64.b64decode(blob["content"],validate=False)
            out.write(data)
            h.update(data)
            print("Assembled chunk",idx+1,"of",len(m["chunks"]),len(data),"bytes")
    actual = h.hexdigest()
    if target.stat().st_size != m["size_bytes"] or actual != m["sha256"]:
        target.unlink(missing_ok=True)
        raise ValueError("SOURCE ZIP SHA-256 / size mismatch: " + actual)
    check_zip(target, m)
    (output / (m["asset"] + ".sha256")).write_text(
        actual + "  " + m["asset"] + "\n", encoding="ascii")
    print("VERIFIED:", target, target.stat().st_size, actual)

if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("--manifest",required=True)
    p.add_argument("--output",default="dist")
    p.add_argument("--repo",required=True)
    args = p.parse_args()
    try: assemble(args.manifest,args.output,args.repo)
    except (ValueError, OSError, subprocess.CalledProcessError, zipfile.BadZipFile) as e:
        print("RELEASE ERROR:",e,file=sys.stderr)
        sys.exit(1)
