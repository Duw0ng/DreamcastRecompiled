#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

PATTERN = re.compile(r"No recompiled/native target registered for Dreamcast address (0x[0-9A-Fa-f]{8})")
KNOWN_BASE = [
    0x8C036102,
    0x8C036118,
    0x8C036160,
    0x8C036236,
    0x8C036280,
    0x8C036526,
]


def fmt(v: int) -> str:
    return f"0x{v:08X}"


def read_seeds(path: Path) -> set[int]:
    out: set[int] = set()
    if not path.exists():
        return out
    for raw in path.read_text(encoding="utf-8", errors="ignore").splitlines():
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        try:
            out.add(int(line, 0) & 0xFFFFFFFF)
        except ValueError:
            pass
    return out


def valid_guest_target(v: int) -> bool:
    # Keep automatic promotion constrained to aligned P1 main-RAM code addresses.
    return (v & 1) == 0 and 0x8C010000 <= v < 0x8D000000


def newest_logs(logs: Path, count: int) -> list[Path]:
    if not logs.exists():
        return []
    items = [p for p in logs.glob("DreamcastRecomp*_session_*.log") if p.is_file()]
    items.sort(key=lambda p: p.stat().st_mtime, reverse=True)
    return items[:count]


def main() -> int:
    ap = argparse.ArgumentParser(description="Extend the ChuChu Late-AOT overlay from recent missing-target logs.")
    ap.add_argument("--root", required=True)
    ap.add_argument("--game-dir", required=True)
    ap.add_argument("--logs", required=True)
    ap.add_argument("--scan-count", type=int, default=8)
    ap.add_argument("--force-known", action="store_true")
    args = ap.parse_args()

    root = Path(args.root).resolve()
    game_dir = Path(args.game_dir).resolve()
    logs = Path(args.logs).resolve()
    seed_file = game_dir / "late_aot" / "late_target_seeds.txt"
    current = read_seeds(seed_file)

    wanted: set[int] = set(KNOWN_BASE if args.force_known else [])
    evidence: list[tuple[Path, int]] = []
    for log in newest_logs(logs, max(1, args.scan_count)):
        text = log.read_text(encoding="utf-8", errors="ignore")
        for m in PATTERN.finditer(text):
            v = int(m.group(1), 16)
            if valid_guest_target(v):
                wanted.add(v)
                evidence.append((log, v))

    missing = sorted(wanted - current)
    if evidence:
        last_log, last_target = evidence[0]
        print(f"[AUTO-LATE-AOT] Latest evidence: {last_log.name} -> {fmt(last_target)}")
    if not missing:
        print("[AUTO-LATE-AOT] Overlay already covers every recent missing target.")
        return 0

    print("[AUTO-LATE-AOT] Adding: " + ", ".join(fmt(x) for x in missing))
    builder = root / "tools" / "late_target" / "BuildLateTargetPatch.py"
    cmd = [sys.executable, str(builder), "--root", str(root), "--game-dir", str(game_dir)]
    # Pass all known/current targets explicitly; builder also preserves seed_file entries.
    for v in sorted(current | wanted):
        cmd += ["--target", fmt(v)]
    return subprocess.run(cmd, cwd=str(root)).returncode


if __name__ == "__main__":
    raise SystemExit(main())
