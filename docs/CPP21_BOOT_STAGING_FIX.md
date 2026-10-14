# cpp21 — Commercial boot staging regression fix

## Regression
cpp20 always restaged the commercial executable into Dreamcast's scrambled 32-byte layout before IP.BIN execution. That is correct for raw/self-boot-like layouts used by the SGGG test image, but wrong for cooked retail GD-ROM images whose boot executable is already plain executable data.

The Windows ChuChu Rocket! regression showed:
- physical sector: 2048 bytes
- user offset: 0
- boot size: 1,532,256 bytes
- cpp20 incorrectly logged `staged scrambled commercial executable`
- execution later faulted at guest PC 0x8C060D32 while reading 0x02100038, before any PVR frame was produced.

## cpp21 behavior
Commercial staging now defaults to `auto` and uses the boot track geometry already loaded from the disc map:

- `sector_size == 2048 && user_offset == 0` -> **plain/pass-through**
- raw layouts (for example 2336-byte sectors with a non-zero user offset) -> **scrambled staging**

The disc map is loaded during runner pre-scan so the geometry is available before `dc_setup_commercial_boot()`.

Diagnostic override:

- `--commercial-boot-stage=auto`
- `--commercial-boot-stage=plain`
- `--commercial-boot-stage=scrambled`

No title/game address whitelist is used.

## Validation
### ChuChu Rocket!
Using the real previously prepared ChuChu combined BOOTSTRAP.BIN and cooked 2048/0 disc geometry:

- generated closure: 5,118 functions / 506,900 known SH-4 instructions / RAW_SH4=0
- generated project build: PASS
- generated_compile_test: RC=0
- runtime selects `mode=plain`
- executable remains plain at 0x8C010000 (1,532,256 bytes)
- GD-ROM commands 0x1E and 0x1F execute
- the old 0x02100038 SH-4 memory fault does not occur
- bounded smoke continues through guest PC 0x8C0DA76E until controlled timeout

The full ChuChu CDI itself was not present in this Linux container, so this is a bootstrap/runtime regression test rather than a complete gameplay run. A Windows CDI retest remains the final acceptance test.

### SGGG
Using the full SGGG CDI available locally:

- physical sector: 2336
- user offset: 8
- runtime selects `mode=scrambled`
- the 3,106,103-byte executable is still staged scrambled
- bounded smoke did not report a SH-4 fault or Dreamcast runtime error

Thus cpp21 restores cooked-disc compatibility without removing the staging behavior required by SGGG.

## Test suite
- normal CTest entries: PASS
- generated_single_build: PASS
- generated_program_build: PASS
- aggregate logical suite: 54/54 PASS

## Relationship to cpp20 native-GPU work
cpp21 keeps the cpp20 seven-step native-GPU/PVR work. The regression fixed here is in commercial boot staging and occurs before the first PVR frame; it is not a rollback of GPU texture decode, persistent buffers, GPU-resident RTT, pipeline caching, expanded GPU fast path, or PVRRendererBackend.
