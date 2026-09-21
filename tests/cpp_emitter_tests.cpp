#include "dcrecomp/cfg.hpp"
#include "dcrecomp/cpp_emitter.hpp"
#include "dcrecomp/dcir.hpp"
#include "dcrecomp/elf32.hpp"
#include "dcrecomp/function_analysis.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

std::string read_all(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    std::ostringstream out;
    out << file.rdbuf();
    return out.str();
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: cpp_emitter_tests <literal_pool.elf> <output-dir>\n";
        return 2;
    }

    const auto elf = dcrecomp::load_elf32(argv[1]);
    const auto analysis = dcrecomp::analyze_function(elf, "_main");
    const auto cfg = dcrecomp::build_cfg(analysis);
    const auto ir = dcrecomp::lower_to_dcir(elf, analysis, cfg);

    const std::filesystem::path out_dir = argv[2];
    std::filesystem::remove_all(out_dir);
    const auto result = dcrecomp::emit_cpp(elf, ir, {out_dir, true, true, true});

    require(std::filesystem::exists(result.function_header), "function header generated");
    require(std::filesystem::exists(result.function_source), "function source generated");
    require(std::filesystem::exists(result.runtime_header), "runtime header generated");
    require(std::filesystem::exists(result.runtime_source), "runtime source generated");
    require(std::filesystem::exists(result.arm7_header), "standalone generated ARM7 header generated");
    require(std::filesystem::exists(result.arm7_source), "standalone generated ARM7 source generated");
    require(std::filesystem::exists(result.cmake_file), "generated CMakeLists exists");
    require(std::filesystem::exists(result.image_source), "embedded image source generated");
    require(std::filesystem::exists(result.native_source), "native override source generated");
    require(std::filesystem::exists(result.runner_source), "native runner generated");
    require(result.printf_address == 0x8C010040u, "_printf address resolved for native override");
    require(result.embedded_bytes > 0, "ELF data sections embedded");

    const auto source = read_all(result.function_source);
    require(source.find("#include <bit>") != std::string::npos, "generated program includes <bit> for std::bit_cast on MSVC");
    require(source.find("dc_load_pc_literal32(runtime, 0x8C010000u, 0x8C010014u, 0x8C010040u)") != std::string::npos, "printf literal keeps storage address for relocation");
    require(source.find("dc_load_pc_literal32(runtime, 0x8C010004u, 0x8C010018u, 0x8C020000u)") != std::string::npos, "rodata literal keeps storage address for relocation");
    require(source.find("dc_write32_hot(runtime, gpc_r15, gpc_pr)") != std::string::npos ||
            source.find("dc_guest_write32_hot(runtime, ctx.r[15], ctx.pr)") != std::string::npos, "PR push emitted");
    require(source.find("ctx.pr = 0x8C01000Au") != std::string::npos ||
            source.find("gpc_pr = 0x8C01000Au") != std::string::npos, "JSR return PR emitted");
    require(source.find("dc_call_dynamic_fast(ctx, runtime, delayed_target)") != std::string::npos, "runtime JSR target uses the 0.0.96 inline dynamic dispatcher");
    require(source.find("runtime.sh4_async_redirect || ctx.pc != 0x8C01000Au") != std::string::npos,
            "dynamic CALL propagates context-switch/non-local return PC");
    require(source.find("gpc_pr = dc_guest_read32_hot(runtime, gpc_r15)") != std::string::npos ||
            source.find("ctx.pr = dc_guest_read32_hot(runtime, ctx.r[15])") != std::string::npos, "PR pop emitted");
    require(source.find("ctx.pc = ctx.pr") != std::string::npos || source.find("ctx.pc = gpc_pr") != std::string::npos, "RTS emitted");
    require(source.find("static SH-4 integer register cache") == std::string::npos &&
            source.find("gpc_tick") == std::string::npos &&
            source.find("gpc_flush") == std::string::npos,
            "0.0.170 production codegen keeps the function-wide GPR cache out of the default path");
    require(source.find("dc_unimplemented") == std::string::npos, "hello _main has no unimplemented op");
    require(source.find("dc_runtime_tick_fast(ctx, runtime,") != std::string::npos,
            "0.0.170 default generated basic blocks retain the direct batched SH-4 fast tick");
    require(source.find("dc_runtime_tick(ctx, runtime,") == std::string::npos,
            "generated basic blocks no longer call the out-of-line legacy tick wrapper");

    const auto runtime = read_all(result.runtime_source);
    const auto runtime_header = read_all(result.runtime_header);
    require(runtime_header.find("#include <bit>") == std::string::npos,
            "low-memory runtime header no longer drags std::bit_cast FPU bodies into every shard");
    const auto runner = read_all(result.runner_source);
    const auto generated_cmake = read_all(result.cmake_file);
    require(runner.find("--perf-profile") != std::string::npos, "runner accepts low-overhead performance profiling");
    require(runner.find("--perf-sample-stride") != std::string::npos, "runner accepts profiling sample stride");
    require(runner.find("perf_sample_stride = 67u") != std::string::npos,
            "0.0.199 runner defaults profiler to prime stride 67");
    require(runtime_header.find("perf_sample_countdown{67u}") != std::string::npos &&
            runtime.find("--runtime.perf_sample_countdown") != std::string::npos,
            "0.0.199 profiler uses countdown sampling instead of modulo phase locking");
    require(runtime.find("perf_tick_index %") == std::string::npos,
            "0.0.199 full-tick profiler removes modulo sampling");
    require(runtime_header.find("DCR_SYNC_CURRENT_PC") != std::string::npos,
            "0.0.199 generated blocks support compile-time lightweight current-PC tracking");
    require(runner.find("--aica-play") != std::string::npos, "runner retains explicit host AICA playback option");
    require(runner.find("DCRSessionLogGuard") != std::string::npos, "0.0.154 runner installs automatic session log tee");
    require(runner.find("DreamcastRecomp_v0.1.1_session_") != std::string::npos, "v0.1.1 runner names timestamped session logs");
    require(runner.find("else if (pre == \"--ct2-compat\") runtime.ct2_compat = true") != std::string::npos,
            "v0.1.1 pre-scans CT2 compatibility before commercial bootstrap setup");
    require(runtime.find("staged scrambled commercial executable") != std::string::npos &&
            runtime.find("BootRand") != std::string::npos &&
            runtime.find("if (runtime.ct2_compat)") != std::string::npos,
            "v0.1.1 emits CT2 32-byte-slice commercial boot staging");

    require(runtime.find("const std::uint64_t raster = runtime.pvr_spg_status_reads++") == std::string::npos,
            "0.0.191 must not advance SPG raster from SPG_STATUS reads");
    require(runtime.find("pvr_spg_sample(runtime, now_cycles)") != std::string::npos,
            "0.0.191 samples SPG_STATUS from guest-cycle raster time");
    require(runtime.find("pvr_spg_frame_cycles(runtime)") != std::string::npos,
            "0.0.191 schedules PVR cadence from guest SPG timing");
    require(runtime.find("pvr_spg_resync(runtime)") != std::string::npos,
            "0.0.191 re-phases SPG on timing-register changes");
    require(runner.find("[DCR LOG] session-log=") != std::string::npos, "runner reports the saved session log path");
    require(runtime.find("0x1FFFFFFFu") != std::string::npos, "runtime maps SH-4 P1/P2 aliases");
    require(runtime.find("struct SH4MmuShadow") != std::string::npos &&
            runtime.find("sh4_translate_guest_address") != std::string::npos &&
            runtime.find("0xF6000000u") != std::string::npos &&
            runtime.find("0xF7000000u") != std::string::npos &&
            runtime.find("sh4-mmu=") != std::string::npos,
            "0.0.191 models MMUCR/UTLB translation for P0/P3 and reports it");
    require(runtime.find("[DCR SH4-FAULT]") != std::string::npos,
            "0.0.191 invalid memory accesses dump the current SH-4 register file");
    require(runtime_header.find("kBootRomPhysicalBase = 0x00000000u") != std::string::npos &&
            runtime_header.find("kBootRomLowSize = 0x00100000u") != std::string::npos,
            "0.0.170 maps the lower 1 MiB Dreamcast boot-ROM aperture");
    require(runtime.find("boot_rom_low_range") != std::string::npos &&
            runtime.find("runtime.boot_rom_low[i]") != std::string::npos,
            "0.0.170 accepts P1/P2 boot-ROM reads instead of faulting");
    require(runtime.find("DCR_BOOT_ROM") != std::string::npos &&
            runtime.find("dc_boot.bin") != std::string::npos,
            "0.0.170 optionally loads user-supplied boot ROM bytes without bundling firmware");
    require(runtime.find("bios-boot=") != std::string::npos,
            "0.0.170 heartbeat reports boot-ROM backing and reads");
    require(runtime.find("kBiosFontPhysicalBase") != std::string::npos, "runtime exposes the BIOS font ROM physical aperture");
    require(runtime.find("bios_font_range") != std::string::npos, "runtime maps BIOS-font P1/P2 aliases instead of faulting on the address returned by BIOS HLE");
    require(runtime.find("runtime.bios_font_rom[i]") != std::string::npos, "runtime reads the synthetic BIOS-font ROM backing");
    require(runtime.find("bios_font_ensure_range") != std::string::npos, "raw BIOS-font reads lazily synthesize compatible glyph data");
    require(runtime.find("MultiByteToWideChar(932u") != std::string::npos, "Windows BIOS-font synthesis maps compressed JIS slots through CP932");
    require(runtime.find("CreateDIBSection") != std::string::npos, "Windows BIOS-font synthesis rasterizes host glyphs without bundling BIOS/font files");
    require(runtime.find("bios_font_pack_wide") != std::string::npos, "synthetic Japanese glyphs are packed into Dreamcast 24x24 BIOS layout");
    require(runtime.find("bfont-rom=") != std::string::npos, "heartbeat reports raw BIOS-font activity and synthesized glyph counts");
    require(runtime_header.find("kBiosFontSize = 0x00100000u") != std::string::npos, "runtime maps the full upper 1 MiB BIOS/font region");
    require(runtime_header.find("kBiosFontDataOffset = 0x20u") != std::string::npos, "synthetic glyph layout starts at the FONTROM_ADDRESS payload offset");
    require(runtime.find("maple-last=") != std::string::npos, "heartbeat records the most recent low-level Maple request");
    require(runtime.find("runtime.targets.find(target)") != std::string::npos, "runtime target dispatcher emitted");
    require(runtime.find("canonical-p1=0x") != std::string::npos &&
            runtime.find("canonical-p2=0x") != std::string::npos &&
            runtime.find("target-bytes=") != std::string::npos &&
            runtime.find("nearest-native=0x") != std::string::npos,
            "0.0.170 missing-target diagnostics expose aliases, nearby native code and guest bytes");
    require(runtime.find("dc_relocate_pc_address") != std::string::npos, "runtime relocates MOVA addresses for copied code templates");
    require(runtime.find("dc_load_pc_literal32") != std::string::npos, "runtime reloads PC-relative literals from relocated code copies");
    require(runtime.find("active_relocation_source") != std::string::npos, "runtime tracks the active relocated template mapping");
    require(runtime.find("const std::uint32_t normal_return_pc = ctx.pr") != std::string::npos,
            "runtime preserves caller return PC across dispatch");
    require(runtime.find("ctx.pc = normal_return_pc") != std::string::npos,
            "native HLE unchanged-PC return is normalized without hiding non-local returns");
    require(runtime.find("pvr_sample_texture") != std::string::npos, "runtime contains basic PVR texture sampling");
    require(runtime.find("pvr_sample_texture_texel") != std::string::npos, "PVR sampler has an integer-texel hot path");
    require(runtime.find("pvr_prepare_texture_cache") != std::string::npos, "PVR rasterizer decodes reusable textures once per VRAM generation");
    require(runtime.find("pvr_vram_page_epoch") != std::string::npos, "PVR texture cache invalidation is scoped to touched VRAM pages");
    require(runtime.find("pvr-tcache=") != std::string::npos, "heartbeat reports decoded-texture cache reuse");
    require(runtime.find("pvr_texture_cache_fast_reuses") != std::string::npos, "PVR texture binding reuses the active decoded surface without a page scan");
    require(runtime.find("pvr_texture_page_slots") != std::string::npos, "0.0.141 tracks cached texture membership per VRAM page");
    require(runtime.find("pvr_texture_dirty_mask") != std::string::npos, "0.0.141 marks only overlapping cached texture slots dirty");
    require(runtime.find("dc_pvr_texture_dirty_region_selftest") != std::string::npos, "0.0.141 generated runtime exposes dirty-region equivalence self-test");
    require(runtime.find("pvr-tdirty=") != std::string::npos, "0.0.141 heartbeat reports dirty-region activity");
    require(runtime.find("pvr_bilerp_argb") != std::string::npos, "PVR bilinear filtering uses the fixed-point hot path");
    require(runtime.find("DCR_HAS_SSE2") != std::string::npos, "x64 generated runtime exposes the SSE2 bilinear implementation");
    require(runtime.find("pvr-tfast=") != std::string::npos, "heartbeat reports fast texture surface rebinds");
    require(runtime.find("pvr-detail=") != std::string::npos, "heartbeat reports whether pixel-level diagnostics are active");
    require(runtime.find(" | fps=") != std::string::npos, "heartbeat reports actual PVR page-flip throughput");
    require(runtime.find("!has_host_buffer && !guest_start") != std::string::npos &&
            runtime.find("kNoTaContext") != std::string::npos,
            "0.0.191 guest STARTRENDER completion is decoupled from host TA geometry availability");
    require(runtime.find("? std::min<std::uint64_t>(1500000u, 450000u + ta_bytes * 100u)") != std::string::npos &&
            runtime.find(": 4096u") != std::string::npos,
            "0.0.191 schedules Flycast-compatible 4096-cycle render-done when no TA context exists");
    require(runtime.find("if (has_host_buffer) runtime.pvr_render_completed = true") != std::string::npos,
            "0.0.191 empty render-done cannot page-flip a stale host raster buffer");
    require(runtime.find("[DCR PERF]") != std::string::npos, "runtime emits low-overhead subsystem profiler summary");
    require(runtime.find("[DCR PERF HOTPC]") != std::string::npos, "runtime emits sampled hot guest PCs");
    require(runtime_header.find("dc_call_dynamic_fast") != std::string::npos,
            "runtime header exposes the force-inlined dynamic dispatch cache-hit path");
    require(runtime_header.find("dc_read32_hot") != std::string::npos &&
            runtime_header.find("dc_write32_hot") != std::string::npos &&
            runtime_header.find("DCR_MEM_FORCE_INLINE") == std::string::npos,
            "runtime header exposes low-memory out-of-line RAM/Store-Queue hot paths");
    require(runtime_header.find("DCR_FPU_FORCE_INLINE") == std::string::npos &&
            runtime_header.find("dc_get_fr_bits") != std::string::npos &&
            runtime_header.find("dc_get_fmov64_bits") != std::string::npos &&
            runtime.find("std::uint32_t dc_get_fr_bits(const SH4Context&") != std::string::npos,
            "hot SH-4 FPU helpers compile once in dc_runtime.cpp rather than every shard");
    require(runtime_header.find("pvr_strip_window") != std::string::npos &&
            runtime_header.find("pvr_strip_size") != std::string::npos,
            "PVR strip assembly uses a fixed rolling three-vertex window");
    require(runtime.find(" | dispatch-inline=") != std::string::npos,
            "heartbeat reports inline dynamic dispatch hits/fallbacks");
    require(runtime_header.find("dc_runtime_tick_fast") != std::string::npos,
            "runtime header exposes the force-inlined SH-4 tick accumulator");
    require(runtime_header.find("__forceinline") != std::string::npos,
            "MSVC Release builds force-inline the SH-4 tick fast path");
    require(runtime.find("return dc_runtime_tick_fast(ctx, runtime, sh4_cycles)") != std::string::npos,
            "legacy tick wrapper delegates to the same accumulator semantics");
    require(runtime.find(" | sh4tick-fast=on") != std::string::npos,
            "heartbeat confirms the inline SH-4 tick fast path is compiled in");
    require(runtime_header.find("pvr_window_pump_cycle_accum") != std::string::npos &&
            runtime.find("pvr_window_pump_cycle_accum += sh4_cycles") != std::string::npos,
            "0.0.170 host UI polling is guest-cycle gated before wall-clock checks");
    require(runtime.find("runtime.device_clock_sh4_cycles >= runtime.device_clock_pvr_target_cycles") != std::string::npos,
            "0.0.170 PVR scheduler helper is called only when its deterministic deadline is due");
    require(runtime.find("runtime.device_clock_sh4_cycles >= runtime.device_clock_host_next_sync_cycle") != std::string::npos &&
            runtime.find("runtime.device_clock_host_next_sync_cycle = runtime.device_clock_sh4_cycles + sync_quantum") != std::string::npos,
            "0.0.199 coalesces host/AICA synchronization behind an explicit re-armed deadline");
    const auto present_begin = runtime.find("void dc_pvr_present_window(DCRuntime& runtime)");
    const auto present_end = runtime.find("void dc_pvr_close_window", present_begin);
    require(present_begin != std::string::npos && present_end != std::string::npos && present_end > present_begin,
            "generated runtime contains bounded PVR present function");
    const std::string present_body = runtime.substr(present_begin, present_end - present_begin);
    require(present_body.find("dc_device_clock_sync_host(runtime)") != std::string::npos,
            "0.0.170 preserves the proven post-Present host/AICA synchronization");
    require(runtime.find(" | perf=") != std::string::npos, "heartbeat reports profiler state/stride/samples");
    require(runtime.find(" | aica-nz=") != std::string::npos, "heartbeat reports nonzero native AICA frames");
    require(runtime.find(" | aica-slots=") != std::string::npos, "heartbeat reports active AICA slots");
    require(runtime.find(" | audio-play=") != std::string::npos, "heartbeat reports host-audio enable state");
    require(runtime.find("pvr_mt_enabled") != std::string::npos, "runtime exposes multithreaded PVR frame rendering");
    require(runtime.find("pvr-mt=") != std::string::npos, "heartbeat reports multithreaded PVR worker/frame timing");
    require(runtime.find("pvr_render_mt_deferred") != std::string::npos, "normal PVR path can rasterize deferred frames by independent row bands");
    require(runtime_header.find("pvr_gpu_enabled") != std::string::npos, "runtime exposes optional D3D11 PVR rasterization");
    require(runtime.find("pvr_render_gpu_deferred") != std::string::npos, "runtime contains the deferred GPU PVR frame path");
    require(runtime.find("D3D11CreateDevice") != std::string::npos, "Windows runtime creates a hardware D3D11 device for PVR rasterization");
    require(runtime.find("DXGI_FORMAT_B8G8R8A8_UNORM") != std::string::npos, "PVR GPU target preserves packed BGRA/ARGB framebuffer layout");
    require(runtime.find(" | pvr-gpu=") != std::string::npos, "heartbeat reports GPU PVR frames/fallback/timing");
    require(runtime.find(" | pvr-gbind=") != std::string::npos, "heartbeat reports deduplicated D3D11 constant/state binds");
    require(runtime.find("vertex_scratch") != std::string::npos && runtime.find("index_scratch") != std::string::npos &&
            runtime.find("run_scratch") != std::string::npos,
            "D3D11 renderer reuses persistent per-frame vertex/index/run scratch buffers");
    require(runtime.find("IASetIndexBuffer") != std::string::npos && runtime.find("DrawIndexed") != std::string::npos,
            "0.0.136 GPU path submits the shared vertex arena through a 32-bit index buffer");
    require(runtime.find("pvr_deferred_vertices") != std::string::npos && runtime.find(" | pvr-idx=arena32/r32/") != std::string::npos,
            "0.0.136 runtime exposes indexed deferred geometry and reduction telemetry");
    require(runtime.find("std::memcmp(&last_constants") != std::string::npos,
            "D3D11 renderer skips redundant constant-buffer updates between equivalent runs");
    require(runtime.find(" | pvr-gtex=") != std::string::npos, "heartbeat reports GPU texture uploads/reuse/unsupported triangles");
    require(runner.find("--pvr-gpu") != std::string::npos, "runner exposes experimental D3D11 PVR backend");
    require(generated_cmake.find("d3d11 d3dcompiler") != std::string::npos, "generated Windows target links D3D11 and runtime shader compiler");
    require(runtime.find("pvr_resolve_texel_coord") != std::string::npos, "bilinear PVR sampling resolves wrap/clamp taps without four normalized-coordinate passes");
    require(runtime.find("pvr_palette_raw") != std::string::npos, "PVR palette sampling uses a compact hot-register mirror");
    require(runtime.find("pvr_apply_background_plane") != std::string::npos, "runtime resolves the hardware PVR background plane instead of leaking the host clear color");
    require(runtime.find("pvr_deferred_translucent") != std::string::npos, "runtime defers translucent TA geometry until the scene boundary");
    require(runtime.find("pvr_flush_deferred_translucent") != std::string::npos, "runtime resolves deferred translucent geometry before render handoff");
    require(runtime.find("std::stable_sort(runtime.pvr_deferred_translucent") != std::string::npos, "translucent PVR geometry is stably sorted by inverse depth");
    require(runtime.find("if (runtime.pvr_state.list_type == 2u)") != std::string::npos, "only the translucent polygon list is deferred by the software rasterizer");
    require(runtime.find("st.vertex_alpha ? vertex : (vertex | 0xFF000000u)") != std::string::npos, "PVR shading honors the vertex-alpha enable bit");
    require(runtime.find("pvr-tsort=") != std::string::npos, "heartbeat reports translucent-list sorting activity");
    require(runtime.find("pvr_depth_test") != std::string::npos, "software PVR honors ISP depth comparison modes");
    require(runtime.find("runtime.pvr_state.depth_mode = static_cast<std::uint8_t>((mode1 >> 29) & 7u)") != std::string::npos, "TA polygon headers decode ISP DepthMode");
    require(runtime.find("runtime.pvr_state.cull_mode = static_cast<std::uint8_t>((mode1 >> 27) & 3u)") != std::string::npos, "TA polygon headers retain CullMode for diagnostics/future culling");
    require(runtime.find("runtime.pvr_state.list_type == 2u ? false") != std::string::npos ||
            runtime.find("st.list_type == 2u ? false") != std::string::npos,
            "sorted translucent color pass suppresses Z writes");
    require(runtime.find("pvr-zm=") != std::string::npos, "heartbeat reports actual PVR depth-mode usage");
    require(runtime.find("pvr-cm=") != std::string::npos, "heartbeat reports PVR cull-mode usage");
    require(runtime.find("pvr-3d=") != std::string::npos, "heartbeat reports isolated 3D raster/depth activity");
    require(runtime.find("pvr-3dprobe=") != std::string::npos, "heartbeat reports active 3D diagnostic stages");
    require(runtime.find("DCR_PVR_NO_BACKGROUND") != std::string::npos, "runtime exposes background suppression probe");
    require(runtime.find("DCR_PVR_3D_DEPTH_ALWAYS") != std::string::npos, "runtime exposes 3D depth bypass probe");
    require(runtime.find("DCR_PVR_3D_WHITE") != std::string::npos, "runtime exposes 3D shading bypass probe");
    require(runtime.find("DCR_PVR_3D_ONLY") != std::string::npos, "runtime exposes isolated 3D-only probe");
    require(runtime.find("DCR_PVR_WIREFRAME") != std::string::npos, "runtime exposes all-geometry wireframe probe");
    require(runtime.find("DCR_PVR_SOURCE_ONLY") != std::string::npos, "runtime exposes TA ingress source filter");
    require(runtime.find("pvr_triangle_ingress_source") != std::string::npos,
            "wireframe/source probes classify actual vertex ingress instead of only header ingress");
    require(runtime.find("pvr-sq2w1=") != std::string::npos,
            "heartbeat reports SQ TYPE2 dword1 pair-swap hints");
    require(runtime.find("pvr-sqpt=") != std::string::npos, "heartbeat reports Store Queue TA type histogram");
    require(runtime.find("pvr-ch2pt=") != std::string::npos, "heartbeat reports CH2 TA type histogram");
    require(runtime.find("pvr-trisrc=") != std::string::npos, "heartbeat reports rasterized triangle provenance");
    require(runtime.find("pvr-objtop=") != std::string::npos, "heartbeat reports hottest apparent OBJECT LIST SET source PC");
    require(runtime.find("incoming_list == 1u || incoming_list == 3u") != std::string::npos, "PVR recognizes modifier-volume list types separately from polygons");
    require(runtime.find("runtime.pvr_long_kind = 3u") != std::string::npos, "PVR consumes 64-byte modifier-volume vertices without parsing the continuation as a header");
    require(runtime.find("pvr-modv=") != std::string::npos, "heartbeat reports skipped modifier-volume activity");
    require(runtime.find("hdr_type == 1u") != std::string::npos, "PVR consumes USER TILE CLIP control parameters explicitly");
    require(runtime.find("runtime.pvr_state.clip_mode = static_cast<std::uint8_t>((pcw >> 16) & 3u)") != std::string::npos, "TA polygon headers retain USER TILE CLIP mode");
    require(runtime.find("runtime.pvr_user_clip_xmin") != std::string::npos, "PVR stores USER TILE CLIP rectangle state");
    require(runtime.find("runtime.pvr_state.user_clip_xmin = runtime.pvr_user_clip_xmin") != std::string::npos, "polygon headers snapshot USER TILE CLIP state for deferred rendering");
    require(runtime.find("clip_mode == 2u") != std::string::npos, "PVR clip mode 2 restricts rasterization to the user rectangle");
    require(runtime.find("clip_mode == 3u") != std::string::npos, "PVR clip mode 3 rejects pixels inside the user rectangle");
    require(runtime.find("pvr-clip=") != std::string::npos, "heartbeat reports USER TILE CLIP activity");
    require(runtime.find("pvr-objset=") != std::string::npos, "heartbeat reports OBJECT LIST SET control parameters");
    require(runtime.find("[PVR BAD-VTX]") != std::string::npos, "PVR retains diagnostics for genuinely non-finite decoded vertices");
    require(runtime.find("const bool plausible = finite;") != std::string::npos,
            "0.0.154 accepts finite off-screen TA coordinates instead of applying an arbitrary XY magnitude cutoff");
    require(runtime.find("pvr-xaccept=") != std::string::npos &&
            runtime.find("pvr_large_finite_vertices_accepted") != std::string::npos,
            "0.0.154 heartbeat reports large finite vertices accepted for renderer clipping");
    require(runtime_header.find("struct alignas(32) PVRTAStreamRaw") != std::string::npos &&
            runtime_header.find("std::vector<std::uint8_t> pvr_ta_stream_source") != std::string::npos &&
            runtime_header.find("std::vector<std::uint32_t> pvr_ta_stream_pc") != std::string::npos &&
            runtime_header.find("static_assert(sizeof(PVRTAStreamRaw) == 32u)") != std::string::npos,
            "0.0.155 keeps TA hot storage at 32 bytes plus one-byte source provenance");
    require(runtime.find("DCR_PVR_PROVENANCE") != std::string::npos &&
            runtime.find("pvr-ta-pack=") != std::string::npos &&
            runtime.find("pvr-prov=") != std::string::npos &&
            runtime.find("/stripgpu/stage32s1/fpusuper/gprcache/sqfuse/zero32/metricbatch") != std::string::npos,
            "0.0.158 exposes optional cold source-PC provenance, GPR cache and zero-copy fused TA staging");
    require(runtime.find("pvr-badv=") != std::string::npos, "heartbeat reports rejected non-finite vertices");
    require(runtime.find("pvr-pt=") != std::string::npos, "heartbeat reports TA parameter-type histogram");
    require(runtime.find("pvr-objsrc=") != std::string::npos, "heartbeat reports object-list-set submission source histogram");
    require(runtime.find("pvr-pktsrc=") != std::string::npos, "heartbeat reports all TA submit-source traffic");
    require(runtime.find("back_z_bits = reg(0x0088u)") != std::string::npos, "background depth comes from ISP_BACKGND_D");
    require(runtime.find("pvr-boardprobe=") != std::string::npos, "heartbeat reports all-geometry board diagnostic mode");
    require(runtime.find("DCR_PVR_BOARD_GEOMETRY") != std::string::npos, "runtime supports board geometry diagnostic probe");
    require(runtime.find("[PVR OBJSET]") != std::string::npos, "runtime logs milestone raw packets for suspicious object-list-set floods");
    require(runtime.find("DCR_PVR_TYPE7_GEOMETRY") != std::string::npos, "runtime contains the type-7 geometry-only A/B probe");
    require(runtime.find("pvr-i7geom=") != std::string::npos, "heartbeat reports type-7 geometry probe state");
    require(runtime.find("pvr-lt=") != std::string::npos, "heartbeat reports rasterized triangles by PVR list type");
    require(runtime.find("ISP_BACKGND_T") != std::string::npos, "runtime documents ISP_BACKGND_T based background-plane resolution");
    require(runtime.find("pvr_palette_to_argb") != std::string::npos, "runtime decodes PVR 4/8bpp palette entries");
    require(runtime.find("pvr_bilinear_samples") != std::string::npos, "runtime contains bilinear PVR texture filtering");
    require(runtime.find("pvr_texture_stride_pixels") != std::string::npos, "runtime models non-twiddled stride textures");
    require(runtime.find("pvr_wrap_uv") != std::string::npos, "runtime models clamp/mirror texture coordinates");
    require(runtime.find("uv_16bit") != std::string::npos, "runtime decodes 16-bit TA texture coordinates");
    require(runtime.find("0x011Cu") != std::string::npos, "runtime applies punch-through alpha reference");
    require(runtime.find("kPvrTexDirect64Base") != std::string::npos, "runtime models PVR direct texture SQ aperture");
    require(runtime.find("pvr_direct_texture_range") != std::string::npos, "runtime routes SQ texture uploads into VRAM");
    require(runtime.find("pvr_submit_sprite64") != std::string::npos, "runtime assembles 64-byte PVR sprite primitives");
    require(runtime.find("pvr_sprite_mode") != std::string::npos, "TA sprite global parameter keeps persistent 64-byte vertex mode");
    require(runtime.find("pvr_sprite_collecting") == std::string::npos, "obsolete one-shot sprite collection state is removed");
    require(runtime.find("Deliberately keep pvr_sprite_mode=true") != std::string::npos, "sprite B-half completion preserves the active sprite header");
    require(runtime.find("pvr-stail=") != std::string::npos, "heartbeat exposes apparent PCW types from sprite continuation payloads");
    require(runtime.find("pvr-tafsm=") != std::string::npos, "heartbeat exposes TA header/no-list/invalid-state diagnostics");
    require(runtime.find("pvr-strip=") != std::string::npos, "heartbeat exposes strip lifecycle diagnostics");
    require(runtime.find("pvr_float_argb") != std::string::npos, "runtime decodes TA four-float vertex colors");
    require(runtime.find("pvr_intensity_argb") != std::string::npos, "runtime decodes TA intensity colors against face color");
    require(runtime.find("pvr_vertex_type_for_state") != std::string::npos, "runtime classifies TA polygon vertex formats 0-14");
    require(runtime.find("pvr_decode_vertex_specialized") != std::string::npos &&
            runtime.find("pvr_vertex_decoder_for_type") != std::string::npos &&
            runtime.find("pvr_select_vertex_decoder") != std::string::npos,
            "0.0.136 selects a compiled specialized TA vertex decoder once per polygon header");
    require(runtime.find("pvr-ta-fast=vtx7/le32/src4/v64cache/specdec") != std::string::npos &&
            runtime.find("pvr-vdec=") != std::string::npos,
            "0.0.136 heartbeat exposes specialized decoder activity");
    require(runtime.find("dc_pvr_vertex_decoder_selftest") != std::string::npos,
            "generated runtime contains generic-vs-specialized vertex decoder self-test");
    require(runtime.find("pvr_parse_vertex32_typed_run<8u>") != std::string::npos &&
            runtime.find("pvr_parse_vertex32_typed_run<7u>") != std::string::npos,
            "0.0.148 retains direct Type-7/8 staged loops");
    require(runtime.find("const bool batch_metrics = true;") != std::string::npos,
            "0.0.191 perf profiling no longer disables the Type-7/8 TA bulk path");
    require(runtime.find("static_cast<double>(scaled) + 0.5") != std::string::npos,
            "0.0.148 retains exact lower-cost intensity rounding");
    require(runtime.find("pvr_defer_triangle_indexed_staged_strip") != std::string::npos &&
            runtime.find("pvr_vertex_plausibility_fast") != std::string::npos,
            "0.0.148 retains staged per-strip state reuse and exact bit plausibility");
    require(runtime.find("const std::uint32_t vertex_index = pvr_deferred_append_vertex(runtime, v);") != std::string::npos &&
            runtime.find("if (!bulk_indexed)") != std::string::npos &&
            runtime.find("runtime.pvr_strip_index_window[slot] = vertex_index;") != std::string::npos &&
            runtime.find("pvr_strip_gpu_unit_z_ok") != std::string::npos,
            "0.0.148 retains arena-only indexed strips and once-per-vertex GPU Z validation");
    require(runtime.find("typed78/exactu8/stripstate/bitplaus/arenaonly/zonce/stripbulk/opidx/opbulk/") != std::string::npos &&
            runtime.find("tacont/spgprog/cullstate/tspfull/texa/clamp/hotleaf/dirtytex") != std::string::npos,
            "0.0.148 heartbeat exposes strip-bulk plus TA/SPG/cull parity markers");
    require(runtime.find("pvr_finalize_staged_indexed_strip_bulk") != std::string::npos &&
            runtime.find("complete_strip = true") != std::string::npos,
            "0.0.148 bulk-closes only complete staged Type-7/8 strips");
    require(runtime.find("pvr-stripbulk=") != std::string::npos &&
            runtime.find("pvr_ta_strip_bulk_fallbacks") != std::string::npos,
            "0.0.148 heartbeat reports live strip-bulk coverage and fallback");
    require(runtime.find("pvr-opidx=") != std::string::npos &&
            runtime.find("pvr-latestrip=") != std::string::npos &&
            runtime.find("pvr_deferred_opaque_indices") != std::string::npos &&
            runtime.find("pvr_deferred_opaque_runs") != std::string::npos &&
            runtime.find("pvr_gpu_build_opaque_stream") != std::string::npos &&
            runtime.find("run.topology = 2u") != std::string::npos,
            "0.0.170 keeps native opaque strips as late first/count descriptors and builds GPU indices once");
    require(runtime.find("pvr-opbulk=") != std::string::npos &&
            runtime.find("pvr_resolve_staged_strip_deferred_mode") != std::string::npos &&
            runtime.find("pvr_opaque_strip_bulk_runs") != std::string::npos,
            "0.0.149 resolves complete opaque strips once and commits their indices in one block");
    require(runtime.find("pvr-typed=") != std::string::npos,
            "0.0.148 heartbeat reports live typed Type-7/8 coverage");
    require(runtime.find("offset == 0x0160u") != std::string::npos &&
            runtime.find("pvr_list_cont_writes") != std::string::npos &&
            runtime.find("pvr_ta_render_pass") != std::string::npos,
            "0.0.148 models TA_LIST_CONT as a preserved multi-pass continuation");
    require(runtime.find("0x0007DF77u") != std::string::npos &&
            runtime.find("0x15F28997u") != std::string::npos &&
            runtime.find("0x00090639u") != std::string::npos,
            "0.0.148 initializes Flycast-aligned PowerVR2 reset defaults");
    require(runtime.find("D3D11_CULL_NONE, D3D11_CULL_NONE, D3D11_CULL_NONE, D3D11_CULL_NONE") != std::string::npos &&
            runtime.find("rasterizer_states[state.cull_mode & 3u]") != std::string::npos,
            "0.0.183 keeps CullMode state identity while disabling unproven D3D11 face culling");
    require(runtime.find("pvr_gpu_upload_cpu_scanout") != std::string::npos &&
            runtime.find("pvr-gcpu-scanout=") != std::string::npos,
            "0.0.183 uploads CPU/MT fallback frames into direct DXGI scanout");
    require(runtime.find("float depth_z{};") == std::string::npos &&
            runtime.find("float depthZ : TEXCOORD1") == std::string::npos &&
            runtime.find("o.pos = float4(ndc, 0.5, 1.0)") != std::string::npos &&
            runtime.find("o.depth = depthInvW / (1.0 + depthInvW)") != std::string::npos &&
            runtime.find("pvr_gpu_depth_remap_frames") == std::string::npos &&
            runtime.find("pvr-gdepth=") == std::string::npos,
            "0.0.191 reverts the 0.0.184 affine depth remap to the 0.0.183 visual baseline");
    require(runtime.find("runtime.pvr_state.mipmap_d") != std::string::npos &&
            runtime.find("runtime.pvr_state.fog_ctrl") != std::string::npos &&
            runtime.find("runtime.pvr_state.src_select") != std::string::npos &&
            runtime.find("runtime.pvr_state.shadow") != std::string::npos,
            "0.0.148 retains full Flycast-aligned PCW/TSP surface identity");
    require(runtime.find("if (flags2.z == 0) texel.a = 1.0") != std::string::npos &&
            runtime.find("clamp(src, colorClampMin, colorClampMax)") != std::string::npos,
            "0.0.148 implements IgnoreTexA and TSP ColorClamp in D3D11");
    require(runtime.find("total_lines = std::max<std::uint32_t>") != std::string::npos &&
            runtime.find("field << 10u") != std::string::npos,
            "0.0.148 derives synthetic SPG status from programmed timing and field state");
    require(runtime.find("pvr_vertex_type_is_64") != std::string::npos, "runtime distinguishes 64-byte TA vertex formats");
    require(runtime.find("pvr_header_is_64") != std::string::npos, "runtime distinguishes 64-byte TA polygon headers");
    require(runtime.find("pvr_long_partial") != std::string::npos, "runtime buffers second halves of 64-byte TA parameters");
    require(runtime.find("pvr_apply_header64_tail") != std::string::npos, "runtime consumes 64-byte TA header continuation data");
    require(runtime.find("pvr_submit_vertex64") != std::string::npos, "runtime consumes 64-byte TA vertex continuation data");
    require(runtime.find("pvr_store_rtt_frame") != std::string::npos, "runtime commits render-to-texture scenes into VRAM");
    require(runtime.find("dc_pvr_enable_window") != std::string::npos, "runtime contains live PVR window support");
    require(runtime.find("pvr_finalize_logical_frame") != std::string::npos, "runtime contains logical-frame buffer rotation");
    require(runtime.find("pvr_render_buffer.swap(runtime.pvr_framebuffer)") != std::string::npos, "runtime hands the TA registration buffer to the render stage without a full-frame copy");
    require(runtime.find("pvr_present_buffer.swap(runtime.pvr_render_buffer)") != std::string::npos, "runtime page-flips the completed render buffer at VBlank");
    require(runtime.find("pvr_vblank_tick") != std::string::npos, "runtime models a PVR VBlank/page-flip stage");
    require(runtime.find("pvr_signal_list_end") != std::string::npos, "runtime raises Holly completion events for TA list delimiters");
    require(runtime.find("kAsicEventPvrPunchBit") != std::string::npos, "runtime models opaque/translucent/punch-through list completion events");
    require(runtime.find("kAsicEventRenderTspBit") != std::string::npos, "runtime models render-done Holly events");
    require(runtime.find("runtime.pvr_list_init_writes == 0u") != std::string::npos, "list-order frame heuristic is disabled after hardware TA_LIST_INIT is observed");
    require(runtime.find("runtime.mmio32[physical] = 0u") != std::string::npos, "PVR command registers self-clear after command consumption");
    require(runtime.find("pvr_pump_window_messages") != std::string::npos, "PVR live window pumps Win32 messages independently of new frames");
    require(runtime.find("pvr_service_host_ui") != std::string::npos, "PVR Win32 message servicing runs during heavy guest/AICA work");
    require(runtime_header.find("pvr_window_pump_interval_ns{10000000ull}") != std::string::npos, "PVR host UI has an independent high-rate pump cadence");
    require(runtime.find("pvr_guest_render_starts") != std::string::npos, "runtime distinguishes guest ISP starts from heuristic starts");
    require(runtime.find("pvr_window_next_present_ns") != std::string::npos, "runtime contains deadline-based frame pacing");
    require(runtime.find("pvr_profile_raster_ns") != std::string::npos, "runtime contains optional PVR profiler");
    require(runtime.find("aica_process_queue") != std::string::npos, "runtime contains KOS AICA command-queue HLE");
    require(runtime.find("AICAARM7Bus") != std::string::npos, "runtime contains ARM7-to-AICA bus adapter");
    require(runtime.find("dc_aica_enable_arm7") != std::string::npos, "runtime exposes native AICA ARM7 execution path");
    require(runtime.find("aica_update_interrupt_lines") != std::string::npos, "runtime routes AICA pending interrupts to ARM7");
    require(runtime.find("aica_advance_timers") != std::string::npos, "runtime advances native AICA timers");
    require(runtime.find("kAicaTimerA") != std::string::npos, "runtime models AICA timer registers");
    require(runtime.find("dc_aica_set_timer_div") != std::string::npos, "runtime exposes deterministic AICA timer pacing control");
    require(runtime.find("PlaySoundA") != std::string::npos, "runtime contains optional Windows host audio playback");
    require(runtime.find("kAicaHostPrefillChunks") != std::string::npos, "runtime contains buffered WinMM prefill queue");
    require(runtime.find("kAicaHostChunkFrames = 512u") != std::string::npos, "runtime uses 0.0.41 low-latency worker chunks");
    require(runtime.find("kAicaHostRingChunks = 32u") != std::string::npos, "runtime contains an expanded PCM producer reservoir");
    require(runtime.find("kAicaHostPrefillChunks = 6u") != std::string::npos, "runtime starts WinMM with enough prefill to absorb ARM7 maintenance slices");
    require(runtime.find("kAicaHostTargetChunks = 8u") != std::string::npos, "runtime keeps a bounded ~93 ms WinMM queue");
    require(runtime.find("aica_host_worker_loop") != std::string::npos, "runtime owns WinMM from a dedicated audio worker");
    require(runtime.find("audio_starves") != std::string::npos, "runtime reports producer starvation separately from device underruns");
    require(runtime.find("audio_overruns") != std::string::npos, "runtime reports stale live PCM dropped by the bounded ring");
    require(runtime.find("SetThreadPriority") != std::string::npos, "runtime raises the dedicated Windows audio worker priority");
    require(runtime.find("load_rd != cmp_rn && load_rd != cmp_rm") != std::string::npos, "runtime recognizes ARM7 polling loads on either CMP operand");
    require(runtime.find("aica_arm7_capture_boot_snapshot") != std::string::npos, "runtime snapshots AICA RAM when ARM7 reset is released");
    require(runtime.find("kAicaAdpcmQuantScale") != std::string::npos, "runtime implements Yamaha AICA ADPCM quantizer scaling");
    require(runtime.find("aica_native_decode_adpcm_nibble") != std::string::npos, "runtime decodes AICA 4-bit ADPCM nibbles");
    require(runtime.find("slot.format == 2u || slot.format == 3u") != std::string::npos, "runtime supports both normal and long-stream AICA ADPCM formats");
    require(runtime.find("adpcm_loop_snapshot_valid") != std::string::npos, "normal AICA ADPCM preserves predictor/quantizer state at loop start");
    require(runtime.find("aica_native_mix_host_clock") != std::string::npos, "live PCM production is decoupled from interpreted ARM7 throughput");
    require(runtime.find("aica_host_mix_enabled && runtime.aica_play_host && runtime.device_clock_host_sync") != std::string::npos,
            "ARM7 timer advancement does not duplicate the host-paced native sample clock");
    require(runtime.find("reset-vector-blank-at-release") != std::string::npos, "runtime classifies blank ARM7 reset-vector fallthrough");
    require(runtime.find("aica_sh4_aica_ram_bytes_written") != std::string::npos, "runtime tracks SH-4 firmware/data writes into AICA RAM");
    require(runtime.find("underrun_restarts") != std::string::npos, "runtime reports WinMM starvation recovery");
    require(runtime.find("dc_aica_pvr_sync_tick") != std::string::npos, "runtime retains legacy PVR-clocked AICA deficit fill");
    require(runtime.find("aica_pvr_sync_frames_added") != std::string::npos, "runtime reports legacy PVR/AICA sync top-up frames");
    require(runtime.find("device_clock_advance_cycles") != std::string::npos, "runtime contains common SH-4/AICA device timeline");
    require(runtime.find("device_clock_run_pvr_cycles") != std::string::npos, "runtime schedules PVR VBlank from guest SH-4 cycles");
    require(runtime.find("device_clock_run_host_pvr") == std::string::npos, "runtime does not drive PVR interrupts from host wall time");
    require(runtime.find("pvr-clockticks") != std::string::npos, "runtime reports guest-clocked PVR ticks");
    require(runtime.find("!runtime.device_clock_enabled) pvr_vblank_tick(runtime)") != std::string::npos, "ISP/heuristic immediate VBlank is compatibility-only when common clock is disabled");
    require(runtime.find("dc_device_clock_sync_host") != std::string::npos, "runtime can keep AICA advancing across live host stalls");
    require(runtime.find("device_clock_aica_hz") != std::string::npos, "runtime uses the AICA clock domain in common-time conversion");
    require(runtime_header.find("device_clock_aica_hz{22579200u}") != std::string::npos, "runtime uses the documented 22.5792 MHz AICA/ARM7 clock");
    require(runtime_header.find("aica_native_arm_steps_per_frame{512u}") != std::string::npos, "44.1 kHz AICA mixing uses 512 ARM/AICA clocks per sample");
    require(runtime_header.find("device_clock_host_max_catchup_steps{4096u}") != std::string::npos, "0.0.91 caps executed ARM7 handler work while timer-idle clocks fast-forward");
    require(runtime_header.find("device_clock_host_sync_quantum_cycles{524288u}") != std::string::npos &&
            runtime_header.find("device_clock_host_next_sync_cycle") != std::string::npos,
            "0.0.199 coalesces host sync with an explicit guest-cycle deadline");
    require(runtime.find("runtime.device_clock_sh4_cycles >= runtime.device_clock_host_next_sync_cycle") != std::string::npos &&
            runtime.find("kLegacyHostSyncQuantum = 262144u") != std::string::npos,
            "0.0.199 preserves ARM7 budget per unit time while reducing host-sync frequency");
    require(runtime.find("host-aica-drop=") != std::string::npos, "heartbeat exposes dropped host-only AICA catch-up debt");
    require(runtime.find("aica_host_fast_forward_timers") != std::string::npos, "host AICA scheduler can fast-forward timer-only idle spans");
    require(runtime.find("device_clock_host_fast_forward_steps") != std::string::npos, "runtime tracks AICA timer fast-forward clocks separately from executed ARM7 work");
    require(runtime.find("host-aica-ff=") != std::string::npos, "heartbeat exposes fast-forwarded AICA clocks/timer events/active slices");
    require(runtime.find(" | audio-clock=") != std::string::npos, "heartbeat exposes host-paced versus ARM-paced native audio");
    require(runtime.find("max_producer_gap_ns") != std::string::npos, "runtime measures emulator PCM producer gaps");
    require(runtime.find("max_submit_gap_ns") != std::string::npos, "runtime measures dedicated-worker WinMM submission gaps");
    require(runtime.find("release_attenuation_step") != std::string::npos, "runtime models native AICA key-off release smoothing");
    require(runtime_header.find("dc_get_xd_bits") != std::string::npos, "runtime models SH-4 XD register pairs");
    require(runtime_header.find("dc_set_xd_bits") != std::string::npos, "runtime writes SH-4 XD register pairs");
    require(runtime_header.find("dc_get_fmov64_bits") != std::string::npos, "runtime dispatches SZ=1 FMOV DR/XD sources");
    require(runtime_header.find("dc_set_fmov64_bits") != std::string::npos, "runtime dispatches SZ=1 FMOV DR/XD destinations");
    require(runtime.find("return static_cast<std::uint64_t>(lo) | (static_cast<std::uint64_t>(hi) << 32u)") != std::string::npos,
            "SZ=1 FMOV preserves raw even/odd lane order instead of numeric-double word order");
    require(runtime.find("fpu_fmov64_sq_stores") != std::string::npos,
            "runtime counts 64-bit FMOV stores that directly feed Store Queue");
    require(runtime.find(" | fmov64=") != std::string::npos,
            "heartbeat reports SZ=1 FMOV load/store/SQ/reg traffic");
    require(runtime.find("kG2DmaBasePhysical = 0x005F7800u") != std::string::npos, "runtime maps the four G2 DMA channel register blocks");
    require(runtime.find("g2_dma_execute") != std::string::npos, "runtime executes G2 DMA transfers instead of leaving START stuck");
    require(runtime.find("kAsicEventG2Dma0Bit = 1u << 15") != std::string::npos, "runtime raises Holly G2/SPU DMA completion events");
    require(runtime.find("dma.start = 0u") != std::string::npos, "G2 DMA completion clears the hardware-owned START bit");
    require(runtime.find("kGdccPlaySectors = 0x15u") != std::string::npos, "GD-ROM HLE recognizes BIOS PLAY_SECTORS command 0x15");
    require(runtime.find("kGdccRelease = 0x17u") != std::string::npos, "GD-ROM HLE recognizes resume-from-pause command 0x17");
    require(runtime.find("cdda_playing") != std::string::npos, "GD-ROM HLE retains CDDA playback state across PLAY/PAUSE/RELEASE/STOP");
    require(runtime.find("dc_gdrom_tick(runtime, sh4_cycles)") != std::string::npos, "guest SH-4 time advances CDDA independently of GD-ROM polling");
    require(runtime.find("cdda_terminated") != std::string::npos, "GD-ROM HLE models terminal CDDA status instead of leaving PLAY sticky");
    require(runtime.find("cdda_sector_ticks") != std::string::npos, "GD-ROM HLE counts guest-timed 75 Hz CDDA sector progression");
    require(runtime.find("0x13u : 0x15u") != std::string::npos, "GETSCD exposes terminated versus no-audio subcode status");
    require(runtime.find("gd-polls=") != std::string::npos, "heartbeat exposes GD-ROM status/subcode polling activity");
    require(runtime.find("cdda=") != std::string::npos, "heartbeat exposes live CDDA state and playhead");
    require(runtime.find("dc_gdrom_mix_cdda_frame(runtime, mix_l, mix_r)") != std::string::npos,
            "native 44.1 kHz mixer consumes real CDDA frames alongside AICA");
    require(runtime_header.find("std::array<std::uint8_t, 2352> cdda_audio_cache") != std::string::npos,
            "runtime keeps one raw 2352-byte CDDA sector cache");
    require(runtime.find("cdda_audio_frames_mixed") != std::string::npos,
            "runtime counts mixed CDDA PCM frames");
    require(runtime.find("cdda-pcm=") != std::string::npos,
            "heartbeat exposes CDDA raw-sector/mixer diagnostics");
    require(runtime.find("gd_find_track_by_number") != std::string::npos,
            "PLAY_TRACKS resolves real global disc track numbers");
    require(runtime.find("CDDA PLAY_TRACKS") != std::string::npos,
            "GD-ROM logs track-based CDDA requests with resolved FAD range");
    require(runtime.find("gd_write_toc(runtime, p[0], p[1])") != std::string::npos,
            "GETTOC/GETTOC2 expose the mapped multi-track TOC area");
    require(runtime.find("gd_disc_stream") != std::string::npos,
            "GD-ROM and CDDA reuse a persistent mapped-disc stream");
    require(runtime_header.find("struct SH4TMUChannel") != std::string::npos, "runtime models SH-4 TMU channel state");
    require(runtime.find("kSh4TmuPhysicalBase = 0x1FD80000u") != std::string::npos, "runtime maps SH-4 TMU P4 registers");
    require(runtime.find("sh4_tmu_tick(runtime, sh4_cycles)") != std::string::npos, "guest SH-4 cycles advance the TMU down-counters");
    require(runtime.find("case 2u: return 256u") != std::string::npos, "TMU TPSC=2 uses the Pphi/64 rate used by retail Katana timing loops");
    require(runtime.find("ch.tcr | 0x0100u") != std::string::npos, "TMU underflow sets the guest-visible UNF flag");
    require(runtime.find(" | tmu0=") != std::string::npos, "heartbeat exposes TMU0 run/count state");


    const auto arm7 = read_all(result.arm7_source);
    require(arm7.find("enter_exception") != std::string::npos, "generated standalone ARM7 contains exception entry");
    require(arm7.find("fiq_exceptions") != std::string::npos, "generated standalone ARM7 tracks FIQ exceptions");
    require(arm7.find("arm7_set_fiq_line") != std::string::npos, "generated standalone ARM7 exposes FIQ line control");

    const auto image = read_all(result.image_source);
    require(image.find("Hello from DreamcastRecomp") == std::string::npos, "image emitted as bytes, not source text shortcut");
    require(image.find("0x8C020000u") != std::string::npos, "rodata load address emitted");
    require(image.find("0x8C010000u") != std::string::npos, "executable .text image is preserved for literal pools/jump tables");

    const auto native = read_all(result.native_source);
    require(native.find("dc_read_c_string(runtime, ctx.r[4])") != std::string::npos, "native printf reads R4 format from Dreamcast RAM");
    require(native.find("runtime.register_target(0x8C010040u, &native_printf)") != std::string::npos, "native printf override registered");
    require(native.find("fp_arg_index = 4u") != std::string::npos, "native printf reads SH-4 floating varargs from DR4+");
    require(native.find("native_pvr_get_stats") != std::string::npos, "native overrides provide host-backed PVR stats");
    require(native.find("native_pvr_shutdown") != std::string::npos, "native overrides provide fast PVR shutdown");

    // runner already loaded above for 0.0.90 profiler/audio option checks.
    require(runner.find("call_recompiled(ctx, runtime, 0x8C010000u)") != std::string::npos, "runner executes recompiled entry");
    require(runner.find("--peek32") != std::string::npos, "runner can inspect guest memory after execution");
    require(runner.find("--membin") != std::string::npos, "runner can load binary fixtures into guest RAM");
    require(runner.find("--mem8") != std::string::npos, "runner can seed guest bytes");
    require(runner.find("--mem16") != std::string::npos, "runner can seed guest halfwords");
    require(runner.find("--pvr-window") != std::string::npos, "runner exposes real-time PVR window option");
    require(runner.find("--pvr-window-throttle-ms") != std::string::npos, "runner exposes PVR live throttling");
    require(runner.find("--pvr-window-fps") != std::string::npos, "runner exposes target-FPS deadline pacing");
    require(runner.find("--pvr-profile") != std::string::npos, "runner exposes PVR performance profiling");
    require(runner.find("--probe-controller-a-then-start") != std::string::npos, "runner exposes deterministic virtual Maple controller probe");
    require(runner.find("--pvr-frame-sync") != std::string::npos, "runner exposes logical-frame synchronized presentation");
    require(runner.find("--aica-kos-hle") != std::string::npos, "runner exposes KOS AICA queue HLE");
    require(runner.find("--aica-arm7") != std::string::npos, "runner exposes native AICA ARM7 execution");
    require(runner.find("--aica-arm7-slice") != std::string::npos, "runner exposes ARM7 scheduling slice control");
    require(runner.find("--aica-arm7-boot") != std::string::npos, "runner exposes ARM7 boot scheduling control");
    require(runner.find("--aica-timer-div") != std::string::npos, "runner exposes deterministic AICA timer divider control");
    require(runner.find("--device-clock") != std::string::npos, "runner exposes common virtual device clock");
    require(runner.find("--device-clock-host") != std::string::npos, "runner exposes host-synchronized live device clock");
    require(runner.find("--maple-host-input") != std::string::npos, "runner exposes host Maple controller input");
    require(runner.find("--controller-profile=") != std::string::npos, "0.0.210 runner exposes controller profile loading");
    require(runtime.find("dc_maple_load_controller_profile") != std::string::npos, "0.0.210 runtime emits controller profile loader");
    require(runtime.find("joyGetPosEx") != std::string::npos, "runtime supports WinMM/DirectInput controller polling");
    require(runtime.find("logical:up") != std::string::npos, "0.1.2 emits backend-neutral controller bindings");
    require(runtime.find("pad.pov == 0xFFFFu") != std::string::npos, "0.1.2 maps native DirectInput POV/D-pad state");
    require(runtime.find("button_down(1)") != std::string::npos, "0.1.2 maps native DS4 Cross to Dreamcast A");
    require(runtime.find("backend == \"winmm\" || backend == \"ps4\"") != std::string::npos, "0.1.2 exposes explicit native PS4 backend");
    require(runtime.find("joyGetNumDevs()") != std::string::npos, "0.1.0 scans WinMM slots when the preferred device is absent");
    require(runtime.find("range:<axis>:<rest>:<full>") != std::string::npos, "0.1.0 documents calibrated WinMM trigger ranges in generated runtime");
    require(runtime.find("binding.rfind(\"range:\", 0) == 0u") != std::string::npos, "0.1.0 emits calibrated analog trigger range parsing");
    require(runtime.find("return button >= 0 && button_down(button) ? 255 : 0;") != std::string::npos, "0.1.0 avoids native PS4 U/V trigger cross-talk by default");
    require(runtime.find("maple_process_dma") != std::string::npos, "runtime emits Maple DMA parser");
    require(runtime.find("dc_maple_enable_host_input") != std::string::npos, "runtime emits host Maple bridge");
    require(runtime.find("maple-input=0x") != std::string::npos, "heartbeat exposes cooked and recently-seen Maple button masks");
    require(runtime.find("maple-src=0x") != std::string::npos, "heartbeat separates keyboard and XInput Maple sources");
    require(runtime.find("down(VK_SPACE) || down('Z')") != std::string::npos, "keyboard exposes convenient Dreamcast A aliases");
    require(native.find("register_maple_host_overrides") != std::string::npos, "native overrides expose KOS Maple host bridge");
    require(runner.find("--aica-wav") != std::string::npos, "runner exposes AICA WAV capture");
    require(runner.find("--cdda-wav") != std::string::npos, "runner exposes source-only CDDA WAV capture");
    require(runtime_header.find("audio_probe_flush_interval_ns{15000000000ull}") != std::string::npos,
            "audio probe snapshots default to a 15-second wall-clock cadence");
    require(runtime.find("dc_audio_probe_periodic_flush(runtime, now)") != std::string::npos,
            "host device clock services periodic WAV snapshots without relying on runner shutdown");
    require(runtime.find("[AUDIO PROBE] periodic WAV flush #") != std::string::npos,
            "periodic audio probe writes are visible in the console");
    require(runtime.find("MoveFileExA") != std::string::npos,
            "Windows probe snapshots publish through a temporary file replacement");
    require(runtime.find("gd_cdda_prepare_scramble") != std::string::npos, "runtime emits CDDA raw/YellowBook per-sector detection");
    require(runtime.find("gd_cdda_repair_sync_prefix") != std::string::npos, "runtime repairs non-audio YellowBook SYNC prefixes inside CDDA");
    require(runtime.find("cdda_audio_scramble_transitions") != std::string::npos, "runtime tracks in-track CDDA encoding transitions");
    require(runtime.find("cdda_audio_sync_repairs") != std::string::npos, "runtime counts repaired CDDA SYNC prefixes");
    require(runtime.find("cdda_audio_impulse_repairs") != std::string::npos, "runtime counts repaired Yellow-Book CDDA impulses");
    require(runtime.find("gd_cdda_repair_impulses") != std::string::npos, "runtime conceals short Yellow-Book CDDA impulse bursts");
    require(runtime.find("std::array<std::int32_t, 5> window") != std::string::npos, "runtime uses a five-sample median for Yellow-Book de-click");
    require(runtime.find("for (unsigned pass = 0u; pass < 4u; ++pass)") != std::string::npos, "runtime performs four validated de-click passes");
    require(runtime.find("kImpulseError = 2048u") != std::string::npos, "CDDA impulse repair matches validated 2048 threshold");
    require(runtime.find("kContextSectors = 5u") != std::string::npos, "Yellow-Book reconstruction keeps +/-2 sectors of source context");
    require(runtime.find("kPolyphase") != std::string::npos, "Yellow-Book reconstruction uses the validated band-limited polyphase resampler");
    require(runtime.find("phase_num") != std::string::npos, "Yellow-Book resampler preserves the exact 196/195 rational phase");
    require(runtime.find("gd_yellowbook_scrambler") != std::string::npos, "runtime emits ECMA-130 YellowBook descrambler");
    require(runtime.find("cdda_audio_scramble_mode") != std::string::npos, "runtime keeps current-sector CDDA scrambling state");
    require(runtime.find("gd_cdda_analyze_audio_sector") != std::string::npos, "runtime emits CDDA per-track byte-order analysis");
    require(runtime.find("cdda-src=T") != std::string::npos, "heartbeat exposes CDDA source track/encoding/order diagnostics");
    require(runtime.find("cdda-clock=") != std::string::npos, "heartbeat exposes host/guest 75 Hz CDDA clock source");
    require(runtime.find("dc_gdrom_sync_host") != std::string::npos, "host-synchronized device clock owns CDDA 75 Hz playhead");
    require(runner.find("--aica-stop-after-starts") != std::string::npos, "runner exposes deterministic AICA application stop limit");

    // Verify that delayed conditional branches use a saved pre-slot T value.
    dcrecomp::DCIRFunction delayed;
    delayed.name = "delayed_t_emit_probe";
    delayed.entry = 0x3000;
    dcrecomp::DCIRBlock delayed_block;
    delayed_block.start_address = 0x3000;
    dcrecomp::DCIRInstruction save_t;
    save_t.op = dcrecomp::DCIROp::SaveT;
    save_t.source_address = 0x3000;
    dcrecomp::DCIRInstruction slot_cmp;
    slot_cmp.op = dcrecomp::DCIROp::CmpEq;
    slot_cmp.source_address = 0x3002;
    slot_cmp.dst = 4;
    slot_cmp.src = 3;
    dcrecomp::DCIRInstruction delayed_branch;
    delayed_branch.op = dcrecomp::DCIROp::BranchIfSavedTrue;
    delayed_branch.source_address = 0x3000;
    delayed_branch.target = 0x3010;
    delayed_block.instructions = {save_t, slot_cmp, delayed_branch};
    delayed.blocks.push_back(delayed_block);

    const auto delayed_result = dcrecomp::emit_cpp(elf, delayed, {out_dir / "delay_t", true, false, false});
    const auto delayed_source = read_all(delayed_result.function_source);
    require(delayed_source.find("bool delayed_t = false") != std::string::npos, "generated function has delayed T temporary");
    require(delayed_source.find("delayed_t = (ctx.sr & 1u) != 0u") != std::string::npos, "generated code snapshots T");
    require(delayed_source.find("if (delayed_t)") != std::string::npos, "generated delayed branch uses saved T");

    // FPSCR.SZ=1 uses odd FMOV register fields to address XD pairs in the
    // opposite FPU bank. Ensure codegen routes those encodings through the
    // DR/XD-aware 64-bit helper instead of leaving a runtime trap.
    dcrecomp::DCIRFunction fmov64;
    fmov64.name = "fmov64_xd_emit_probe";
    fmov64.entry = 0x4000;
    dcrecomp::DCIRBlock fmov64_block;
    fmov64_block.start_address = 0x4000;
    dcrecomp::DCIRInstruction fmov_xd;
    fmov_xd.op = dcrecomp::DCIROp::FmovLoadPostInc;
    fmov_xd.source_address = 0x4000;
    fmov_xd.src = 14;
    fmov_xd.dst = 1; // odd field => XD0 when FPSCR.SZ=1
    fmov64_block.instructions = {fmov_xd};
    fmov64.blocks.push_back(fmov64_block);
    const auto fmov64_result = dcrecomp::emit_cpp(elf, fmov64, {out_dir / "fmov64_xd", true, false, false});
    const auto fmov64_source = read_all(fmov64_result.function_source);
    require(fmov64_source.find("dc_set_fmov64_bits(ctx, 1u") != std::string::npos, "SZ=1 odd FMOV destination uses XD-aware helper");
    require(fmov64_source.find("FMOV SZ=1 odd/XD register form") == std::string::npos, "SZ=1 odd FMOV no longer emits unimplemented trap");

    // 0.0.200: FPSCR.DN is architectural state, not merely a stored bit.
    // Full FPSCR writes must synchronize the host denormal mode so native
    // floating-point arithmetic follows SH-4 denormal-to-zero semantics.
    dcrecomp::DCIRFunction fpscr_sync;
    fpscr_sync.name = "fpscr_dn_sync_emit_probe";
    fpscr_sync.entry = 0x4800;
    dcrecomp::DCIRBlock fpscr_block;
    fpscr_block.start_address = 0x4800;
    dcrecomp::DCIRInstruction lds_fpscr;
    lds_fpscr.op = dcrecomp::DCIROp::LdsFpscr;
    lds_fpscr.source_address = 0x4800;
    lds_fpscr.src = 2;
    dcrecomp::DCIRInstruction ldsl_fpscr;
    ldsl_fpscr.op = dcrecomp::DCIROp::LdsLFpscr;
    ldsl_fpscr.source_address = 0x4802;
    ldsl_fpscr.src = 3;
    fpscr_block.instructions = {lds_fpscr, ldsl_fpscr};
    fpscr_sync.blocks.push_back(fpscr_block);
    const auto fpscr_sync_result = dcrecomp::emit_cpp(elf, fpscr_sync, {out_dir / "fpscr_dn_sync", true, false, false});
    const auto fpscr_sync_source = read_all(fpscr_sync_result.function_source);
    require(fpscr_sync_source.find("dc_write_fpscr(ctx, ctx.r[2])") != std::string::npos,
            "0.0.200 LDS FPSCR synchronizes host FPU mode");
    require(fpscr_sync_source.find("dc_tmp_fpscr") != std::string::npos &&
            fpscr_sync_source.find("dc_write_fpscr(ctx, dc_tmp_fpscr)") != std::string::npos,
            "0.0.200 LDS.L FPSCR synchronizes host FPU mode");
    require(runtime.find("dc_sync_host_fpu_mode") != std::string::npos &&
            runtime.find("kFpscrDn = 1u << 18") != std::string::npos &&
            runtime.find("_DN_FLUSH") != std::string::npos &&
            runtime.find("kMxcsrFlushToZero = 1u << 15") != std::string::npos,
            "0.0.200 runtime mirrors SH-4 FPSCR.DN into the host FP control state");
    require(runner.find("dc_write_fpscr(ctx, static_cast<std::uint32_t>(std::stoul(read_value(), nullptr, 0)))") != std::string::npos,
            "0.0.200 runner FPSCR override also synchronizes host FPU mode");

    // 0.0.140: exact same-block provenance replaces the old fixed lookback. A
    // PREF is specialized only while its live base still derives from a store;
    // a definite overwrite before PREF must return to the generic path.
    dcrecomp::DCIRFunction ta_pref;
    ta_pref.name = "ta_pref_native_emit_probe";
    ta_pref.entry = 0x5000;
    dcrecomp::DCIRBlock ta_block;
    ta_block.start_address = 0x5000;
    dcrecomp::DCIRInstruction ta_store;
    ta_store.op = dcrecomp::DCIROp::Store32;
    ta_store.source_address = 0x5000;
    ta_store.dst = 4;
    ta_store.src = 2;
    dcrecomp::DCIRInstruction ta_pref_i;
    ta_pref_i.op = dcrecomp::DCIROp::Pref;
    ta_pref_i.source_address = 0x5002;
    ta_pref_i.dst = 4;
    ta_block.instructions = {ta_store, ta_pref_i};
    ta_pref.blocks.push_back(ta_block);
    const auto ta_pref_result = dcrecomp::emit_cpp(elf, ta_pref, {out_dir / "ta_pref_native", true, false, false});
    const auto ta_pref_source = read_all(ta_pref_result.function_source);
    require(ta_pref_source.find("dc_pref_ta_native(runtime, ctx.r[4], 0x00005002u)") != std::string::npos,
            "0.0.140 recognizes an exact local Store Queue producer PREF site");

    dcrecomp::DCIRFunction ta_local_kill = ta_pref;
    ta_local_kill.name = "ta_pref_local_kill_probe";
    dcrecomp::DCIRInstruction local_kill;
    local_kill.op = dcrecomp::DCIROp::MovImm;
    local_kill.source_address = 0x5002;
    local_kill.dst = 4;
    local_kill.immediate = 0;
    ta_local_kill.blocks[0].instructions = {ta_store, local_kill, ta_pref_i};
    ta_local_kill.blocks[0].instructions[2].source_address = 0x5004;
    const auto ta_local_kill_result = dcrecomp::emit_cpp(elf, ta_local_kill, {out_dir / "ta_pref_local_kill", true, false, false});
    const auto ta_local_kill_source = read_all(ta_local_kill_result.function_source);
    require(ta_local_kill_source.find("dc_guest_pref(runtime, ctx.r[4], 0x00005004u)") != std::string::npos,
            "0.0.140 local provenance dies on a definite overwrite before PREF");
    require(runtime.find("pref_ta_native_hits") != std::string::npos &&
            runtime.find("pvr-ta-native=") != std::string::npos,
            "0.0.140 runtime exposes guarded native TA PREF hit/fallback telemetry");

    // 0.0.140: producer provenance crosses direct CFG edges and MOV aliases,
    // but only as a MUST fact. Definite overwrites kill both local and inherited
    // provenance.
    dcrecomp::DCIRFunction ta_flow;
    ta_flow.name = "ta_pref_cfg_flow_probe";
    ta_flow.entry = 0x6000;
    dcrecomp::DCIRBlock ta_flow_a;
    ta_flow_a.start_address = 0x6000;
    dcrecomp::DCIRInstruction flow_store;
    flow_store.op = dcrecomp::DCIROp::Store32;
    flow_store.source_address = 0x6000;
    flow_store.dst = 4;
    flow_store.src = 2;
    dcrecomp::DCIRInstruction flow_branch;
    flow_branch.op = dcrecomp::DCIROp::Branch;
    flow_branch.source_address = 0x6002;
    flow_branch.target = 0x6010;
    ta_flow_a.instructions = {flow_store, flow_branch};

    dcrecomp::DCIRBlock ta_flow_b;
    ta_flow_b.start_address = 0x6010;
    dcrecomp::DCIRInstruction flow_alias;
    flow_alias.op = dcrecomp::DCIROp::MovReg;
    flow_alias.source_address = 0x6010;
    flow_alias.src = 4;
    flow_alias.dst = 5;
    dcrecomp::DCIRInstruction flow_pref;
    flow_pref.op = dcrecomp::DCIROp::Pref;
    flow_pref.source_address = 0x6012;
    flow_pref.dst = 5;
    ta_flow_b.instructions = {flow_alias, flow_pref};
    ta_flow.blocks = {ta_flow_a, ta_flow_b};

    const auto ta_flow_result = dcrecomp::emit_cpp(elf, ta_flow, {out_dir / "ta_pref_cfg_flow", true, false, false});
    const auto ta_flow_source = read_all(ta_flow_result.function_source);
    require(ta_flow_source.find("dc_pref_ta_native(runtime, ctx.r[5], 0x00006012u, true)") != std::string::npos,
            "0.0.140 propagates MUST TA producer provenance across blocks and MOV aliases");

    dcrecomp::DCIRFunction ta_kill = ta_flow;
    ta_kill.name = "ta_pref_cfg_kill_probe";
    ta_kill.blocks[1].instructions.clear();
    dcrecomp::DCIRInstruction flow_kill;
    flow_kill.op = dcrecomp::DCIROp::MovImm;
    flow_kill.source_address = 0x6010;
    flow_kill.dst = 4;
    flow_kill.immediate = 0;
    dcrecomp::DCIRInstruction killed_pref;
    killed_pref.op = dcrecomp::DCIROp::Pref;
    killed_pref.source_address = 0x6012;
    killed_pref.dst = 4;
    ta_kill.blocks[1].instructions = {flow_kill, killed_pref};
    const auto ta_kill_result = dcrecomp::emit_cpp(elf, ta_kill, {out_dir / "ta_pref_cfg_kill", true, false, false});
    const auto ta_kill_source = read_all(ta_kill_result.function_source);
    require(ta_kill_source.find("dc_guest_pref(runtime, ctx.r[4], 0x00006012u)") != std::string::npos,
            "0.0.140 kills CFG producer provenance after a definite GPR overwrite");

    // MUST join: if only one predecessor carries producer provenance, the join
    // PREF must stay generic rather than relying on a may-union false positive.
    dcrecomp::DCIRFunction ta_join;
    ta_join.name = "ta_pref_cfg_must_join_probe";
    ta_join.entry = 0x7000;
    dcrecomp::DCIRBlock join_entry; join_entry.start_address = 0x7000;
    dcrecomp::DCIRInstruction join_cond; join_cond.op = dcrecomp::DCIROp::BranchIfTrue; join_cond.source_address = 0x7000; join_cond.target = 0x7010;
    join_entry.instructions = {join_cond};
    dcrecomp::DCIRBlock join_no_store; join_no_store.start_address = 0x7002;
    dcrecomp::DCIRInstruction join_no_store_branch; join_no_store_branch.op = dcrecomp::DCIROp::Branch; join_no_store_branch.source_address = 0x7002; join_no_store_branch.target = 0x7020;
    join_no_store.instructions = {join_no_store_branch};
    dcrecomp::DCIRBlock join_store; join_store.start_address = 0x7010;
    dcrecomp::DCIRInstruction join_store_i = ta_store; join_store_i.source_address = 0x7010; join_store_i.dst = 4;
    dcrecomp::DCIRInstruction join_store_branch; join_store_branch.op = dcrecomp::DCIROp::Branch; join_store_branch.source_address = 0x7012; join_store_branch.target = 0x7020;
    join_store.instructions = {join_store_i, join_store_branch};
    dcrecomp::DCIRBlock join_sink; join_sink.start_address = 0x7020;
    dcrecomp::DCIRInstruction join_pref; join_pref.op = dcrecomp::DCIROp::Pref; join_pref.source_address = 0x7020; join_pref.dst = 4;
    join_sink.instructions = {join_pref};
    ta_join.blocks = {join_entry, join_no_store, join_store, join_sink};
    const auto ta_join_result = dcrecomp::emit_cpp(elf, ta_join, {out_dir / "ta_pref_cfg_must_join", true, false, false});
    const auto ta_join_source = read_all(ta_join_result.function_source);
    require(ta_join_source.find("dc_guest_pref(runtime, ctx.r[4], 0x00007020u)") != std::string::npos,
            "0.0.140 requires producer provenance on every predecessor at CFG joins");
    require(runtime.find("pvr-ta-flow=") != std::string::npos &&
            runtime.find("pref_ta_native_flow_hits") != std::string::npos,
            "0.0.140 runtime exposes exact local-vs-CFG TA native telemetry");

    // 0.0.156: recognize a complete 32-byte same-block SQ producer and delay
    // its stores until PREF can submit the packet directly to TA. The runtime
    // guard preserves the exact old SQ/PREF path when the producer is not a
    // full, aligned, single-queue packet.
    dcrecomp::DCIRFunction ta_fuse;
    ta_fuse.name = "ta_pref_fused_32_emit_probe";
    ta_fuse.entry = 0x8000;
    dcrecomp::DCIRBlock ta_fuse_block;
    ta_fuse_block.start_address = 0x8000;
    for (std::uint32_t n = 0; n < 8u; ++n) {
        dcrecomp::DCIRInstruction st;
        st.op = dcrecomp::DCIROp::Store32Disp;
        st.source_address = 0x8000u + n * 2u;
        st.dst = 4;
        st.src = static_cast<std::uint8_t>(2u + (n & 3u));
        st.immediate = static_cast<std::int32_t>(n * 4u);
        ta_fuse_block.instructions.push_back(st);
    }
    dcrecomp::DCIRInstruction ta_fuse_pref;
    ta_fuse_pref.op = dcrecomp::DCIROp::Pref;
    ta_fuse_pref.source_address = 0x8010;
    ta_fuse_pref.dst = 4;
    ta_fuse_block.instructions.push_back(ta_fuse_pref);
    ta_fuse.blocks.push_back(ta_fuse_block);
    const auto ta_fuse_result = dcrecomp::emit_cpp(elf, ta_fuse, {out_dir / "ta_pref_fused_32", true, false, false});
    const auto ta_fuse_source = read_all(ta_fuse_result.function_source);
    require(ta_fuse_source.find("std::array<SQTAFusedCapture, 8u>") != std::string::npos &&
            ta_fuse_source.find("dc_pref_ta_fused_packet") != std::string::npos &&
            ta_fuse_source.find("sqf_0[0u]") != std::string::npos,
            "0.0.156 fuses a complete 32-byte same-block SQ producer into TA PREF");

    dcrecomp::DCIRFunction ta_fuse_barrier = ta_fuse;
    ta_fuse_barrier.name = "ta_pref_fused_barrier_probe";
    dcrecomp::DCIRInstruction barrier_load;
    barrier_load.op = dcrecomp::DCIROp::Load32;
    barrier_load.source_address = 0x800Fu;
    barrier_load.dst = 7;
    barrier_load.src = 8;
    ta_fuse_barrier.blocks[0].instructions.insert(ta_fuse_barrier.blocks[0].instructions.end() - 1, barrier_load);
    const auto ta_fuse_barrier_result = dcrecomp::emit_cpp(elf, ta_fuse_barrier, {out_dir / "ta_pref_fused_barrier", true, false, false});
    const auto ta_fuse_barrier_source = read_all(ta_fuse_barrier_result.function_source);
    require(ta_fuse_barrier_source.find("std::array<SQTAFusedCapture, 8u>") == std::string::npos,
            "0.0.156 SQ fusion refuses producers separated from PREF by a memory-observing barrier");

    // 0.0.170: function-wide static GPR caching is now experimental-only.
    // Production codegen defaults to the proven context-backed path; explicit
    // DCR_GPR_CACHE=1 still emits the hardened cache for controlled A/B tests.
    dcrecomp::DCIRFunction gpr_probe;
    gpr_probe.name = "gpr_static_cache_emit_probe";
    gpr_probe.entry = 0x8800;
    dcrecomp::DCIRBlock gpr_block; gpr_block.start_address = 0x8800;
    for (std::uint32_t n = 0; n < 8u; ++n) {
        dcrecomp::DCIRInstruction add;
        add.op = dcrecomp::DCIROp::AddReg;
        add.source_address = 0x8800u + n * 2u;
        add.dst = 2;
        add.src = 3;
        gpr_block.instructions.push_back(add);
    }
    dcrecomp::DCIRInstruction gpr_ret;
    gpr_ret.op = dcrecomp::DCIROp::Return;
    gpr_ret.source_address = 0x8810;
    gpr_block.instructions.push_back(gpr_ret);
    gpr_probe.blocks = {gpr_block};
