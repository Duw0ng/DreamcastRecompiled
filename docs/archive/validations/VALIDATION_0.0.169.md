# DreamcastRecomp 0.0.170 validation

Compatibility-only change over 0.0.168.

## Exact synthetic regression

A `.raw_boot` fixture mapped at P1 loads the address of a pointer cell through its P2 alias, dereferences the cell, then executes `JSR @R0`. The analyzer must resolve the final P2 code address while reading the cell bytes from the P1-backed raw image.

## Real Crazy Taxi 2 static audit

Using the user-supplied retail CDI only for local validation (not redistributed):

- Disc probe: Crazy Taxi 2, 2 tracks, boot FAD 11852, 1ST_READ.BIN 1,460,116 bytes.
- 0.0.168 closure did **not** register canonical P1 `0x8C00E1A0`.
- With the 0.0.170 P1/P2 file-backed resolver, `0x8C00E1A0` is present in the generated function map and analyzes as 51 known / 0 unknown instructions.
- This proves the original live crash class is fixed without a title-specific seed.

The broader Crazy Taxi 2 closure still contains many data-like `RAW_SH4` candidates from conservative symbol-free discovery; those are intentionally not papered over in this patch and remain useful compatibility telemetry for later versions.
