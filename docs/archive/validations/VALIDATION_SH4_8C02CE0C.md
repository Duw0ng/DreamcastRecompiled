# SH-4 closure hotfix — 0x8C02CE0C

Date: 2026-09-17

## Reported runtime failure

`Error: falta el destino SH-4 0x8C02CE0C, llamado desde 0x8C0190A4.`

The caller uses a nested runtime method table. The missing target is a compact
literal-fed tail veneer:

- entry: `0x8C02CE0C`
- shape: `MOV.L @(disp,PC),Rn` -> `JMP @Rn` -> valid delay slot
- static method row: `0x8C02CD80`, `0x8C02CE0C`, `0x8C02CDC6`
- the two sibling methods were already proven closure entries.

## Fix

`anchored_dense_callback_table_targets()` now also recognizes short, aligned
three-method SDK rows when at least two members are already proven executable.

The missing third member is promoted only if:

1. it is local and not probable ASCII/pointer-table data;
2. it matches the exact literal-fed `MOV.L -> JMP @Rn` veneer shape;
3. the delay slot decodes as valid SH-4;
4. the literal tail destination is already a known closure entry or independently
   passes the dense callable predicate.

No game-specific address whitelist or manual seed is used.

## Targeted validation

Using the ChuChu Rocket commercial BOOTSTRAP.BIN:

- closure passes: 12
- synthetic/reachable functions: 4,980
- reachable instructions: 480,189
- known SH-4: 480,189
- unknown SH-4: 0
- RAW_SH4: 0
- anchored dense callbacks: 70
- `0x8C02CE0C` present in `function_map.csv`: YES

Result: the reported missing-target failure is resolved at SH-4 closure level.
