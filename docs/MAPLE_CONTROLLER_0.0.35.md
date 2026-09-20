# DreamcastRecomp 0.0.35 — Maple controller/input bootstrap

## Scope

This milestone introduces the first reusable Maple controller path. It intentionally implements both a low-level packet model and a temporary application-facing KallistiOS compatibility bridge so interactive homebrew can be tested before the complete Maple/Holly interrupt pipeline exists.

## Raw controller model

The runtime exposes a standard controller on port A, unit 0. `DEVINFO` advertises `MAPLE_FUNC_CONTROLLER`; `GETCOND` returns the standard raw condition layout:

```text
uint16 buttons   active-low
uint8  rtrig
uint8  ltrig
uint8  joyx      128=center
uint8  joyy      128=center
uint8  joy2x
uint8  joy2y
```

The cooked runtime state uses the KallistiOS convention (pressed digital bits are 1, centered axes are zero), and packet generation converts it back to the hardware active-low/unsigned representation.

## DMA bootstrap

The Maple MMIO window recognizes DMA address, enable, state/trigger and backing control registers. Starting DMA walks KallistiOS-style descriptors, writes responses to guest memory and returns the state register to idle. The generated smoke test issues an actual `GETCOND` descriptor and verifies the response packet.

This milestone completes DMA synchronously. **Holly Maple-DMA completion IRQ delivery is not implemented yet.** Therefore the raw transport is useful and testable, but the entire KOS Maple scheduler/IRQ stack is not yet claimed to run unmodified.

## KOS host bridge

`--maple-host-input` registers native overrides for common enumeration/status helpers and fabricates guest-memory-compatible controller/device state. This is the practical bridge used by current interactive tests while low-level completion interrupts are developed.

## Windows mappings

Keyboard:

- Arrow keys: D-pad
- J or Space: A
- K: B
- U: X
- I: Y
- Enter: Start
- Q / E: left / right trigger
- W A S D: analog stick

XInput is loaded dynamically (`xinput1_4`, `xinput1_3`, or `xinput9_1_0`) when available. Standard face buttons, D-pad, triggers and left stick map to the Dreamcast controller; shoulder buttons currently provide C/Z for homebrew that advertises extended controls.

## Next Maple work

1. Holly DMA-completion IRQ.
2. Asynchronous frame completion and VBlank-triggered DMA.
3. Remove the KOS API bridge once real driver enumeration/polling progresses end-to-end.
4. VMU and rumble after the standard controller path is stable.
