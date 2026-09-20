# DreamcastRecomp 0.0.84 validation

## Runtime issue addressed

After the 0.0.83 Store Queue/FMOV64 correction produced visually correct ChuChu Rocket gameplay, completing a timed 4P Battle round reached an unregistered native target:

- join/call site: indirect `JSR @R3` in the result path around `0x8C0E43A6`
- runtime target observed on the taken predecessor: `0x8C0E43EC`

The code shape loads callback A into R3, executes a delayed conditional branch that can skip a second literal load, and then joins at one `JSR @R3`. Straight-line resolution sees only the fall-through callback B. 0.0.84 records both predecessor literal values as branch-selected call targets and promotes validated executable entries into the raw closure.

## Static commercial validation

Using the supplied ChuChu Rocket CDI:

- closure passes: 23
- reachable functions: 2,965
- reachable instructions: 295,215
- known SH-4: 295,215
- unknown SH-4: 0
- RAW_SH4: 0
- branch-selected newly promoted targets: 1
- `0x8C0E43EC`: present in generated function map
- alternate fall-through target `0x8C0E4688`: remains present

No manual seed for either result-path callback is required.

## Regression suite

47/47 CTest tests pass. A new function-analysis fixture models:

```text
MOV.L callback_a,R3
BF/S  join
 NOP
MOV.L callback_b,R3
join:
JSR @R3
 NOP
```

The normal call resolver retains callback B while branch-selected-call analysis also records callback A and B as possible targets.
