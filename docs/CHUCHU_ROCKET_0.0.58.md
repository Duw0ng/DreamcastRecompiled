# ChuChu Rocket! acceptance — 0.0.59

The save-slot confirmation no longer enters a device-less backup transition by design. DreamcastRecomp now exposes a persistent VMU at A1 and implements the storage Maple commands needed by the Katana backup path.

Expected Windows test: START -> memory-card screen -> select A1 -> A. The next log should show non-zero `vmu=A1/devinfo/minfo/read/write/sync` counters as the title enumerates and accesses the card.