#if defined(_WIN32)
    _putenv_s("DCR_GPR_CACHE", "1");
#else
    setenv("DCR_GPR_CACHE", "1", 1);
#endif
    const auto gpr_on_result = dcrecomp::emit_cpp(elf, gpr_probe, {out_dir / "gpr_cache_on", true, false, false});
    const auto gpr_on_source = read_all(gpr_on_result.function_source);
    require(gpr_on_source.find("static SH-4 integer register cache") != std::string::npos &&
            gpr_on_source.find("gpc_r2") != std::string::npos &&
            gpr_on_source.find("gpc_r3") != std::string::npos &&
            gpr_on_source.find("gpc_flush(3u)") != std::string::npos,
            "0.0.170 explicit DCR_GPR_CACHE=1 retains hardened branch-free GPR caching");

    // 0.0.158 regression: a multi-entry AOT function can start at a block that
    // bypasses a pure assignment.  Write-only destinations therefore must be
    // preloaded so an exit/flush does not overwrite the architectural value
    // with the local's zero initializer.
    dcrecomp::DCIRFunction gpr_multi_entry;
    gpr_multi_entry.name = "gpr_multi_entry_write_only_probe";
    gpr_multi_entry.entry = 0x8A00;
    dcrecomp::DCIRBlock gpr_me_a; gpr_me_a.start_address = 0x8A00;
    for (std::uint32_t n = 0; n < 12u; ++n) {
        dcrecomp::DCIRInstruction gpr_me_mov;
        gpr_me_mov.op = dcrecomp::DCIROp::MovImm;
        gpr_me_mov.source_address = 0x8A00u + n * 2u;
        gpr_me_mov.dst = 2;
        gpr_me_mov.immediate = 0x12345678u + n;
        gpr_me_a.instructions.push_back(gpr_me_mov);
    }
    dcrecomp::DCIRInstruction gpr_me_br;
    gpr_me_br.op = dcrecomp::DCIROp::Branch;
    gpr_me_br.source_address = 0x8A18;
    gpr_me_br.target = 0x8A20;
    gpr_me_a.instructions.push_back(gpr_me_br);
    dcrecomp::DCIRBlock gpr_me_b; gpr_me_b.start_address = 0x8A20;
    dcrecomp::DCIRInstruction gpr_me_ret;
    gpr_me_ret.op = dcrecomp::DCIROp::Return;
    gpr_me_ret.source_address = 0x8A20;
    gpr_me_b.instructions = {gpr_me_ret};
    gpr_multi_entry.blocks = {gpr_me_a, gpr_me_b};
    const auto gpr_me_result = dcrecomp::emit_cpp(elf, gpr_multi_entry, {out_dir / "gpr_multi_entry", true, false, false});
    const auto gpr_me_source = read_all(gpr_me_result.function_source);
    require(gpr_me_source.find("gpc_r2 = ctx.r[2u]") != std::string::npos,
            "0.0.170 experimental GPR cache preserves multi-entry write-only destinations");

    // 0.0.170 regression: a CALL is an architectural observation barrier.
    // Cached writable state must be materialized before the callee and reloaded
    // afterwards so callee-visible changes cannot be overwritten by stale locals.
    dcrecomp::DCIRFunction gpr_call_barrier = gpr_probe;
    gpr_call_barrier.name = "gpr_call_barrier_reload_probe";
    dcrecomp::DCIRInstruction gpr_call;
    gpr_call.op = dcrecomp::DCIROp::Call;
    gpr_call.source_address = 0x8810;
    gpr_call.target = 0x8B00;
    gpr_call_barrier.blocks[0].instructions.insert(gpr_call_barrier.blocks[0].instructions.end() - 1, gpr_call);
    gpr_call_barrier.blocks[0].instructions.back().source_address = 0x8812;
    const auto gpr_call_result = dcrecomp::emit_cpp(elf, gpr_call_barrier, {out_dir / "gpr_call_barrier", true, false, false});
    const auto gpr_call_source = read_all(gpr_call_result.function_source);
    require(gpr_call_source.find("gpc_flush(2u)") != std::string::npos &&
            gpr_call_source.find("call_recompiled(ctx, runtime, 0x00008B00u)") != std::string::npos &&
            gpr_call_source.find("gpc_reload()") != std::string::npos,
            "0.0.170 experimental GPR cache flushes/reloads around CALL barriers");
