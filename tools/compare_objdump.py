#!/usr/bin/env python3
"""Run sh-elf-objdump and dc_analyzer against the same ELF.

The script preserves both full reports and creates a simple side-by-side text
file. It intentionally does not claim semantic equivalence yet; the goal in
0.0.2 is to make discrepancies easy to inspect while the decoder grows.
"""
from __future__ import annotations

import argparse
import subprocess
from pathlib import Path


def run(cmd: list[str]) -> str:
    proc = subprocess.run(cmd, text=True, stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT, check=False)
    if proc.returncode != 0:
        raise SystemExit(f"Command failed ({proc.returncode}): {' '.join(cmd)}\n{proc.stdout}")
    return proc.stdout


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("elf")
    ap.add_argument("--objdump", default="sh-elf-objdump")
    ap.add_argument("--analyzer", default="dc_analyzer")
    ap.add_argument("--out-dir", default="compare")
    ap.add_argument("--max-instructions", default="200")
    args = ap.parse_args()

    out = Path(args.out_dir)
    out.mkdir(parents=True, exist_ok=True)

    ref = run([args.objdump, "-d", args.elf])
    ours = run([args.analyzer, args.elf, "--disassemble", "--from-entry",
                "--max-instructions", str(args.max_instructions), "--stats"])

    (out / "objdump.txt").write_text(ref, encoding="utf-8")
    (out / "dcrecomp.txt").write_text(ours, encoding="utf-8")

    left = ref.splitlines()
    right = ours.splitlines()
    width = min(max([len(x) for x in left] + [20]), 100)
    rows = []
    for idx in range(max(len(left), len(right))):
        l = left[idx] if idx < len(left) else ""
        r = right[idx] if idx < len(right) else ""
        rows.append(f"{l[:width]:<{width}} | {r}")
    (out / "side_by_side.txt").write_text("\n".join(rows) + "\n", encoding="utf-8")

    print(f"Wrote {out / 'objdump.txt'}")
    print(f"Wrote {out / 'dcrecomp.txt'}")
    print(f"Wrote {out / 'side_by_side.txt'}")


if __name__ == "__main__":
    main()
