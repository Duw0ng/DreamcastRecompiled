// Regression tests for the SH-4 interpreter fallback (interp-fallback-v1).
//
//  1. Differential corpus: real GCC-compiled SH-4 code (integer, 64-bit,
//     libgcc division, jump tables, recursion, function pointers, bitfields,
//     single/double FPU with FPSCR.PR switching) runs through
//     call_recompiled() with no AOT code at all. Results must match the same C
//     source compiled for the host.
//  2. Mixed mode: some functions are registered as "AOT" (native C++ with the
//     generated-code contract). Interpreted -> AOT calls/tail branches and
//     AOT -> interpreted calls must produce the same results.
//  3. Hand-written probes: delay slots, GBR forms, MAC.W/MAC.L (+saturation),
//     an interrupt taken mid-function (unwind + resume), SLEEP and TRAPA.
//  4. Seed file: verified entries are written, continuations are not, and
//     code that differs from the boot image is reported as runtime-code.
//  5. DCR_SH4_INTERP disabled keeps the historical exception.

#include "dc_runtime.hpp"
#include "dc_sh4_interp.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../tests/interp/interp_fixtures.inc"

// Host reference build of the very same corpus.
namespace host_ref {
#if defined(_MSC_VER)
#define __attribute__(x)
#define __builtin_sqrtf(x) std::sqrt(static_cast<float>(x))
#define __builtin_sqrt(x) std::sqrt(static_cast<double>(x))
#define __builtin_fabsf(x) std::fabs(static_cast<float>(x))
#endif
#include "../tests/interp/interp_corpus.c"
#if defined(_MSC_VER)
#undef __attribute__
#endif
} // namespace host_ref

using namespace dcrecomp_generated;

namespace {

constexpr std::uint32_t kSentinel = 0x8CFFFFF0u;
constexpr std::uint32_t kStackTop = 0x8C800000u;
constexpr std::uint32_t kOutBuffer = 0x8C900000u;
constexpr std::uint32_t kCounter = 0x8C900100u;
constexpr std::uint32_t kScratch = 0x8C900200u;
constexpr std::uint32_t kVbr = kProbeBase + 0x10000u;

int g_failures = 0;
int g_checks = 0;

void check(bool ok, const std::string& what) {
    ++g_checks;
    if (!ok) {
        ++g_failures;
        std::cerr << "[FAIL] " << what << "\n";
    }
}

std::string hex(std::uint32_t v) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "0x%08X", v);
    return buf;
}

struct Machine {
    std::unique_ptr<DCRuntime> rt = std::make_unique<DCRuntime>();
    SH4Context ctx{};

    explicit Machine(const std::string& seeds = std::string(), bool enabled = true) {
        dc_sh4_interp_reset(*rt);
        dc_sh4_interp_configure(*rt, enabled, seeds, false);
        dc_load_bytes(*rt, kCorpusBase, kCorpusImage, sizeof(kCorpusImage));
        dc_load_bytes(*rt, kProbeBase, kProbeImage, sizeof(kProbeImage));
        ctx.sr = 0x400000F0u;          // privileged, interrupts masked
        dc_write_fpscr(ctx, 0x00080000u); // GCC -m4 ABI: double precision
        ctx.vbr = kVbr;
    }
    ~Machine() { dc_sh4_interp_reset(*rt); }

    // The generated runner's top-level trampoline: keep dispatching ctx.pc
    // after an interrupt/exception unwound the C++ stack.
    std::uint32_t call(std::uint32_t entry, std::uint32_t r4 = 0u) {
        ctx.r[4] = r4;
        ctx.r[15] = kStackTop;
        ctx.pr = kSentinel;
        rt->sh4_async_redirect = false;
        call_recompiled(ctx, *rt, entry);
        unsigned guard = 0u;
        while (ctx.pc != kSentinel) {
            if (++guard > 100000u) throw std::runtime_error("trampoline did not reach the sentinel");
            const std::uint32_t next = ctx.pc;
            rt->sh4_async_redirect = false;
            call_recompiled(ctx, *rt, next);
        }
        return ctx.r[0];
    }
    std::uint32_t r32(std::uint32_t a) { return dc_read32(*rt, a); }
    void w32(std::uint32_t a, std::uint32_t v) { dc_write32(*rt, a, v); }
    void w16(std::uint32_t a, std::uint16_t v) { dc_write16(*rt, a, v); }
};

