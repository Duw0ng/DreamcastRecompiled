# DreamcastRecomp roadmap — after 0.0.51

## 0.0.51 checkpoint

ChuChu Rocket! now passes the post-SEGA timed wait that froze 0.0.50 at 5,185 TA packets / 144 frames. The SH-4 TMU down-counter is live, PVR resumes repeated guest-driven scenes, and the same route passes GD-ROM PLAY_SECTORS without leaving CDDA permanently in PLAY.

## 0.0.52 target

Primary target: run beyond the current >8,600-packet / >449-frame checkpoint and classify the first new visible or hardware boundary rather than expanding subsystems speculatively.

Priority order:

1. capture the first post-SEGA visible screen produced after the TMU wait;
2. validate framebuffer/page-flip and texture/blend correctness on that screen;
3. validate Maple input as soon as an interactive menu is reached;
4. implement TMU underflow interrupt delivery if a concrete title path begins depending on `TCR.UNIE/UNF`;
5. add external TMU clock sources only when observed;
6. continue GD-ROM/CDDA semantics from the next real request;
7. improve ARM7 execution speed so `host-aica-drop` can fall without starving the host.

Static closure remains a non-goal unless a new valid indirect target appears: the current commercial closure is already 2,194 functions / 165,555 known SH-4 / 0 unknown / RAW=0.
