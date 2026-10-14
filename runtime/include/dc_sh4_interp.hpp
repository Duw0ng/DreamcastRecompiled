#pragma once

// SH-4 interpreter fallback (v1.2 cpp38e "interp-fallback-v1").
//
// The AOT pipeline only knows the SH-4 entrypoints its static closure found.
// Before this module, any dynamic jump/call to an unknown address made
// call_recompiled() throw and the game stopped. The interpreter executes such
// code directly from guest memory with the same execution contract as a
// generated function, so it can be stored in the dispatch cache and mixed
// freely with AOT code:
//
//   * entry:  ctx.pc holds the first instruction to execute;
//   * exit:   RTS leaves ctx.pc = PR, RTE leaves ctx.pc = SPC, a tail branch
//             into AOT code returns after the AOT callee returns;
//   * every basic block boundary calls dc_runtime_tick_fast(), so interrupts
//             and async redirects (sh4_async_redirect) unwind exactly like AOT.
//
// Nothing here changes the frozen Guest ABI1 (DCRuntime/SH4Context layouts):
// the interpreter's own state lives in a runtime-local side table.
//
// Every new entrypoint whose bytes still match the boot image is appended to a
// per-game seed file (DCR_SH4_INTERP_SEEDS) so the next "Preparar SH-4"
// recompiles it natively. Code that only exists at runtime (overlays loaded
// from disc, generated code) is reported but never seeded, because the static
// recompiler would read different bytes at that address.
//
// Environment:
//   DCR_SH4_INTERP=0          disable the fallback (old behaviour: throw)
//   DCR_SH4_INTERP_SEEDS=path per-game learned seed file (append-only)
//   DCR_SH4_INTERP_TRACE=1    log every new interpreted entrypoint

#include <cstddef>
#include <cstdint>
#include <string>

namespace dcrecomp_generated {

struct SH4Context;
struct DCRuntime;

struct DCSh4InterpStats {
    std::uint64_t function_entries{};    // interpreter invocations
    std::uint64_t instructions{};        // SH-4 instructions executed (incl. delay slots)
    std::uint64_t unique_entries{};      // distinct missing targets routed to the interpreter
    std::uint64_t seeds_written{};       // verified entrypoints appended to the seed file
    std::uint64_t runtime_code_entries{};// entrypoints not present in the boot image
    std::uint64_t aot_calls{};           // calls from interpreted code into AOT code
    std::uint64_t aot_tail_branches{};   // branches from interpreted code into AOT code
    std::uint64_t interrupts{};          // async redirects taken while interpreting
    std::uint64_t sleeps{};
    std::uint64_t traps{};
    std::uint64_t pc_relative_in_delay_slot{};
};

// RecompiledFunction-compatible entry: interprets from ctx.pc.
void dc_sh4_interp_function(SH4Context& ctx, DCRuntime& runtime);

// True unless DCR_SH4_INTERP=0 (or disabled through dc_sh4_interp_configure).
bool dc_sh4_interp_enabled(DCRuntime& runtime);

// True when the fallback is enabled and `target` is an even address in guest
// main RAM. Other targets (BIOS/MMIO/unmapped, odd) keep the historical error.
bool dc_sh4_interp_accepts(DCRuntime& runtime, std::uint32_t target);

// Called by call_recompiled() the first time a missing target is routed to the
// interpreter. Records/validates the entrypoint for the AOT seed file.
void dc_sh4_interp_note_entry(DCRuntime& runtime, std::uint32_t target, std::uint32_t caller_pc);

// The exact bytes the static recompiler sees (BOOTSTRAP.BIN and its base).
// Seeds are only written when guest memory still matches this image.
void dc_sh4_interp_set_reference_image(DCRuntime& runtime, std::uint32_t base,
                                       const std::uint8_t* data, std::size_t size);

// Explicit configuration (tests/tools). Environment variables are read lazily
// on first use when this is never called.
void dc_sh4_interp_configure(DCRuntime& runtime, bool enabled, const std::string& seeds_path, bool trace);

// Explicit optional dynamic snapshot capture, independent of the interpreter
// enable flag and without changing the frozen Guest ABI1 struct.
void dc_sh4_interp_configure_dynamic(DCRuntime& runtime,bool enabled,const std::string& directory);

// Drop all interpreter state for this runtime (tests re-use stack addresses).
void dc_sh4_interp_reset(DCRuntime& runtime);

DCSh4InterpStats dc_sh4_interp_stats(DCRuntime& runtime);

// Defined in dc_runtime.cpp (P1/P2/relocation alias resolution used by
// call_recompiled). Declared here because it is runtime-internal and not part
// of the frozen guest header.
std::uint32_t dc_canonical_code_target(const DCRuntime& runtime, std::uint32_t target);
void dc_sh4_interp_print_stats(DCRuntime& runtime);

} // namespace dcrecomp_generated
