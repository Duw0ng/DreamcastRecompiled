# DreamcastRecomp 0.0.152 — validation

## Scope

0.0.152 is a focused PVR/TA performance build on top of 0.0.151. It preserves the TA Object-Control decoder lock and changes only the deferred opaque geometry representation needed to keep eligible D3D11 triangle strips compact, plus diagnostic correlation for BAD-VTX/short strips.

## Native strip invariant

For a complete staged 4-vertex non-translucent strip with GPU fastpath and CullMode 0/1, the opaque index stream must be:

```text
0, 1, 2, 3, FFFFFFFF
```

The logical triangle count remains 2. The old representation was six triangle-list indices (`0,1,2,1,2,3`). CullMode 2/3 and sorted translucent geometry remain triangle lists.

## Fallback invariant

`PVRDeferredOpaqueRun` carries topology. If the D3D11 frame later becomes unsupported, the multithread software renderer parses strip segments separated by `0xFFFFFFFF` and reconstructs the previous consecutive triangle sequence on demand.

## Diagnostics

- `pvr-stripgpu=runs/vertices/actual-indices/indices-saved/fallbacks/topology-switches`
- `pvr-shortbad=bad0,bad1,bad2,bad3plus/rejected-vertices`
- `[PVR SHORT-STRIP]` logs the first short strip containing rejected vertices.

## Regression

```text
CTest:                    50/50 PASS
Generated runtime build: PASS
Generated compile test:  exit 0
TA staging self-test:    PASS
```

Windows D3D11 behavior is intended for the supplied commercial runner test. The next ChuChu Rocket! session log should be used to measure real `pvr-stripgpu` coverage, index savings, topology switches and FPS in Mouse Mania.