#if defined(_WIN32)
    _putenv_s("DCR_GPR_CACHE", "");
#else
    unsetenv("DCR_GPR_CACHE");
#endif
    const auto gpr_off_result = dcrecomp::emit_cpp(elf, gpr_probe, {out_dir / "gpr_cache_default_off", true, false, false});
    const auto gpr_off_source = read_all(gpr_off_result.function_source);
    require(gpr_off_source.find("static SH-4 integer register cache") == std::string::npos &&
            gpr_off_source.find("gpc_r2") == std::string::npos,
            "0.0.170 production codegen defaults to the exact context-backed GPR baseline");

    // 0.0.158: 0.0.156 measurements proved the adaptive hot-trace scaffolding
    // slower even when disabled. Production codegen therefore emits no trace
    // branches at all; the experiment remains opt-in at recompilation time.
    dcrecomp::DCIRFunction hot_loop;
    hot_loop.name = "hot_trace_emit_probe";
    hot_loop.entry = 0x9000;
    dcrecomp::DCIRBlock hot_a; hot_a.start_address = 0x9000;
    dcrecomp::DCIRInstruction hot_add; hot_add.op = dcrecomp::DCIROp::AddImm; hot_add.source_address = 0x9000; hot_add.dst = 2; hot_add.immediate = 1;
    dcrecomp::DCIRInstruction hot_br; hot_br.op = dcrecomp::DCIROp::Branch; hot_br.source_address = 0x9002; hot_br.target = 0x9000;
    hot_a.instructions = {hot_add, hot_br};
    hot_loop.blocks = {hot_a};
    const auto hot_loop_result = dcrecomp::emit_cpp(elf, hot_loop, {out_dir / "hot_trace_default_off", true, false, false});
    const auto hot_loop_source = read_all(hot_loop_result.function_source);
    require(hot_loop_source.find("htr_profile_count") == std::string::npos &&
            hot_loop_source.find("htr_tick") == std::string::npos,
            "0.0.170 production codegen keeps adaptive hot-trace scaffolding removed");
