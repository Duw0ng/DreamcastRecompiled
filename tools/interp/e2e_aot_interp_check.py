#!/usr/bin/env python3
"""End-to-end check: real AOT guest + SH-4 interpreter fallback + seed learning.

Linux/maintainer tool (needs gcc-sh4-linux-gnu, a host C compiler, CMake and a
built dc_raw_recomp). It reproduces the whole production loop on a small
GCC-compiled SH-4 program (tests/interp/interp_corpus.c):

  1. Recompile with the AOT closure seeded with every function EXCEPT the ones
     only reachable through function pointers (dc_square, op_*, quicksort).
  2. Build the modular guest against runtime/ and run it with
     DCR_SH4_INTERP=0: the historical "No recompiled/native target" abort.
  3. Run it again with the interpreter: results must equal the host build of
     the same C source, and the missing entrypoints are written as seeds.
  4. Recompile with the learned seed file: the next run interprets nothing.

    python tools/interp/e2e_aot_interp_check.py --recomp build/dc_raw_recomp
"""
import argparse
import os
import pathlib
import shutil
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
SRC = ROOT / "tests" / "interp"
BASE = 0x8C010000
OUT = 0x8C900000
WORDS = 33
CFLAGS = ["-O2", "-ml", "-m4", "-ffreestanding", "-fno-builtin", "-fno-pic", "-fno-pie",
          "-ffp-contract=off", "-fno-math-errno", "-fno-stack-protector"]
POINTER_ONLY = {"dc_square", "op_add", "op_sub", "op_xor", "op_mul", "op_mix", "quicksort"}

STUB = """
        .little
        .text
        .global e2e_start
        .align 2
e2e_start:
        sts.l   pr,@-r15
        mov.l   .Lfpscr, r0
        lds     r0, fpscr
        mov.l   .Lbuf, r4
        mov.l   .Lmain, r0
        jsr     @r0
        nop
        lds.l   @r15+, pr
        rts
        nop
        .align 2
.Lbuf:   .long {out:#x}
.Lmain:  .long dc_test_main
.Lfpscr: .long 0x00080000
"""


def run(cmd, **kw):
    return subprocess.run(cmd, check=True, **kw)


def nm(prefix, elf):
    out = run([prefix + "nm", str(elf)], capture_output=True, text=True).stdout
    return {p[2]: int(p[0], 16) for p in (l.split() for l in out.splitlines()) if len(p) == 3 and p[1] in "Tt"}


def host_expected(tmp):
    src = tmp / "host.c"
    src.write_text('#include <stdio.h>\n#include "%s"\nint main(void){u32 o[DC_COUNT];dc_test_main(o);'
                   'for(int i=0;i<%d;i++)printf("%%08X\\n",o[i]);return 0;}\n' % ((SRC / "interp_corpus.c").as_posix(), WORDS))
    exe = tmp / "host"
    run(["cc", "-O2", "-ffp-contract=off", str(src), "-o", str(exe), "-lm"])
    return run([str(exe)], capture_output=True, text=True).stdout.split()


