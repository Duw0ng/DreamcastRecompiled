# DreamcastRecomp 0.0.78 — validation

## Windows target

The 0.0.77 TYPE7 geometry probe proved that removing texture sampling turns submitted surfaces white, while the stage preview / gameplay board itself still has no corresponding white geometry. The same run reports a very large Parameter Type 2 / OBJECT LIST SET count, so 0.0.78 instruments TA framing by submit source before applying further rendering guesses.

## New diagnostics

- `pvr-pt=a,b,c,d,e,f,g,h`: decoded TA parameter-type histogram for packets that are not 64-byte continuation halves.
- `pvr-objsrc=sq,ch2,cpu,unknown`: source histogram for packets decoded as OBJECT LIST SET.
- `[PVR OBJSET]`: bounded raw dumps at counts 1, 64, 4096 and 262144, including source PC and PCW interpreted as float.
- `pvr-badv=total/nonfinite/extreme/probe-accepted`.
- `pvr-boardprobe=on`: all non-background submitted geometry is solid white and bypasses texture, depth, blending and USER TILE CLIP.

## Closure

A canonical `RTS; NOP` callback is executable evidence even if its 4-byte body is immediately followed by a dense in-image pointer table. The pointer-table heuristic now yields to this architectural function form without title-address seeds.

## Background depth correction

The software PVR now sources the background-plane Z from `ISP_BACKGND_D` (0x88). This removes the previous `-FLT_MAX` sentinel which made LESS/LEQUAL geometry impossible against an otherwise valid background plane.
