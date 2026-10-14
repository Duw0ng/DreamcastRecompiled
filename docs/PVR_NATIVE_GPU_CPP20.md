# PVR native-GPU checkpoint (cpp20)

This checkpoint moves the existing Direct3D 11 PVR path further away from CPU raster/translation work and toward a native host-GPU renderer. The Dreamcast PVR/TA semantics still have to be modeled, but the expensive host rendering work is increasingly performed directly by D3D11.

## Scope

The implementation follows the seven-step plan selected for cpp20. Vulkan is intentionally **not** implemented in this checkpoint. DX9, DX10 and DX12 are also outside the active roadmap for this stage.

### 1. GPU texture decode

The deferred PVR state can now retain immutable raw Dreamcast texture bytes (and palette snapshots) instead of pre-decoding every GPU texture into CPU BGRA pixels.

On D3D feature level 11, a compute shader decodes supported Dreamcast formats into a GPU `R8G8B8A8_UNORM` texture. The current compute path covers:

- ARGB1555
- RGB565
- ARGB4444
- YUV422/UYVY
- PAL4
- PAL8
- twiddled textures
- rectangular twiddle
- non-twiddled/stride textures
- VQ textures
- mip-base offsets used by the existing translator

If the compute path is unavailable or an actual GPU failure requires software replay, CPU texture materialization remains as a compatibility fallback.

### 2. Direct TA vertex upload

`PVRSoftVertex` is now the GPU vertex layout used by the D3D11 input layout. The old per-frame `PVRSoftVertex -> PVRGpuVertex` conversion scratch array is removed.

The D3D11 renderer uploads the deferred TA vertex arena directly. Extra provenance fields present in the host struct are simply not referenced by the D3D input layout.

### 3. Persistent vertex/index streaming

The D3D11 backend now rotates through a three-slot VB/IB stream ring. Resources are grown only when capacity is insufficient and are otherwise reused with `D3D11_MAP_WRITE_DISCARD`.

This reduces resource churn and avoids forcing every frame through one single streaming buffer.

### 4. GPU-resident render-to-texture

A PVR RTT cache now retains compatible render targets as GPU resources. A completed RTT is copied GPU-to-GPU rather than immediately performing a staging readback and RGB565 CPU writeback.

Compatible non-twiddled RGB565 sampling can bind the resident RTT SRV directly. CPU synchronization is lazy: if the SH-4 later reads the corresponding VRAM range, the runtime performs a staging readback, quantizes into Dreamcast RGB565 VRAM and marks the entry synchronized. Guest VRAM writes invalidate overlapping RTT entries.

Current limitation: direct GPU re-sampling preserves the host BGRA8 render-target precision until a CPU-visible synchronization occurs. A future GPU resolve/quantization pass can make the resident path bit-closer to the physical RGB565 storage without introducing a CPU round trip.

### 5. PVR pipeline/state cache

D3D11 rasterizer, blend, depth and sampler combinations are cached behind a compact PVR pipeline key. Draw runs request one cached `PVRGpuPipelineState` instead of independently rebuilding state choices every time.

The shader programs are also initialized once with the backend, including the texture-decode compute shader.

### 6. Wider GPU fast path

The GPU frame path no longer requires an already CPU-decoded texture snapshot. A raw GPU-decode snapshot is sufficient.

Negative inverse-W values no longer automatically reject an entire frame; the existing D3D11 depth mapping clamps the value for host depth output. CPU-decoded texture snapshots are only materialized if a real fallback occurs.

The goal is to make `pvr_gpu_fallback_frames` and `pvr_gpu_fallback_materializations` approach zero on compatible content rather than replaying a whole frame in software for one avoidable condition.

### 7. `PVRRendererBackend`

The PVR/TA translator now calls a renderer interface rather than calling the D3D11 implementation directly:

```text
PVR/TA translation
       |
       v
PVRRendererBackend
       |
       +-- PVRD3D11RendererBackend   (cpp20)
       +-- Vulkan backend            (future, after DX11 validation)
```

The backend surface currently owns render, page-flip, presentation and explicit readback operations. Dreamcast list/state/texture semantics remain above that boundary so a future Vulkan backend does not have to duplicate the whole PVR frontend.

## Native-GPU telemetry

The heartbeat now includes:

```text
pvr-gnative=compute/texels/avoided/pipeline_hits/pipeline_misses/rtt_commits/lazy_readbacks/fallback_materializations
```

Interpretation:

1. `compute`: GPU texture decode dispatches.
2. `texels`: texels scheduled for GPU decode.
3. `avoided`: CPU texture decodes avoided by raw GPU snapshots.
4. `pipeline_hits`: PVR pipeline-cache hits.
5. `pipeline_misses`: new cached PVR pipeline states.
6. `rtt_commits`: render targets kept GPU-resident.
7. `lazy_readbacks`: RTTs synchronized back to Dreamcast VRAM because CPU/SH-4 access required it.
8. `fallback_materializations`: deferred CPU texture materializations caused by a true GPU fallback.

Existing telemetry remains useful:

- `pvr-gpu=...` for GPU frames/fallback/timing/readback.
- `pvr-gtex=...` for texture upload/reuse/unsupported counts.
- `pvr-gpresent=...` for direct presentation/readback mode.

## Windows validation

Run a known title with:

```bat
run_game.bat "C:\Games\game.cdi" --renderer=dx11
```

For performance comparisons, use the same title, scene, resolution, build type and input path. Compare against `--renderer=software` only when the title has a known-correct software baseline.

A healthy FL11 run should show:

- `pvr_gpu_frames` rising continuously;
- `pvr_gpu_fallback_frames` close to zero;
- `pvr-gnative` compute/avoided counters increasing on textured scenes;
- pipeline hits quickly dominating misses;
- RTT commits increasing on RTT-heavy content with lazy readbacks staying much lower;
- fallback materializations staying at or near zero.

Do not infer a performance win from the architecture alone; record Windows FPS/frame timing and visual correctness. The Linux validation environment used for this checkpoint can compile and test generated non-Windows code, but it cannot execute the `_WIN32` D3D11 device path because the Windows SDK/runtime is not present.

## Validation status

- Core/non-generated test set: **52/52 PASS**.
- Generated single-function build test: **PASS**.
- Generated multi-function program build test: **PASS**.
- Total logical project tests: **54/54 PASS**.
- Latest generated runtime contains the compute decoder, triple VB/IB ring, GPU RTT cache, pipeline cache, native telemetry and `PVRRendererBackend` boundary.
- Real D3D11 execution still requires a Windows test machine.
