#!/usr/bin/env python3
import argparse
import shutil
import sys
import zipfile
from pathlib import Path


def copy_required(src: Path, dst: Path) -> None:
    if not src.is_file():
        raise FileNotFoundError(f"missing required portable input: {src}")
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)


def main() -> int:
    ap = argparse.ArgumentParser(description="Synchronize a ready-to-run DreamcastRecompiled portable from an existing ABI1 guest cache.")
    ap.add_argument("--root", required=True)
    ap.add_argument("--game-dir", required=True)
    ap.add_argument("--build-dir", required=True)
    ap.add_argument("--slug", required=True)
    ap.add_argument("--title", default="")
    ap.add_argument("--abi", default="1")
    ap.add_argument("--runtime-id", required=True)
    args = ap.parse_args()

    root = Path(args.root).resolve()
    game_dir = Path(args.game_dir).resolve()
    build_dir = Path(args.build_dir).resolve()
    launcher = root / "portable" / "DreamcastRecompiled.exe"
    tools_dir = root / "build" / "Release"

    final = game_dir / "portable"
    staging = game_dir / "portable.__sync__"
    if staging.exists():
        shutil.rmtree(staging)
    (staging / "bin" / "plugins").mkdir(parents=True, exist_ok=True)

    try:
        copy_required(launcher, staging / "DreamcastRecompiled.exe")
        for name in ("dreamcast_program.exe", "DreamcastRuntime.dll", "dcr_game_module.dll"):
            copy_required(build_dir / "Release" / name, staging / "bin" / name)
        for name in ("dcr_network.dll", "dcr_maple.dll", "dcr_gdrom.dll"):
            copy_required(build_dir / "plugins" / "Release" / name, staging / "bin" / "plugins" / name)
        for name in ("dc_disc_probe.exe", "dc_boot_prepare.exe"):
            copy_required(tools_dir / name, staging / "bin" / name)
    except FileNotFoundError as exc:
        print(f"[PORTABLE] WARN: {exc}")
        shutil.rmtree(staging, ignore_errors=True)
        return 2

    # Only native DLLs and their manifests, never BOOTSTRAP.BIN/guest image.
    patches = game_dir / "incremental" / "native"
    if patches.exists():
        for module in sorted(patches.glob("dcr_sh4_patch_*.dll")):
            copy_required(module, staging / "bin" / "incremental" / "native" / module.name)
            manifest = module.with_suffix(".json")
            if manifest.is_file():
                copy_required(manifest, staging / "bin" / "incremental" / "native" / manifest.name)

    (staging / "portable_manifest.txt").write_text(
        "DCR_PORTABLE_GUEST=1\n"
        f"GUEST_ABI={args.abi}\n"
        f"RUNTIME={args.runtime_id}\n"
        f"GAME={args.slug}\n"
        f"TITLE={args.title}\n",
        encoding="utf-8",
    )
    (staging / "README_TESTER.txt").write_text(
        "DreamcastRecompiled portable auto-sync\n\n"
        "Open DreamcastRecompiled.exe and select your own original CDI/GDI.\n"
        "The package contains no original game image, IP.BIN, BOOT.DISC.BIN, or BOOTSTRAP.BIN.\n"
        "The launcher prepares the retail bootstrap locally from the selected disc and feeds it to the precompiled guest at runtime.\n",
        encoding="utf-8",
    )

    forbidden_ext = {".cdi", ".gdi"}
    forbidden_names = {"ip.bin", "boot.disc.bin", "bootstrap.bin", "boot.bin"}
    for path in staging.rglob("*"):
        if not path.is_file():
            continue
        if path.suffix.lower() in forbidden_ext or path.name.lower() in forbidden_names:
            print(f"[PORTABLE] AUDIT FAILURE: retail file leaked into portable: {path}")
            shutil.rmtree(staging, ignore_errors=True)
            return 3

    if final.exists():
        shutil.rmtree(final)
    staging.rename(final)

    zip_path = game_dir / f"{args.slug}_portable_latest.zip"
    if zip_path.exists():
        zip_path.unlink()
    with zipfile.ZipFile(zip_path, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as zf:
        for path in sorted(final.rglob("*")):
            if path.is_file():
                zf.write(path, path.relative_to(final))

    print(f"[PORTABLE] READY: {final}")
    print(f"[PORTABLE] ZIP  : {zip_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
