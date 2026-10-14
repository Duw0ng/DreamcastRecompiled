#!/usr/bin/env python3
from pathlib import Path
import subprocess, sys
root=Path(__file__).resolve().parents[3]
checks=[]
def check(name, cond):
    checks.append((name,bool(cond))); print(("PASS" if cond else "FAIL")+": "+name)
def txt(p): return p.read_text(encoding="utf-8-sig",errors="replace")
rt=txt(root/"runtime/dc_runtime.cpp"); em=txt(root/"src/codegen/cpp_emitter.cpp"); run=txt(root/"run_game.bat"); go=txt(root/"portable/launcher/main_windows.go")
check("runtime build id", "v1.2-cpp38c-spg-wait-fast-forward" in rt)
check("SPG remains guest-cycle sampled", "pvr_spg_sample(runtime, now_cycles)" in rt)
check("same-PC same-value detector", "pc == ff.last_pc && spg.status == ff.last_status" in rt)
check("few-cycle guard", "delta <= 256u" in rt)
check("one-scanline skip", "line_cycles - in_line" in rt)
check("render-done clamp", "pvr_render_done_due_cycle" in rt and "std::min(skip, runtime.pvr_render_done_due_cycle - now_cycles)" in rt)
check("PVR/VBlank clamp", "device_clock_pvr_target_cycles" in rt and "std::min(skip, runtime.device_clock_pvr_target_cycles - dev_now)" in rt)
check("Holly pending blocks skip", "runtime.holly_pending[0] | runtime.holly_pending[1] | runtime.holly_pending[2]" in rt)
check("heartbeat telemetry", "spg-ff=" in rt and "last_hit_pc" in rt)
check("emitter mirrors SPG fast-forward", "pvr_spg_maybe_fast_forward_wait" in em and "spg-ff=" in em)
check("run_game default on", "DCR_SPG_WAIT_FF=!SPG_WAIT_FF!" in run and 'set "SPG_WAIT_FF=1"' in run)
check("A/B switches", "--spg-ff-on" in run and "--spg-ff-off" in run)
check("cpp38b2 guest impl retained", "DCR_GUEST_IMPL_REV=cpp38b2-metricfree-v1" in run)
check("portable cpp38c + SPG FF", "DCR_BUILD_SHORT=cpp38c" in go and "DCR_SPG_WAIT_FF=1" in go)
proc=subprocess.run([sys.executable,str(root/"tools/modular/guest_abi_fingerprint.py"),str(root/"runtime/include/dc_runtime.hpp"),str(root/"runtime/guest_abi/v1/dc_runtime.hpp")],capture_output=True,text=True)
check("ABI1 structural fingerprint MATCH", proc.returncode==0 and "MATCH" in proc.stdout and "00ea21b77c60d564cf2c455c9c9d870892047aea3c3a49f28bcc844b6e04054f" in proc.stdout)
bad=[n for n,ok in checks if not ok]
print(f"\n{len(checks)-len(bad)}/{len(checks)} checks passed")
if bad: print("FAILED: "+", ".join(bad)); raise SystemExit(1)