def guest_run(build, env_extra):
    exe = build / "dreamcast_program"
    args = [str(exe)] + [f"--peek32={OUT + 4 * i:#010x}" for i in range(WORDS)]
    env = dict(os.environ, LD_LIBRARY_PATH=f"{build}/Release:{build}", **env_extra)
    proc = subprocess.run(args, cwd=build, env=env, capture_output=True, text=True, timeout=120)
    values = [l.split()[-1][2:].upper().zfill(8) for l in proc.stdout.splitlines() if l.startswith("[PEEK32]")]
    return proc, values


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--recomp", required=True, help="path to dc_raw_recomp")
    ap.add_argument("--prefix", default="sh4-linux-gnu-")
    ap.add_argument("--keep", action="store_true")
    ns = ap.parse_args()
    tmp = pathlib.Path(tempfile.mkdtemp(prefix="dcr_e2e_"))
    try:
        stub = tmp / "stub.S"
        stub.write_text(STUB.format(out=OUT))
        objs = []
        for src in (SRC / "interp_corpus.c", SRC / "sh4_freestanding.c", stub):
            obj = tmp / (src.stem + ".o")
            run([ns.prefix + "gcc", *(CFLAGS if src.suffix == ".c" else ["-ml", "-m4"]), "-c", str(src), "-o", str(obj)])
            objs.append(str(obj))
        elf = tmp / "e2e.elf"
        run([ns.prefix + "gcc", "-nostdlib", "-static", "-no-pie", "-Wl,--build-id=none", "-Wl,-z,noexecstack",
             f"-Wl,-Ttext={BASE:#x}", "-Wl,-e,e2e_start", *objs, "-lgcc", "-o", str(elf)])
        image = tmp / "e2e.bin"
        run([ns.prefix + "objcopy", "-O", "binary", "-R", ".eh_frame", "-R", ".comment", str(elf), str(image)])
        syms = nm(ns.prefix, elf)
        seeds = tmp / "aot_seeds.txt"
        seeds.write_text("".join(f"{a:#010x}\n" for n, a in syms.items() if n not in POINTER_ONLY))
        expected = host_expected(tmp)

        def build_guest(extra_seed_files):
            gen = tmp / "gen"
            shutil.rmtree(gen, ignore_errors=True)
            cmd = [ns.recomp, str(image), f"--base={BASE:#x}", f"--entry={syms['e2e_start']:#x}",
                   f"--seed-file={seeds}", f"--output={gen}", f"--map={gen / 'function_map.csv'}"]
            cmd += [f"--seed-file={s}" for s in extra_seed_files]
            run(cmd, stdout=subprocess.DEVNULL)
            run([sys.executable, str(ROOT / "tools/modular/modularize_generated.py"), str(gen),
                 "--runtime-root", str(ROOT / "runtime")], stdout=subprocess.DEVNULL)
            build = tmp / "build"
            run(["cmake", "-S", str(gen), "-B", str(build), "-DCMAKE_BUILD_TYPE=Release",
                 f"-DDCR_RUNTIME_ROOT={ROOT / 'runtime'}"], stdout=subprocess.DEVNULL)
            run(["cmake", "--build", str(build), "--target", "dreamcast_program", "--parallel"], stdout=subprocess.DEVNULL)
            return build

        failures = []
        build = build_guest([])
        proc, _ = guest_run(build, {"DCR_SH4_INTERP": "0"})
        if "No recompiled/native target registered" not in proc.stdout + proc.stderr:
            failures.append("interpreter OFF did not reproduce the historical missing-target abort")
        learned = tmp / "interp_seeds.txt"
        proc, got = guest_run(build, {"DCR_SH4_INTERP_SEEDS": str(learned),
                                      "DCR_EXTERNAL_RAW_IMAGE": "", "DCR_SH4_INTERP": "1"})
        if got != expected:
            failures.append(f"interpreter ON: results differ\n  expected {expected}\n  got      {got}")
        print("[E2E] interpreter ON :", [l for l in proc.stderr.splitlines() if "summary" in l])
        # The embedded-image runner has no boot-image reference, so entries are
        # reported as runtime-code. Promote them for this check exactly like a
        # portable (BOOTSTRAP.BIN) run would have written them.
        promoted = tmp / "learned_seeds.txt"
        entries = [l.split()[2] for l in learned.read_text().splitlines() if l.startswith("# runtime-code 0x")]
        promoted.write_text("".join(e + "\n" for e in entries))
        if not entries:
            failures.append("no interpreted entrypoints were recorded")
        build = build_guest([promoted])
        proc, got = guest_run(build, {})
        if got != expected:
            failures.append("after learning: results differ")
        if "SH4-INTERP" in proc.stderr:
            failures.append("after learning: interpreter still used\n" + proc.stderr)
        if failures:
            print("[E2E] FAIL\n" + "\n".join(failures))
            return 1
        print(f"[E2E] PASS: AOT abort without interpreter, {len(entries)} entries interpreted with "
              f"bit-exact results, 0 after re-seeding")
        return 0
    finally:
        if not ns.keep:
            shutil.rmtree(tmp, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