std::vector<std::uint32_t> host_results() {
    std::vector<std::uint32_t> out(DC_COUNT);
    host_ref::dc_test_main(out.data());
    return out;
}

std::vector<std::uint32_t> guest_results(Machine& m) {
    m.call(kCorpus_dc_test_main, kOutBuffer);
    std::vector<std::uint32_t> out(DC_COUNT);
    for (std::uint32_t i = 0; i < DC_COUNT; ++i) out[i] = m.r32(kOutBuffer + i * 4u);
    return out;
}

void compare(const std::vector<std::uint32_t>& expected, const std::vector<std::uint32_t>& got, const char* label) {
    static const char* names[] = {
        "arith", "div", "wide.lo", "wide.hi", "switch-1", "switch0", "switch1", "switch2", "switch3", "switch4",
        "switch5", "switch6", "switch7", "switch8", "switch9", "switch10", "switch11", "switch12", "switch13",
        "switch14", "fib", "ackermann", "sort", "fnptr", "struct", "bytes", "float", "double", "convert",
        "long_loop", "apply", "mix+apply", "marker"};
    for (std::size_t i = 0; i < expected.size(); ++i) {
        const std::string name = i < sizeof(names) / sizeof(names[0]) ? names[i] : ("slot" + std::to_string(i));
        check(expected[i] == got[i], std::string(label) + " " + name + ": expected " + hex(expected[i]) +
                                         " got " + hex(got[i]));
    }
}

// ---- "AOT" stand-ins with the generated-function contract ----------------

void aot_dc_mix(SH4Context& ctx, DCRuntime&) {
    ctx.r[0] = host_ref::dc_mix(ctx.r[4], ctx.r[5]);
    ctx.pc = ctx.pr;
}

// dc_apply(fn, x) = fn(x) + fn(x + 1) * 3, calling back into guest code the
// way generated code does (set PR, call_recompiled, verify the return PC).
void aot_dc_apply(SH4Context& ctx, DCRuntime& rt) {
    const std::uint32_t saved_pr = ctx.pr;
    const std::uint32_t fn = ctx.r[4];
    const std::uint32_t x = ctx.r[5];
    const std::uint32_t ret = kCorpus_dc_apply + 0x10u; // any in-function return site
    ctx.r[4] = x;
    ctx.pr = ret;
    call_recompiled(ctx, rt, fn);
    if (rt.sh4_async_redirect || ctx.pc != ret) return;
    const std::uint32_t first = ctx.r[0];
    ctx.r[4] = x + 1u;
    ctx.pr = ret;
    call_recompiled(ctx, rt, fn);
    if (rt.sh4_async_redirect || ctx.pc != ret) return;
    ctx.r[0] = first + ctx.r[0] * 3u;
    ctx.pc = saved_pr;
}

void test_differential() {
    const auto expected = host_results();
    check(expected[32] == 0x600DF00Du, "host reference marker");
    Machine m;
    const auto got = guest_results(m);
    compare(expected, got, "interp-only");
    const auto stats = dc_sh4_interp_stats(*m.rt);
    check(stats.instructions > 100000u, "interpreter executed the corpus");
    check(stats.aot_calls == 0u && stats.aot_tail_branches == 0u, "no AOT in interp-only mode");
    std::cout << "[INFO] corpus: " << stats.instructions << " SH-4 instructions, "
              << stats.function_entries << " interpreter entries, " << stats.unique_entries << " unique targets\n";
}

void test_mixed_mode() {
    const auto expected = host_results();
    Machine m;
    m.rt->register_target(kCorpus_dc_mix, &aot_dc_mix);
    m.rt->register_target(kCorpus_dc_apply, &aot_dc_apply);
    const auto got = guest_results(m);
    compare(expected, got, "mixed");
    const auto stats = dc_sh4_interp_stats(*m.rt);
    check(stats.aot_calls > 0u, "interpreted code called AOT code");
    check(stats.aot_tail_branches > 0u, "interpreted code tail-branched into AOT code (op_mix -> dc_mix)");
}

void test_delay_slots() {
    Machine m;
    check(m.call(kProbe_probe_delay) == 0x36C0u, "delay-slot probe returns 0x36C0 (got " + hex(m.ctx.r[0]) + ")");
}