#if defined(_WIN32)
    _putenv_s("DCR_EMIT_HOT_TRACE", "1");
#else
    setenv("DCR_EMIT_HOT_TRACE", "1", 1);
#endif
    const auto hot_loop_exp_result = dcrecomp::emit_cpp(elf, hot_loop, {out_dir / "hot_trace_experiment", true, false, false});
#if defined(_WIN32)
    _putenv_s("DCR_EMIT_HOT_TRACE", "");
#else
    unsetenv("DCR_EMIT_HOT_TRACE");
#endif
    const auto hot_loop_exp_source = read_all(hot_loop_exp_result.function_source);
    require(hot_loop_exp_source.find("htr_profile_count") != std::string::npos &&
            hot_loop_exp_source.find("htr_tick") != std::string::npos,
            "0.0.170 retains adaptive hot trace only as explicit codegen experiment");
    require(runtime.find("gpr-cache=") != std::string::npos &&
            runtime.find("sq-zero=") != std::string::npos &&
            runtime.find("pvr_submit_packet_staged") != std::string::npos &&
            runtime.find("sq-fuse=") != std::string::npos &&
            runtime_header.find("SQTAFusedCapture") != std::string::npos &&
            runtime_header.find("sq_ta_fusion_sites") != std::string::npos,
            "0.0.170 runtime exposes GPR-cache and zero-copy SQ-fusion telemetry");

    std::cout << "C++ emitter/runtime tests: PASS\n";
    return 0;
}
