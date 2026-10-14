# Validation — cpp38c runtime-only launcher fix

- `VERIFY_CPP38C.py`: **15/15 PASS**.
- Runtime/SPG source tree is byte-identical to the previously validated cpp38c package.
- Guest ABI v1 header is unchanged.
- Cached cpp38b2 migration block was removed from the existing-cache path.
- `TEST_CPP38C_FAST.bat` and `TEST_CPP38C_SPG_FF_OFF.bat` force `--reuse` and disable portable sync during the A/B test.
- Under `--reuse`, missing/invalid/contaminated guest cache aborts without deleting or compiling the guest.
- Under `--reuse`, `dc_raw_recomp` is not required and is not built.
- `generated/cpp` is never patched/configured/built from the cached-guest path.
- Only DreamcastRuntime/plugins may rebuild once for the cpp38c runtime build id.
