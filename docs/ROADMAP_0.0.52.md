# DreamcastRecomp roadmap after 0.0.52

## Immediate 0.0.53 goals

1. Validate 0.0.52 on Windows through Sonic Team and the title screen.
2. Press Enter/START and follow the first real menu/input boundary.
3. Characterize the title-screen black texture rectangles from the actual TA headers, TCW/TSP state and texture memory rather than adding image-specific masks.
4. Extend texture alpha/addressing/blend support only from observed PVR state.
5. Keep symbol-free commercial closure at 0 unknown / `RAW_SH4=0` and reject false data promotion.
6. Continue reducing interpreted ARM7 cost without sacrificing SH-4/PVR/UI responsiveness.
