# DreamcastRecomp v1.2 cpp37b — GDROM_COMMAND_DMA_NATIVE

This checkpoint builds directly on the user-validated cpp37a GD-ROM media plugin.

The native GD-ROM plugin now owns command lifecycle state, multi-transfer state and G1/GD DMA transfer/IRQ handling. The Runtime retains a compatibility mirror solely because CDDA remains there until cpp37c. No field was added to `DCRuntime`, and Guest ABI1 stays frozen.

The user-visible naming is also simplified. The old generated runner can still be reused, but Runtime replaces its historical window title with `DreamcastRecompiled - <game> [cpp37b]`. The portable launcher is v0.9 and uses the same short naming convention.

First cpp37b `--reuse` run rebuilds Runtime/plugins due to the new runtime build ID, but does not recompile the SAFE SH-4 guest.