void test_gbr() {
    Machine m;
    const std::uint32_t r0 = m.call(kProbe_probe_gbr, kScratch);
    check(r0 == 0x89ABCDCAu, "GBR probe result (got " + hex(r0) + ")");
    check(m.ctx.gbr == kScratch, "LDC Rn,GBR");
    check(dc_read8(*m.rt, kScratch + 1u) == 0xDCu, "OR.B/AND.B/XOR.B @(R0,GBR)");
    check(dc_read16(*m.rt, kScratch + 4u) == 0xFFFEu, "MOV.W R0,@(disp,GBR)");
}

void test_mac() {
    Machine m;
    m.w16(kScratch + 0u, 0x7FFFu);
    m.w16(kScratch + 2u, 0xFFFDu); // -3
    m.w16(kScratch + 4u, 0x7FFFu);
    m.w16(kScratch + 6u, 1000u);
    m.w32(kScratch + 16u, 0x7FFFFFFFu);
    m.w32(kScratch + 20u, 0xFFFFFFFBu); // -5
    m.w32(kScratch + 24u, 0x7FFFFFFFu);
    m.w32(kScratch + 28u, 7u);
    check(m.call(kProbe_probe_mac, kScratch) == 77u, "MAC probe completed");
    const std::int64_t macw = static_cast<std::int64_t>(0x7FFF) * 0x7FFF + static_cast<std::int64_t>(-3) * 1000;
    const std::uint64_t macl_plain = static_cast<std::uint64_t>(
        static_cast<std::int64_t>(0x7FFFFFFF) * 0x7FFFFFFF + static_cast<std::int64_t>(-5) * 7);
    const std::uint64_t macl_sat = 0x00007FFFFFFFFFFFull - 35u; // saturated, then -35
    const std::uint32_t res = kScratch + 32u;
    check(m.r32(res + 0u) == static_cast<std::uint32_t>(static_cast<std::uint64_t>(macw) >> 32), "MAC.W S=0 MACH");
    check(m.r32(res + 4u) == static_cast<std::uint32_t>(macw), "MAC.W S=0 MACL");
    check(m.r32(res + 8u) == static_cast<std::uint32_t>(macl_plain >> 32), "MAC.L S=0 MACH");
    check(m.r32(res + 12u) == static_cast<std::uint32_t>(macl_plain), "MAC.L S=0 MACL");
    check(m.r32(res + 16u) == static_cast<std::uint32_t>(macl_sat >> 32), "MAC.L S=1 MACH (48-bit saturation) got " + hex(m.r32(res + 16u)));
    check(m.r32(res + 20u) == static_cast<std::uint32_t>(macl_sat), "MAC.L S=1 MACL got " + hex(m.r32(res + 20u)));
    check(m.r32(res + 24u) == 1u, "MAC.W S=1 overflow flag in MACH");
    check(m.r32(res + 28u) == 0x7FFFFFFFu, "MAC.W S=1 saturates MACL");
}

void arm_bank_b_event(Machine& m) {
    m.ctx.sr = 0x40000000u;                 // MD=1, BL=0, IMASK=0
    m.rt->holly_pending[1] |= 1u;            // bank B bit 0 pending, still masked
    m.w32(kCounter, 0u);
}

void test_interrupt_mid_function() {
    Machine m;
    arm_bank_b_event(m);
    const std::uint32_t r0 = m.call(kProbe_probe_irq);
    check(r0 == 150u, "loop result survives an interrupt (got " + std::to_string(r0) + ")");
    check(m.r32(kCounter) == 1u, "interrupt handler ran exactly once (got " + std::to_string(m.r32(kCounter)) + ")");
    check((m.rt->holly_pending[1] & 1u) == 0u, "handler acknowledged the event");
    check(dc_sh4_interp_stats(*m.rt).interrupts >= 1u, "interrupt counted");
}

void test_sleep() {
    Machine m;
    arm_bank_b_event(m);
    check(m.call(kProbe_probe_sleep) == 42u, "SLEEP resumes after the interrupt");
    check(m.r32(kCounter) == 1u, "SLEEP woke through the interrupt handler");
}

void test_trapa() {
    Machine m;
    m.w32(kCounter + 4u, 0u);
    check(m.call(kProbe_probe_trapa) == 8u, "TRAPA resumes after RTE with banked registers restored");
    check(m.r32(kCounter + 4u) == (0x21u << 2u), "TRA register holds imm<<2 (got " + hex(m.r32(kCounter + 4u)) + ")");
}

