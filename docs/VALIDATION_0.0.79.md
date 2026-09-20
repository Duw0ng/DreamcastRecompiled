# DreamcastRecomp 0.0.81 — validation

## Target

The 0.0.78 ChuChu Rocket! board probe showed that forcing every submitted primitive white did not reveal the board or rotating star-background object. It also decoded very few sprites relative to the amount of 2D activity on screen, while tens of millions of 32-byte bursts appeared as Parameter Type 2 / OBJECT LIST SET. Visible mice were sparse and inherited the orange-monster texture.

## Root cause fixed in this checkpoint

The software TA parser treated Sprite Global Parameters as one-shot state. A Dreamcast sprite vertex is 64 bytes (two 32-byte TA transfers), and a single Sprite Global Parameter can remain active for multiple following Sprite Vertex Parameters. 0.0.78 cleared sprite collection after one 64-byte vertex, allowing later sprite payload halves to be interpreted as independent PCWs.

0.0.81 keeps persistent sprite mode until a new global parameter or EOL, and consumes every second 32-byte sprite half as payload unconditionally. It also refuses vertices in the no-list state instead of decoding them with stale polygon/texture state.

## Diagnostic counters

- `pvr-spr=drawn/headers/vertex64/reused-header`
- `pvr-stail=t0,t1,t2,t3,t4,t5,t6,t7` — what each sprite B-half *would* have looked like if incorrectly parsed as a PCW. A large t2 value directly measures how much of the old OBJECT LIST SET count was sprite payload.
- `pvr-tafsm=polygon-headers/vertices-without-header/invalid-params`
- `pvr-strip=starts/eos/completed/short/abandoned`
- Existing `pvr-objset`, `pvr-pt`, `pvr-pktsrc`, `pvr-objsrc`, `pvr-vt`, `pvr-tex`, `pvr-tcache` remain available.

## Visual validation order

1. Normal runner: Stage Select preview, then 4P Battle.
2. Check whether board tiles appear, mouse count increases, mice use their own texture, and the rotating star object becomes visible.
3. Capture at least one heartbeat from active gameplay.
4. If major geometry is still missing, use BOARD-GEOMETRY. 0.0.81 filters extreme finite vertices again, so the probe should no longer be obscured by the giant white corruption shapes accepted intentionally in 0.0.78.

## Build / closure regression

- Core Linux build: PASS (GCC 14.2).
- CTest: 47/47 PASS.
- Supplied ChuChu Rocket! CDI closure: 2,964 functions, 294,910 reachable/known SH-4 instructions, 0 unknown, `RAW_SH4=0`.
- Generated commercial `dc_runtime.cpp`: compiled successfully. The enormous generated program TU was not used as a visual/runtime validation on this Linux host; final behavior still requires the Windows gameplay test.

## Dedicated sprite-state smoke

A generated-runtime smoke test submitted one Sprite Global Parameter followed by two independent 64-byte Sprite Vertex Parameters. Both B-halves deliberately began with float values whose top bits would decode as Parameter Type 2 if treated as PCWs. Result:

```text
sprites=2 headers=1 vertex64=2 reused=1 objset=0 stail2=2
```

This verifies the 0.0.81 state change itself: the second sprite reuses the active header, continuation payload is measured by `pvr-stail` but is not counted as OBJECT LIST SET, and both sprites reach `pvr_submit_sprite64`.
