# Release notes — cpp38b1

This build intentionally backs out one cpp38b optimization rather than adding another speculative fast path. Live ChuChu A/B logs showed the direct D3D11 mapped-index construction path running slower than cpp38a despite almost identical PVR packets, triangles and GPU binds.

cpp38b1 therefore returns index generation to cacheable CPU memory and performs one sequential upload, while keeping the validated cpp38a TA/CH2 fast paths. It also restores the historical FPU superblock runtime mode, disables hot FPU metrics for normal runs, and retains the 262144-cycle host-sync cadence.

Use `TEST_CPP38B1_FAST.bat` for the recommended baseline. `TEST_CPP38B1_REGIONPLUS_AB.bat` and `TEST_CPP38B1_SYNC524_AB.bat` change one variable at a time for comparison.