std::string read_file(const std::filesystem::path& p) {
    std::ifstream in(p);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

void test_seed_file() {
    const auto dir = std::filesystem::temp_directory_path() / "dcr_interp_seed_test";
    std::filesystem::create_directories(dir);
    const auto seeds = dir / "interp_seeds.txt";
    std::filesystem::remove(seeds);
    {
        Machine m(seeds.string());
        dc_sh4_interp_set_reference_image(*m.rt, kCorpusBase, kCorpusImage, sizeof(kCorpusImage));
        // Same bytes copied outside the boot image: runtime-only code.
        std::vector<std::uint8_t> copy(64);
        for (std::size_t i = 0; i < copy.size(); ++i) copy[i] = dc_read8(*m.rt, kCorpus_dc_square + static_cast<std::uint32_t>(i));
        dc_load_bytes(*m.rt, 0x8C200000u, copy.data(), copy.size());
        m.call(kCorpus_dc_test_main, kOutBuffer);
        check(m.call(0x8C200000u, 9u) == 82u, "relocated copy of dc_square executes");
        m.call(kProbe_probe_delay);
    }
    const std::string text = read_file(seeds);
    auto has = [&](const std::string& needle) { return text.find(needle) != std::string::npos; };
    char line[32];
    std::snprintf(line, sizeof(line), "0x%08X  #", kCorpus_dc_test_main);
    check(has(line), "entry function seeded");
    std::snprintf(line, sizeof(line), "0x%08X  #", kCorpus_dc_square);
    check(has(line), "function-pointer target seeded");
    check(has("# runtime-code 0x8C200000"), "code outside the boot image is reported, not seeded");
    // The probe image is not part of the reference image either.
    std::snprintf(line, sizeof(line), "# runtime-code 0x%08X", kProbe_probe_delay);
    check(has(line), "probe outside reference image reported as runtime-code");
    std::size_t active = 0;
    std::istringstream lines(text);
    std::string l;
    while (std::getline(lines, l)) {
        if (l.rfind("0x", 0) == 0) {
            ++active;
            const auto addr = static_cast<std::uint32_t>(std::stoul(l, nullptr, 16));
            check(addr < kProbeBase, "no probe address among active corpus seeds: " + l);
        }
    }
    check(active >= 10u, "seed file has the corpus entrypoints (" + std::to_string(active) + ")");

    // Second run: nothing new is appended.
    const auto size_before = std::filesystem::file_size(seeds);
    {
        Machine m(seeds.string());
        dc_sh4_interp_set_reference_image(*m.rt, kCorpusBase, kCorpusImage, sizeof(kCorpusImage));
        m.call(kCorpus_dc_test_main, kOutBuffer);
    }
    check(std::filesystem::file_size(seeds) == size_before, "seed file is de-duplicated across runs");

    // With the probe image as reference, the interrupted function and the IRQ
    // vector are real entrypoints; the resume address after the interrupt is a
    // continuation and must not be seeded.
    std::filesystem::remove(seeds);
    {
        Machine m(seeds.string());
        dc_sh4_interp_set_reference_image(*m.rt, kProbeBase, kProbeImage, sizeof(kProbeImage));
        arm_bank_b_event(m);
        check(m.call(kProbe_probe_irq) == 150u, "probe_irq result with seed logging");
    }
    {
        std::istringstream probe_lines(read_file(seeds));
        std::size_t probe_active = 0;
        while (std::getline(probe_lines, l)) {
            if (l.rfind("0x", 0) != 0) continue;
            ++probe_active;
            const auto addr = static_cast<std::uint32_t>(std::stoul(l, nullptr, 16));
            check(addr == kProbe_probe_irq || addr == kVbr + 0x600u,
                  "only real entrypoints are seeded (unexpected " + l + ")");
        }
        check(probe_active == 2u, "probe_irq and the IRQ vector are seeded (" + std::to_string(probe_active) + ")");
    }

    std::filesystem::remove_all(dir);
}

void test_dynamic_snapshot_capture() {
    Machine m;
    const auto dir=std::filesystem::temp_directory_path()/"dcr_dynamic_snapshot_test";
    std::filesystem::remove_all(dir);
    dc_sh4_interp_configure_dynamic(*m.rt,true,dir.string());
    const std::uint32_t entry=0x8C220000u;
    const std::uint8_t code[] = {0x09,0x00,0x0B,0x00,0x09,0x00};
    dc_load_bytes(*m.rt,entry,code,sizeof(code));
    dc_sh4_interp_note_entry(*m.rt,entry,0x8C010000u);
    std::size_t count=0;
    for(const auto& file:std::filesystem::directory_iterator(dir)) {
        ++count;
        check(file.path().extension()==".bin","dynamic snapshot binary");
        check(file.file_size()==4096u,"dynamic snapshot captures full 4KiB page");
        check(file.path().filename().string().find("snapshot_8C220000_8C220000_")==0,
              "dynamic snapshot records entry and base");
    }
    check(count==1,"first dynamic snapshot stored");
    dc_sh4_interp_note_entry(*m.rt,entry,0x8C010000u);
    count=std::distance(std::filesystem::directory_iterator(dir),std::filesystem::directory_iterator{});
    check(count==1,"unchanged snapshot deduplicated");
    dc_write8(*m.rt,entry,0x1Bu);
    dc_sh4_interp_note_entry(*m.rt,entry,0x8C010000u);
    count=std::distance(std::filesystem::directory_iterator(dir),std::filesystem::directory_iterator{});
    check(count==2,"changed RAM captures a distinct content variant");
    std::filesystem::remove_all(dir);
}

void test_disabled_keeps_exception() {
    Machine m(std::string(), false);
    bool threw = false;
    try {
        m.call(kCorpus_dc_test_main, kOutBuffer);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    check(threw, "DCR_SH4_INTERP=0 keeps the historical missing-target exception");
}

void test_illegal_instruction_reports() {
    Machine m;
    // 0xFFFF is not an SH-4 instruction.
    const std::uint8_t bad[] = {0xFF, 0xFF, 0x09, 0x00};
    dc_load_bytes(*m.rt, 0x8C210000u, bad, sizeof(bad));
    bool threw = false;
    try {
        m.call(0x8C210000u);
    } catch (const std::runtime_error& e) {
        threw = std::string(e.what()).find("0x8C210000") != std::string::npos;
    }
    check(threw, "illegal instruction reports its address");
}

void test_error_context_and_scope() {
    {
        Machine m;
        const std::uint8_t bad[] = {0xFF, 0xFF, 0x09, 0x00};
        dc_load_bytes(*m.rt, 0x8C210000u, bad, sizeof(bad));
        std::string what;
        try { m.call(0x8C210000u); } catch (const std::runtime_error& e) { what = e.what(); }
        check(what.find("| entry 0x8C210000") != std::string::npos, "illegal instruction names the interpreted entry: " + what);
    }
    // Targets outside guest main RAM keep the historical missing-target error
    // instead of interpreting BIOS/MMIO/unmapped memory.
    for (const std::uint32_t target : {0x00001000u, 0xA0001000u, 0xFF000000u, 0x8C010001u}) {
        Machine m;
        bool threw = false;
        try { m.call(target); } catch (const std::runtime_error& e) {
            threw = std::string(e.what()).find("SH4-INTERP") == std::string::npos;
        }
        check(threw, "no interpretation outside main RAM / odd target: " + hex(target));
        check(dc_sh4_interp_stats(*m.rt).function_entries == 0u, "interpreter not entered for " + hex(target));
    }
}

} // namespace

int main() {
    try {
        test_differential();
        test_mixed_mode();
        test_delay_slots();
        test_gbr();
        test_mac();
        test_interrupt_mid_function();
        test_sleep();
        test_trapa();
        test_seed_file();
        test_dynamic_snapshot_capture();
        test_disabled_keeps_exception();
        test_illegal_instruction_reports();
        test_error_context_and_scope();
    } catch (const std::exception& e) {
        std::cerr << "[FAIL] unexpected exception: " << e.what() << "\n";
        return 1;
    }
    if (g_failures != 0) {
        std::cerr << "[FAIL] SH-4 interpreter: " << g_failures << " of " << g_checks << " checks failed\n";
        return 1;
    }
    std::cout << "[PASS] SH-4 interpreter fallback: " << g_checks << " checks\n";
    return 0;
}
