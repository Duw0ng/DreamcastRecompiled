# Holly / Maple completion event — 0.0.40

The Maple payload path remains synchronous in 0.0.40, but DMA completion is no longer invisible to the System ASIC model.

KallistiOS defines `ASIC_EVT_MAPLE_DMA` as event `0x000c`: register bank A, bit 12. The generated runtime now models the three Holly pending/acknowledge registers at:

```text
0xA05F6900  ACK/PENDING A
0xA05F6904  ACK/PENDING B
0xA05F6908  ACK/PENDING C
```

After a low-level Maple DMA descriptor chain completes:

```text
Holly pending A |= (1 << 12)
```

Writing that bit back to `0xA05F6900` acknowledges and clears it.

The generated standalone regression now performs a real controller `GETCOND`, verifies active-low controller data, verifies bit 12 becomes pending, writes the acknowledge bit and verifies it clears.

## Current boundary

This is **event-pending/ack plumbing**, not full asynchronous SH-4 interrupt entry yet. Delivering the pending Holly event through mask registers, SH-4 interrupt priority, context save/restore and the guest IRQ handler is the next layer.
