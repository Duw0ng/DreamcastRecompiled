# DreamcastRecomp v1.2 cpp34b — MODULAR_FULL_ISP_ABI1

This build supersedes cpp34a for the first modular ChuChu Rocket network test.

## Why cpp33c still failed

The cpp33c log reached a valid dual ISP partition but remained at `m0/c0/rw0,0/ppp0`: the retail title never touched the modem. The flash counters climbed to roughly 1,000,007 INFO calls and 4,000,029 READ calls, indicating a configuration-validation loop.

The previous PlanetWeb seed was incomplete: it had 0x80/0x82/0x83/0x84 and C0/C1/C2, but omitted documented 0x81 and C3/C4/C5 records. cpp34b creates a complete browser profile and also populates the generic Katana ISP records so titles using either parser can pass configuration detection.

## Modular behavior

Guest ABI remains version 1. Once a game is compiled for cpp34, later network work does not require rebuilding its generated SH-4 closure. `--rebuild-network` rebuilds only the external network plugin.

## Validation

- Core build: PASS.
- Tests: 54/54 PASS.
- Generated runtime/program build: PASS.
- Modular generated fixture: PASS.
- Network-only rebuild: 2 steps (plugin compile + link), generated SH-4 compile count: 0.
