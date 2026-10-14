# SGGG cpp16 checkpoint

This checkpoint extends cpp15 with generic stored-callback discovery for strong leaf-style SH-4 functions.

## New cpp16 analyzer fix

A stored callback may be a large leaf worker that never saves PR because it never calls another function. The previous stored-callback validator required an early PR save and therefore rejected SGGG target `0x8C212C58`, even though the callback pointer is explicitly loaded by the object initializer around `0x8C1F2DE4` and stored unchanged into the object at `R4+12`.

The new rule is deliberately narrow and only applies after stored-callback provenance has already been proven:

- the first instruction must push one of R8-R14 to R15,
- at least four distinct callee-saved GPRs must be pushed within the first eight halfwords,
- no PR save is required for this leaf-style shape,
- early control transfers are rejected,
- the ordinary generic raw-data entry detector is unchanged.

SGGG `0x8C212C58` pushes R14/R13/R12/R11/R10/R9/R8 and is therefore a very strong match.

## Validation

- Full project CTest: 54/54 PASS.
- Closure generated without a `0x8C212C58` manual seed.
- Closure converged at 6,384 reachable functions.
- Reachable SH-4 instructions: 676,866.
- Known SH-4: 676,866.
- Unknown SH-4: 0.
- RAW_SH4: 0.
- `0x8C212C58` is present as its own native function-map row:
  `0x8C212C58,sub_8C212C58,369,369,0,23,0,0,0`

The complete function map, address-taken evidence map, and closure summary are included in this checkpoint under `docs/SGGG_CPP16_*`.

## Previously preserved fixes included

- Correct retail restaging: IP.BIN remains intact; only the main executable at `0x8C010000` is restaged/scrambled.
- Commercial IP.BIN static PC-relative literal handling after low-RAM reuse.
- Low-RAM AOT overwrite detection and dynamic rebind to matching relocation templates (VBR/RTE fix).
- Stored-callback register tracking window extended to 0x20 bytes.
- Valid runtime seed `0x8C181BF8` persisted in the SGGG profile.
- BIOS Flash INFO/READ HLE from cpp11.
- Optional late START Maple GETCOND diagnostic from cpp11.

## Next runtime milestone

Compile the emitted 6,384-function runtime and verify that the visual path passes the former ~884-frame failure at `0x8C212C58`, then continue toward the historical 2,566-frame milestone and TITLE/MENU.
