# cpp36a — MAPLE DEVICE TREE

cpp36a promotes `dcr_maple` from the cpp35b single A0/A1 state into a port-aware Maple bus without changing Guest ABI1.

## Device tree

```text
MapleBus
├─ Port A: Controller + Slot 1 VMU + Slot 2 Empty
├─ Port B: Controller(auto host #1) + Slot 1/2 Empty
├─ Port C: Controller(auto host #2) + Slot 1/2 Empty
└─ Port D: Controller(auto host #3) + Slot 1/2 Empty
```

Port A preserves the existing profile/calibration path. B/C/D auto-map host controller indices 1/2/3. Expansion slots are represented explicitly by the V3 control API so PuruPuru and other devices can be added without another DMA/bus redesign. The VMU storage implementation remains A1 in cpp36a.

## Maple Control V3

`dcr.maple.control.v3` adds port-aware controller state, two expansion slots, trace control and device-tree inspection. `dcr.maple.control.v2` remains available as the Port-A compatibility view.

## Trace

Set:

```text
DCR_MAPLE_TRACE=1
```

or run:

```bat
run_game.bat "game.cdi" --reuse --maple-trace
```

Trace is off by default and reports DMA routing such as port/unit/device/cmd without changing guest code.

## Cache rule

Guest ABI1 is unchanged. `dcr_game_module.dll` is reused. `--rebuild-maple` builds only `dcr_maple` through the cpp35b1 separated runtime build.
