// SH-4 interpreter fallback. See include/dc_sh4_interp.hpp for the contract.
//
// Semantics deliberately mirror the C++ the AOT emitter generates (same
// runtime helpers for memory, FPU banks, SR/FPSCR writes and unimplemented
// traps), so a function behaves the same whether it was recompiled or not.
// Instructions the static decoder does not know yet (MAC.L, MAC.W, the GBR
// addressing forms, SLEEP, TRAPA) are implemented here directly.

#include "dc_sh4_interp.hpp"
#include "dc_runtime.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace dcrecomp_generated {
namespace {

constexpr std::uint32_t kSrT = 1u << 0;
constexpr std::uint32_t kSrS = 1u << 1;
constexpr std::uint32_t kSrQ = 1u << 8;
constexpr std::uint32_t kSrM = 1u << 9;
constexpr std::uint32_t kSrBl = 1u << 28;
constexpr std::uint32_t kSrRb = 1u << 29;
constexpr std::uint32_t kSrMd = 1u << 30;
constexpr std::uint32_t kFpscrPr = 1u << 19;
constexpr std::uint32_t kFpscrSz = 1u << 20;
constexpr std::uint32_t kFpscrFr = 1u << 21;

// Straight-line instructions between scheduler ticks. Generated code ticks once
// per basic block; long blocks are rare, but a bounded run keeps interrupts
// responsive even inside a tight interpreted loop body.
constexpr unsigned kMaxStraightLine = 64u;
// SLEEP polls the scheduler in slices until an interrupt arrives.
constexpr std::uint64_t kSleepSliceCycles = 512u;
constexpr unsigned kSleepMaxSlices = 1u << 18;
// Bytes compared between guest memory and the boot image before seeding.
constexpr std::size_t kSeedVerifyBytes = 32u;
constexpr std::size_t kTargetMemoSize = 4096u;
constexpr unsigned kVerboseEntryLimit = 12u;
// Bounded snapshots of guest RAM. The entire window will be checked again
// before dispatching any compiled dynamic entry, not only its first opcode.
constexpr std::size_t kDynamicSnapshotBytes = 4096u;
constexpr std::size_t kMaxDynamicSnapshotsPerRun = 64u;
constexpr std::size_t kMaxDynamicVariantsPerEntry = 4u;
std::uint64_t fnv64(const std::uint8_t* bytes, std::size_t size) {
    std::uint64_t h = 14695981039346656037ull;
    for (std::size_t i=0; i<size; ++i) { h ^= bytes[i]; h *= 1099511628211ull; }
    return h;
}


bool env_flag(const char* name, bool fallback) {
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') return fallback;
    switch (value[0]) {
        case '0': case 'n': case 'N': case 'f': case 'F': return false;
        default: return true;
    }
}

std::string hex8(std::uint32_t value) {
    std::ostringstream out;
    out << "0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << value;
    return out.str();
}

std::uint32_t main_ram_offset(std::uint32_t address, bool& ok) {
    const std::uint32_t physical = address & 0x1FFFFFFFu;
    ok = physical >= DCRuntime::kMainRamPhysicalBase && physical < 0x10000000u;
    return (physical - DCRuntime::kMainRamPhysicalBase) &
           static_cast<std::uint32_t>(DCRuntime::kMainRamSize - 1u);
}

struct InterpState {
    bool configured{};
    bool enabled{true};
    bool trace{};
    bool dynamic_capture{};
    std::string dynamic_dir;
    std::size_t dynamic_snapshots{};
    std::unordered_map<std::uint32_t,std::size_t> dynamic_variants;
    std::string seeds_path;
    bool seeds_loaded{};
    bool seeds_write_failed{};

    // Addresses execution resumes at after an unwind (interrupt return target,
    // return address of a call made from interpreted code, SLEEP/TRAPA resume).
    // They are continuations, not function entries, and are never seeded.
    std::unordered_set<std::uint32_t> continuations;
    // Entrypoints already reported (this run or loaded from the seed file).
    std::unordered_set<std::uint32_t> known_entries;

    std::vector<std::uint8_t> reference_image;
    std::uint32_t reference_base{};
    bool has_reference{};

    struct TargetMemo {
        std::uint32_t address{0xFFFFFFFFu};
        bool registered{};
    };
    std::array<TargetMemo, kTargetMemoSize> target_memo{};

    DCSh4InterpStats stats{};
    unsigned verbose_entries{};
};

using StateMap = std::unordered_map<const DCRuntime*, std::unique_ptr<InterpState>>;

struct StateRegistry {
    StateMap states;
    ~StateRegistry() {
        // Final summary for interactive runs. Never let diagnostics throw while
        // the runtime DLL is unloading.
        try {
            for (const auto& [rt, st] : states) {
                if (!st || st->stats.function_entries == 0u) continue;
                std::cerr << "[DCR SH4-INTERP] summary: entries=" << st->stats.unique_entries
                          << " seeds-written=" << st->stats.seeds_written
                          << " runtime-code=" << st->stats.runtime_code_entries
                          << " calls=" << st->stats.function_entries
                          << " instructions=" << st->stats.instructions << "\n";
            }
        } catch (...) {
        }
    }
};

StateRegistry& registry() {
    static StateRegistry instance;
    return instance;
}

const DCRuntime* g_last_owner = nullptr;
InterpState* g_last_state = nullptr;

void configure_from_environment(InterpState& st) {
    if (st.configured) return;
    st.configured = true;
    st.enabled = env_flag("DCR_SH4_INTERP", true);
    st.trace = env_flag("DCR_SH4_INTERP_TRACE", false);
    st.dynamic_capture = env_flag("DCR_SH4_DYNAMIC_AOT", false);
    if (const char* path = std::getenv("DCR_SH4_DYNAMIC_DIR"); path && *path)
        st.dynamic_dir = path;
    if (const char* path = std::getenv("DCR_SH4_INTERP_SEEDS"); path != nullptr && *path != '\0')
        st.seeds_path = path;
}

InterpState& state_for(DCRuntime& runtime) {
    if (g_last_owner == &runtime && g_last_state != nullptr) return *g_last_state;
    auto& slot = registry().states[&runtime];
    if (!slot) slot = std::make_unique<InterpState>();
    configure_from_environment(*slot);
    g_last_owner = &runtime;
    g_last_state = slot.get();
    return *slot;
}

std::uint16_t fetch16(DCRuntime& runtime, std::uint32_t pc) {
    // P1/P2 are architecturally untranslated; commercial code lives there.
    if (pc >= 0x80000000u && pc < 0xC0000000u) {
        bool ok = false;
        const std::uint32_t off = main_ram_offset(pc, ok);
        if (ok) {
            return static_cast<std::uint16_t>(runtime.main_ram[off]) |
                   static_cast<std::uint16_t>(static_cast<std::uint16_t>(runtime.main_ram[off + 1u]) << 8u);
        }
    }
    return dc_read16(runtime, pc);
}

std::uint32_t sext8(std::uint32_t v) { return static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int8_t>(v & 0xFFu))); }
std::uint32_t sext16(std::uint32_t v) { return static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(v & 0xFFFFu))); }
std::uint32_t sext12(std::uint32_t v) { v &= 0xFFFu; return (v & 0x800u) ? (v | 0xFFFFF000u) : v; }

void set_t(SH4Context& ctx, bool value) { ctx.sr = (ctx.sr & ~kSrT) | (value ? kSrT : 0u); }
bool get_t(const SH4Context& ctx) { return (ctx.sr & kSrT) != 0u; }

std::uint8_t rd8(DCRuntime& rt, std::uint32_t a) { return dc_read8(rt, a); }
std::uint16_t rd16(DCRuntime& rt, std::uint32_t a) { return dc_read16(rt, a); }
std::uint32_t rd32(DCRuntime& rt, std::uint32_t a) { return dc_read32_hot(rt, a); }
void wr8(DCRuntime& rt, std::uint32_t a, std::uint32_t v) { dc_write8(rt, a, static_cast<std::uint8_t>(v & 0xFFu)); }
void wr16(DCRuntime& rt, std::uint32_t a, std::uint32_t v) { dc_write16(rt, a, static_cast<std::uint16_t>(v & 0xFFFFu)); }
void wr32(DCRuntime& rt, std::uint32_t a, std::uint32_t v) { dc_write32_hot(rt, a, v); }

enum class Kind {
    Normal,        // executed; continue at pc + 2
    CondBranch,    // BT/BF without delay slot
    Delayed,       // control transfer with a delay slot (see Control::kind)
    Sleep,
    Trap,
};

enum class Transfer { Jump, Call, Return, Rte, CondJump };

struct Control {
    Transfer kind{Transfer::Jump};
    std::uint32_t target{};
    std::uint32_t return_pc{};
    bool taken{true};
    std::uint32_t trap_imm{};
};

// Raised for undecodable/illegal instructions so the innermost interpreter
// frame can append where the interpreted path was entered from.
struct IllegalInstruction : std::runtime_error {
    using std::runtime_error::runtime_error;
};

[[noreturn]] void illegal(std::uint32_t pc, std::uint16_t op, const char* why) {
    std::ostringstream out;
    out << "[DCR SH4-INTERP] " << why << " 0x" << std::hex << std::uppercase << std::setw(4)
        << std::setfill('0') << op << " at " << hex8(pc);
    throw IllegalInstruction(out.str());
}

// ---------------------------------------------------------------------------
// FPU helpers mirroring the generated code.
// ---------------------------------------------------------------------------

std::array<std::uint32_t, 16>& active_bank(SH4Context& ctx) {
    return (ctx.fpscr & kFpscrFr) != 0u ? ctx.xf_bits : ctx.fr_bits;
}
const std::array<std::uint32_t, 16>& inactive_bank(SH4Context& ctx) {
    return (ctx.fpscr & kFpscrFr) != 0u ? ctx.fr_bits : ctx.xf_bits;
}

Kind exec_fpu(SH4Context& ctx, DCRuntime& rt, InterpState& st, std::uint32_t pc, std::uint16_t op) {
    (void)st;
    const std::uint8_t n = static_cast<std::uint8_t>((op >> 8u) & 0xFu);
    const std::uint8_t m = static_cast<std::uint8_t>((op >> 4u) & 0xFu);
    auto& r = ctx.r;
    const bool sz = (ctx.fpscr & kFpscrSz) != 0u;
    switch (op & 0xFu) {
        case 0x0: // FADD
            dc_require_fpu_round_nearest(ctx, pc);
            if (dc_fpu_double_precision(ctx)) dc_set_dr_double(ctx, n, dc_get_dr_double(ctx, n) + dc_get_dr_double(ctx, m));
            else dc_set_fr_float(ctx, n, dc_get_fr_float(ctx, n) + dc_get_fr_float(ctx, m));
            return Kind::Normal;
        case 0x1: // FSUB
            dc_require_fpu_round_nearest(ctx, pc);
            if (dc_fpu_double_precision(ctx)) dc_set_dr_double(ctx, n, dc_get_dr_double(ctx, n) - dc_get_dr_double(ctx, m));
            else dc_set_fr_float(ctx, n, dc_get_fr_float(ctx, n) - dc_get_fr_float(ctx, m));
            return Kind::Normal;
        case 0x2: // FMUL
            dc_require_fpu_round_nearest(ctx, pc);
            if (dc_fpu_double_precision(ctx)) dc_set_dr_double(ctx, n, dc_get_dr_double(ctx, n) * dc_get_dr_double(ctx, m));
            else dc_set_fr_float(ctx, n, dc_get_fr_float(ctx, n) * dc_get_fr_float(ctx, m));
            return Kind::Normal;
        case 0x3: // FDIV
            dc_require_fpu_round_nearest(ctx, pc);
            if (dc_fpu_double_precision(ctx)) dc_set_dr_double(ctx, n, dc_get_dr_double(ctx, n) / dc_get_dr_double(ctx, m));
            else dc_set_fr_float(ctx, n, dc_get_fr_float(ctx, n) / dc_get_fr_float(ctx, m));
            return Kind::Normal;
        case 0x4: { // FCMP/EQ
            dc_require_fpu_round_nearest(ctx, pc);
            const bool result = dc_fpu_double_precision(ctx)
                ? dc_get_dr_double(ctx, n) == dc_get_dr_double(ctx, m)
                : dc_get_fr_float(ctx, n) == dc_get_fr_float(ctx, m);
            set_t(ctx, result);
            return Kind::Normal;
        }
        case 0x5: { // FCMP/GT
            dc_require_fpu_round_nearest(ctx, pc);
            const bool result = dc_fpu_double_precision(ctx)
                ? dc_get_dr_double(ctx, n) > dc_get_dr_double(ctx, m)
                : dc_get_fr_float(ctx, n) > dc_get_fr_float(ctx, m);
            set_t(ctx, result);
            return Kind::Normal;
        }
        case 0x6: { // FMOV @(R0,Rm),FRn
            const std::uint32_t address = r[0] + r[m];
            if (!sz) dc_set_fr_bits(ctx, n, rd32(rt, address));
            else dc_set_fmov64_bits(ctx, n, dc_read64_fmov_hot(rt, address));
            return Kind::Normal;
        }
        case 0x7: { // FMOV FRm,@(R0,Rn)
            const std::uint32_t address = r[n] + r[0];
            if (!sz) wr32(rt, address, dc_get_fr_bits(ctx, m));
            else dc_write64_fmov_hot(rt, address, dc_get_fmov64_bits(ctx, m));
            return Kind::Normal;
        }
        case 0x8: // FMOV @Rm,FRn
            if (!sz) dc_set_fr_bits(ctx, n, rd32(rt, r[m]));
            else dc_set_fmov64_bits(ctx, n, dc_read64_fmov_hot(rt, r[m]));
            return Kind::Normal;
        case 0x9: // FMOV @Rm+,FRn
            if (!sz) { dc_set_fr_bits(ctx, n, rd32(rt, r[m])); r[m] += 4u; }
            else { dc_set_fmov64_bits(ctx, n, dc_read64_fmov_hot(rt, r[m])); r[m] += 8u; }
            return Kind::Normal;
        case 0xA: // FMOV FRm,@Rn
            if (!sz) wr32(rt, r[n], dc_get_fr_bits(ctx, m));
            else dc_write64_fmov_hot(rt, r[n], dc_get_fmov64_bits(ctx, m));
            return Kind::Normal;
        case 0xB: // FMOV FRm,@-Rn
            if (!sz) { r[n] -= 4u; wr32(rt, r[n], dc_get_fr_bits(ctx, m)); }
            else { r[n] -= 8u; dc_write64_fmov_hot(rt, r[n], dc_get_fmov64_bits(ctx, m)); }
            return Kind::Normal;
        case 0xC: // FMOV FRm,FRn
            if (!sz) dc_set_fr_bits(ctx, n, dc_get_fr_bits(ctx, m));
            else dc_set_fmov64_bits(ctx, n, dc_get_fmov64_bits(ctx, m));
            return Kind::Normal;
        case 0xE: { // FMAC FR0,FRm,FRn
            dc_require_fpu_round_nearest(ctx, pc);
            if ((ctx.fpscr & kFpscrPr) != 0u) dc_unimplemented(pc, "FMAC is only available when FPSCR.PR=0");
            auto& frb = active_bank(ctx);
            const float f0 = std::bit_cast<float>(frb[0u]);
            const float fs = std::bit_cast<float>(frb[m]);
            const float fd = std::bit_cast<float>(frb[n]);
            frb[n] = std::bit_cast<std::uint32_t>(dc_host_fma(f0, fs, fd));
            return Kind::Normal;
        }
        case 0xD:
            break;
        default:
            illegal(pc, op, "unsupported FPU instruction");
    }

    // FnxD family.
    if (op == 0xFBFDu) { ctx.fpscr ^= kFpscrFr; return Kind::Normal; } // FRCHG
    if (op == 0xF3FDu) { ctx.fpscr ^= kFpscrSz; return Kind::Normal; } // FSCHG
    switch (m) {
        case 0x0: dc_set_fr_bits(ctx, n, ctx.fpul); return Kind::Normal;            // FSTS FPUL,FRn
        case 0x1: ctx.fpul = dc_get_fr_bits(ctx, n); return Kind::Normal;           // FLDS FRm,FPUL
        case 0x2: // FLOAT FPUL,FRn/DRn
            dc_require_fpu_round_nearest(ctx, pc);
            if (dc_fpu_double_precision(ctx)) dc_set_dr_double(ctx, n, static_cast<double>(static_cast<std::int32_t>(ctx.fpul)));
            else dc_set_fr_float(ctx, n, static_cast<float>(static_cast<std::int32_t>(ctx.fpul)));
            return Kind::Normal;
        case 0x3: // FTRC FRm/DRm,FPUL
            dc_require_fpu_round_nearest(ctx, pc);
            ctx.fpul = dc_ftrc_to_u32(dc_fpu_double_precision(ctx) ? dc_get_dr_double(ctx, n)
                                                                    : static_cast<double>(dc_get_fr_float(ctx, n)));
            return Kind::Normal;
        case 0x4: // FNEG
            if (dc_fpu_double_precision(ctx)) dc_set_dr_bits(ctx, n, dc_get_dr_bits(ctx, n) ^ 0x8000000000000000ull);
            else dc_set_fr_bits(ctx, n, dc_get_fr_bits(ctx, n) ^ 0x80000000u);
            return Kind::Normal;
        case 0x5: // FABS
            if (dc_fpu_double_precision(ctx)) dc_set_dr_bits(ctx, n, dc_get_dr_bits(ctx, n) & 0x7FFFFFFFFFFFFFFFull);
            else dc_set_fr_bits(ctx, n, dc_get_fr_bits(ctx, n) & 0x7FFFFFFFu);
            return Kind::Normal;
        case 0x6: // FSQRT
            dc_require_fpu_round_nearest(ctx, pc);
            if (dc_fpu_double_precision(ctx)) dc_set_dr_double(ctx, n, std::sqrt(dc_get_dr_double(ctx, n)));
            else dc_set_fr_float(ctx, n, std::sqrt(dc_get_fr_float(ctx, n)));
            return Kind::Normal;
        case 0x7: { // FSRRA
            dc_require_fpu_round_nearest(ctx, pc);
            if ((ctx.fpscr & kFpscrPr) != 0u) dc_unimplemented(pc, "FSRRA requires FPSCR.PR=0");
            auto& frb = active_bank(ctx);
            const float x = std::bit_cast<float>(frb[n]);
            frb[n] = std::bit_cast<std::uint32_t>(1.0f / std::sqrt(x));
            return Kind::Normal;
        }
        case 0x8: // FLDI0
            if (dc_fpu_double_precision(ctx)) dc_unimplemented(pc, "FLDI0 requires FPSCR.PR=0");
            dc_set_fr_bits(ctx, n, 0x00000000u);
            return Kind::Normal;
        case 0x9: // FLDI1
            if (dc_fpu_double_precision(ctx)) dc_unimplemented(pc, "FLDI1 requires FPSCR.PR=0");
            dc_set_fr_bits(ctx, n, 0x3F800000u);
            return Kind::Normal;
        case 0xA: // FCNVSD FPUL,DRn
            if ((n & 1u) != 0u) break;
            dc_require_fpu_round_nearest(ctx, pc);
            if (!dc_fpu_double_precision(ctx)) dc_unimplemented(pc, "FCNVSD requires FPSCR.PR=1");
            dc_set_dr_double(ctx, n, static_cast<double>(std::bit_cast<float>(ctx.fpul)));
            return Kind::Normal;
        case 0xB: // FCNVDS DRm,FPUL
            if ((n & 1u) != 0u) break;
            dc_require_fpu_round_nearest(ctx, pc);
            if (!dc_fpu_double_precision(ctx)) dc_unimplemented(pc, "FCNVDS requires FPSCR.PR=1");
            ctx.fpul = std::bit_cast<std::uint32_t>(static_cast<float>(dc_get_dr_double(ctx, n)));
            return Kind::Normal;
        case 0xE: { // FIPR FVm,FVn
            const unsigned vn = ((op >> 10u) & 3u) * 4u;
            const unsigned vm = ((op >> 8u) & 3u) * 4u;
            dc_require_fpu_round_nearest(ctx, pc);
            if ((ctx.fpscr & kFpscrPr) != 0u) dc_unimplemented(pc, "FIPR requires FPSCR.PR=0");
            auto& frb = active_bank(ctx);
            double sum = 0.0;
            for (unsigned i = 0; i < 4u; ++i)
                sum += static_cast<double>(std::bit_cast<float>(frb[vm + i])) *
                       static_cast<double>(std::bit_cast<float>(frb[vn + i]));
            frb[vn + 3u] = std::bit_cast<std::uint32_t>(static_cast<float>(sum));
            return Kind::Normal;
        }
        case 0xF: {
            if ((op & 0x0100u) == 0u) { // FSCA FPUL,DRn
                const std::uint8_t dn = static_cast<std::uint8_t>(n & 0xEu);
                dc_require_fpu_round_nearest(ctx, pc);
                if (dc_fpu_double_precision(ctx)) dc_unimplemented(pc, "FSCA requires FPSCR.PR=0");
                constexpr double tau = 6.283185307179586476925286766559;
                const double angle = static_cast<double>(ctx.fpul & 0xFFFFu) * tau / 65536.0;
                dc_set_fr_float(ctx, dn, static_cast<float>(std::sin(angle)));
                dc_set_fr_float(ctx, static_cast<std::uint8_t>(dn + 1u), static_cast<float>(std::cos(angle)));
                return Kind::Normal;
            }
            if ((op & 0x0300u) == 0x0100u) { // FTRV XMTRX,FVn
                const unsigned vn = ((op >> 10u) & 3u) * 4u;
                dc_require_fpu_round_nearest(ctx, pc);
                if ((ctx.fpscr & kFpscrPr) != 0u) dc_unimplemented(pc, "FTRV requires FPSCR.PR=0");
                auto& frb = active_bank(ctx);
                const auto& xfb = inactive_bank(ctx);
                double v[4];
                for (unsigned i = 0; i < 4u; ++i) v[i] = std::bit_cast<float>(frb[vn + i]);
                double o[4];
                for (unsigned row = 0; row < 4u; ++row) {
                    o[row] = static_cast<double>(std::bit_cast<float>(xfb[row + 0u])) * v[0] +
                             static_cast<double>(std::bit_cast<float>(xfb[row + 4u])) * v[1] +
                             static_cast<double>(std::bit_cast<float>(xfb[row + 8u])) * v[2] +
                             static_cast<double>(std::bit_cast<float>(xfb[row + 12u])) * v[3];
                }
                for (unsigned row = 0; row < 4u; ++row)
                    frb[vn + row] = std::bit_cast<std::uint32_t>(static_cast<float>(o[row]));
                return Kind::Normal;
            }
            break;
        }
        default:
            break;
    }
    illegal(pc, op, "unsupported FPU instruction");
}

// ---------------------------------------------------------------------------
// One instruction. Control transfers fill `c` and return a non-Normal kind;
// their delay slot is executed by the caller.
// ---------------------------------------------------------------------------

Kind exec_one(SH4Context& ctx, DCRuntime& rt, InterpState& st, std::uint32_t pc,
              std::uint16_t op, bool in_delay_slot, Control& c) {
    auto& r = ctx.r;
    const unsigned n = (op >> 8u) & 0xFu;
    const unsigned m = (op >> 4u) & 0xFu;

    switch (op >> 12u) {
    case 0x0:
        switch (op & 0xFu) {
            case 0x2:
                switch (op & 0xF0u) {
                    case 0x00: r[n] = ctx.sr; return Kind::Normal;   // STC SR,Rn
                    case 0x10: r[n] = ctx.gbr; return Kind::Normal;  // STC GBR,Rn
                    case 0x20: r[n] = ctx.vbr; return Kind::Normal;  // STC VBR,Rn
                    case 0x30: r[n] = ctx.ssr; return Kind::Normal;  // STC SSR,Rn
                    case 0x40: r[n] = ctx.spc; return Kind::Normal;  // STC SPC,Rn
                    default:
                        if ((op & 0x80u) != 0u) { r[n] = ctx.r_bank[(op >> 4u) & 7u]; return Kind::Normal; } // STC Rm_BANK,Rn
                        break;
                }
                break;
            case 0x3:
                switch (op & 0xF0u) {
                    case 0x00: // BSRF Rn
                        if (in_delay_slot) break;
                        c.kind = Transfer::Call; c.target = pc + 4u + r[n]; c.return_pc = pc + 4u;
                        ctx.pr = c.return_pc;
                        return Kind::Delayed;
                    case 0x20: // BRAF Rn
                        if (in_delay_slot) break;
                        c.kind = Transfer::Jump; c.target = pc + 4u + r[n];
                        return Kind::Delayed;
                    case 0x80: // PREF @Rn
                        if ((r[n] & 0xFC000000u) == 0xE0000000u) dc_pref(rt, r[n], pc);
                        return Kind::Normal;
                    case 0x90: case 0xA0: case 0xB0: // OCBI/OCBP/OCBWB: functional no-ops
                        return Kind::Normal;
                    case 0xC0: // MOVCA.L R0,@Rn
                        wr32(rt, r[n], r[0]);
                        return Kind::Normal;
                    default: break;
                }
                break;
            case 0x4: wr8(rt, r[n] + r[0], r[m]); return Kind::Normal;   // MOV.B Rm,@(R0,Rn)
            case 0x5: wr16(rt, r[n] + r[0], r[m]); return Kind::Normal;  // MOV.W Rm,@(R0,Rn)
            case 0x6: wr32(rt, r[n] + r[0], r[m]); return Kind::Normal;  // MOV.L Rm,@(R0,Rn)
            case 0x7: ctx.macl = r[n] * r[m]; return Kind::Normal;        // MUL.L Rm,Rn
            case 0x8:
                switch (op) {
                    case 0x0008: ctx.sr &= ~kSrT; return Kind::Normal;           // CLRT
                    case 0x0018: ctx.sr |= kSrT; return Kind::Normal;            // SETT
                    case 0x0028: ctx.mach = 0u; ctx.macl = 0u; return Kind::Normal; // CLRMAC
                    case 0x0038: dc_unimplemented(pc, "LDTLB requires MMU/TLB model");
                    case 0x0048: ctx.sr &= ~kSrS; return Kind::Normal;           // CLRS
                    case 0x0058: ctx.sr |= kSrS; return Kind::Normal;            // SETS
                    default: break;
                }
                break;
            case 0x9:
                if (op == 0x0009u) return Kind::Normal;                                  // NOP
                if (op == 0x0019u) { ctx.sr &= ~(kSrM | kSrQ | kSrT); return Kind::Normal; } // DIV0U
                if ((op & 0xF0u) == 0x20u) { r[n] = ctx.sr & kSrT; return Kind::Normal; }   // MOVT Rn
                break;
            case 0xA:
                switch (op & 0xF0u) {
                    case 0x00: r[n] = ctx.mach; return Kind::Normal;
                    case 0x10: r[n] = ctx.macl; return Kind::Normal;
                    case 0x20: r[n] = ctx.pr; return Kind::Normal;
                    case 0x30: r[n] = ctx.sgr; return Kind::Normal;  // STC SGR,Rn (0x0n3A)
                    case 0x50: r[n] = ctx.fpul; return Kind::Normal;
                    case 0x60: r[n] = ctx.fpscr; return Kind::Normal;
                    case 0xF0: r[n] = ctx.dbr; return Kind::Normal;  // STC DBR,Rn
                    default: break;
                }
                break;
            case 0xB:
                if (in_delay_slot) break;
                if (op == 0x000Bu) { // RTS
                    c.kind = Transfer::Return; c.target = ctx.pr;
                    return Kind::Delayed;
                }
                if (op == 0x001Bu) return Kind::Sleep; // SLEEP
                if (op == 0x002Bu) { // RTE: SR is restored before the delay slot executes
                    c.kind = Transfer::Rte; c.target = ctx.spc;
                    dc_write_sr(ctx, ctx.ssr);
                    return Kind::Delayed;
                }
                break;
            case 0xC: r[n] = sext8(rd8(rt, r[m] + r[0])); return Kind::Normal;    // MOV.B @(R0,Rm),Rn
            case 0xD: r[n] = sext16(rd16(rt, r[m] + r[0])); return Kind::Normal;  // MOV.W @(R0,Rm),Rn
            case 0xE: r[n] = rd32(rt, r[m] + r[0]); return Kind::Normal;          // MOV.L @(R0,Rm),Rn
            case 0xF: { // MAC.L @Rm+,@Rn+
                const std::uint32_t vn = rd32(rt, r[n]); r[n] += 4u;
                const std::uint32_t vm = rd32(rt, r[m]); r[m] += 4u;
                const std::int64_t product = static_cast<std::int64_t>(static_cast<std::int32_t>(vn)) *
                                             static_cast<std::int64_t>(static_cast<std::int32_t>(vm));
                std::int64_t mac = static_cast<std::int64_t>((static_cast<std::uint64_t>(ctx.mach) << 32u) | ctx.macl);
                mac = static_cast<std::int64_t>(static_cast<std::uint64_t>(mac) + static_cast<std::uint64_t>(product));
                if ((ctx.sr & kSrS) != 0u) {
                    constexpr std::int64_t kMax48 = 0x00007FFFFFFFFFFFll;
                    constexpr std::int64_t kMin48 = -0x0000800000000000ll;
                    if (mac > kMax48) mac = kMax48;
                    else if (mac < kMin48) mac = kMin48;
                }
                ctx.mach = static_cast<std::uint32_t>(static_cast<std::uint64_t>(mac) >> 32u);
                ctx.macl = static_cast<std::uint32_t>(static_cast<std::uint64_t>(mac));
                return Kind::Normal;
            }
            default: break;
        }
        break;

    case 0x1: // MOV.L Rm,@(disp,Rn)
        wr32(rt, r[n] + (op & 0xFu) * 4u, r[m]);
        return Kind::Normal;

    case 0x2:
        switch (op & 0xFu) {
            case 0x0: wr8(rt, r[n], r[m]); return Kind::Normal;
            case 0x1: wr16(rt, r[n], r[m]); return Kind::Normal;
            case 0x2: wr32(rt, r[n], r[m]); return Kind::Normal;
            case 0x4: r[n] -= 1u; wr8(rt, r[n], r[m]); return Kind::Normal;   // same order as the AOT lowering
            case 0x5: r[n] -= 2u; wr16(rt, r[n], r[m]); return Kind::Normal;
            case 0x6: r[n] -= 4u; wr32(rt, r[n], r[m]); return Kind::Normal;
            case 0x7: { // DIV0S Rm,Rn
                const std::uint32_t q = (r[n] >> 31u) & 1u;
                const std::uint32_t mm = (r[m] >> 31u) & 1u;
                ctx.sr = (ctx.sr & ~(kSrM | kSrQ | kSrT)) | (q << 8u) | (mm << 9u) | ((mm != q) ? kSrT : 0u);
                return Kind::Normal;
            }
            case 0x8: set_t(ctx, (r[n] & r[m]) == 0u); return Kind::Normal;
            case 0x9: r[n] &= r[m]; return Kind::Normal;
            case 0xA: r[n] ^= r[m]; return Kind::Normal;
            case 0xB: r[n] |= r[m]; return Kind::Normal;
            case 0xC: { // CMP/STR
                const std::uint32_t x = r[n] ^ r[m];
                set_t(ctx, (x & 0xFF000000u) == 0u || (x & 0x00FF0000u) == 0u ||
                           (x & 0x0000FF00u) == 0u || (x & 0x000000FFu) == 0u);
                return Kind::Normal;
            }
            case 0xD: r[n] = (r[n] >> 16u) | (r[m] << 16u); return Kind::Normal; // XTRCT
            case 0xE: ctx.macl = static_cast<std::uint32_t>(static_cast<std::uint16_t>(r[n])) *
                                 static_cast<std::uint32_t>(static_cast<std::uint16_t>(r[m]));
                      return Kind::Normal; // MULU.W
            case 0xF: ctx.macl = static_cast<std::uint32_t>(
                          static_cast<std::int32_t>(static_cast<std::int16_t>(r[n])) *
                          static_cast<std::int32_t>(static_cast<std::int16_t>(r[m])));
                      return Kind::Normal; // MULS.W
            default: break;
        }
        break;

    case 0x3:
        switch (op & 0xFu) {
            case 0x0: set_t(ctx, r[n] == r[m]); return Kind::Normal;
            case 0x2: set_t(ctx, r[n] >= r[m]); return Kind::Normal;
            case 0x3: set_t(ctx, static_cast<std::int32_t>(r[n]) >= static_cast<std::int32_t>(r[m])); return Kind::Normal;
            case 0x4: { // DIV1 Rm,Rn (AOT lowering, plus the Rn==Rm case below)
                const std::uint32_t old_q = (ctx.sr >> 8u) & 1u;
                const std::uint32_t mm = (ctx.sr >> 9u) & 1u;
                std::uint32_t q = (r[n] >> 31u) & 1u;
                std::uint32_t nn = (r[n] << 1u) | (ctx.sr & 1u);
                // Architecturally Rm is read after Rn was shifted; that only
                // matters for DIV1 Rn,Rn (verified against qemu-sh4).
                const std::uint32_t d = n == m ? nn : r[m];
                std::uint32_t tmp0;
                bool tmp1;
                if (old_q == 0u) {
                    if (mm == 0u) { tmp0 = nn; nn -= d; tmp1 = nn > tmp0; q = (q == 0u) ? static_cast<std::uint32_t>(tmp1) : static_cast<std::uint32_t>(!tmp1); }
                    else { tmp0 = nn; nn += d; tmp1 = nn < tmp0; q = (q == 0u) ? static_cast<std::uint32_t>(!tmp1) : static_cast<std::uint32_t>(tmp1); }
                } else {
                    if (mm == 0u) { tmp0 = nn; nn += d; tmp1 = nn < tmp0; q = (q == 0u) ? static_cast<std::uint32_t>(tmp1) : static_cast<std::uint32_t>(!tmp1); }
                    else { tmp0 = nn; nn -= d; tmp1 = nn > tmp0; q = (q == 0u) ? static_cast<std::uint32_t>(!tmp1) : static_cast<std::uint32_t>(tmp1); }
                }
                r[n] = nn;
                ctx.sr = (ctx.sr & ~(kSrQ | kSrT)) | (q << 8u) | ((q == mm) ? kSrT : 0u);
                return Kind::Normal;
            }
            case 0x5: { // DMULU.L
                const std::uint64_t p = static_cast<std::uint64_t>(r[n]) * static_cast<std::uint64_t>(r[m]);
                ctx.macl = static_cast<std::uint32_t>(p); ctx.mach = static_cast<std::uint32_t>(p >> 32u);
                return Kind::Normal;
            }
            case 0x6: set_t(ctx, r[n] > r[m]); return Kind::Normal;
            case 0x7: set_t(ctx, static_cast<std::int32_t>(r[n]) > static_cast<std::int32_t>(r[m])); return Kind::Normal;
            case 0x8: r[n] -= r[m]; return Kind::Normal;
            case 0xA: { // SUBC
                const std::uint32_t old_n = r[n];
                const std::uint32_t tmp = old_n - r[m];
                const std::uint32_t result = tmp - (ctx.sr & 1u);
                const bool b1 = old_n < tmp, b2 = tmp < result;
                r[n] = result; set_t(ctx, b1 || b2);
                return Kind::Normal;
            }
            case 0xB: { // SUBV
                const std::int32_t a = static_cast<std::int32_t>(r[n]);
                const std::int32_t b = static_cast<std::int32_t>(r[m]);
                const std::uint32_t result = r[n] - r[m];
                const std::int32_t s = static_cast<std::int32_t>(result);
                r[n] = result;
                set_t(ctx, (a >= 0 && b < 0 && s < 0) || (a < 0 && b >= 0 && s >= 0));
                return Kind::Normal;
            }
            case 0xC: r[n] += r[m]; return Kind::Normal;
            case 0xD: { // DMULS.L
                const std::int64_t p = static_cast<std::int64_t>(static_cast<std::int32_t>(r[n])) *
                                       static_cast<std::int64_t>(static_cast<std::int32_t>(r[m]));
                const auto bits = static_cast<std::uint64_t>(p);
                ctx.macl = static_cast<std::uint32_t>(bits); ctx.mach = static_cast<std::uint32_t>(bits >> 32u);
                return Kind::Normal;
            }
            case 0xE: { // ADDC
                const std::uint32_t old_n = r[n];
                const std::uint32_t tmp = old_n + r[m];
                const std::uint32_t result = tmp + (ctx.sr & 1u);
                const bool c1 = tmp < old_n, c2 = result < tmp;
                r[n] = result; set_t(ctx, c1 || c2);
                return Kind::Normal;
            }
            case 0xF: { // ADDV
                const std::int32_t a = static_cast<std::int32_t>(r[n]);
                const std::int32_t b = static_cast<std::int32_t>(r[m]);
                const std::uint32_t result = r[n] + r[m];
                const std::int32_t s = static_cast<std::int32_t>(result);
                r[n] = result;
                set_t(ctx, (a >= 0 && b >= 0 && s < 0) || (a < 0 && b < 0 && s >= 0));
                return Kind::Normal;
            }
            default: break;
        }
        break;

    case 0x4:
        switch (op & 0xFFu) {
            case 0x00: case 0x20: { const std::uint32_t old = r[n]; set_t(ctx, (old >> 31u) != 0u); r[n] = old << 1u; return Kind::Normal; } // SHLL/SHAL
            case 0x01: { const std::uint32_t old = r[n]; set_t(ctx, (old & 1u) != 0u); r[n] = old >> 1u; return Kind::Normal; }              // SHLR
            case 0x21: { const std::uint32_t old = r[n]; set_t(ctx, (old & 1u) != 0u);
                         r[n] = static_cast<std::uint32_t>(static_cast<std::int32_t>(old) >> 1); return Kind::Normal; }                     // SHAR
            case 0x04: { const std::uint32_t old = r[n]; const std::uint32_t bit = old >> 31u; r[n] = (old << 1u) | bit; set_t(ctx, bit != 0u); return Kind::Normal; } // ROTL
            case 0x05: { const std::uint32_t old = r[n]; const std::uint32_t bit = old & 1u; r[n] = (old >> 1u) | (bit << 31u); set_t(ctx, bit != 0u); return Kind::Normal; } // ROTR
            case 0x24: { const std::uint32_t old = r[n]; const std::uint32_t t = ctx.sr & 1u; r[n] = (old << 1u) | t; set_t(ctx, (old >> 31u) != 0u); return Kind::Normal; } // ROTCL
            case 0x25: { const std::uint32_t old = r[n]; const std::uint32_t t = ctx.sr & 1u; r[n] = (old >> 1u) | (t << 31u); set_t(ctx, (old & 1u) != 0u); return Kind::Normal; } // ROTCR
            case 0x08: r[n] <<= 2u; return Kind::Normal;
            case 0x09: r[n] >>= 2u; return Kind::Normal;
            case 0x18: r[n] <<= 8u; return Kind::Normal;
            case 0x19: r[n] >>= 8u; return Kind::Normal;
            case 0x28: r[n] <<= 16u; return Kind::Normal;
            case 0x29: r[n] >>= 16u; return Kind::Normal;
            case 0x10: r[n] -= 1u; set_t(ctx, r[n] == 0u); return Kind::Normal;                         // DT
            case 0x11: set_t(ctx, static_cast<std::int32_t>(r[n]) >= 0); return Kind::Normal;          // CMP/PZ
            case 0x15: set_t(ctx, static_cast<std::int32_t>(r[n]) > 0); return Kind::Normal;           // CMP/PL
            case 0x1B: { // TAS.B @Rn
                const std::uint8_t old = rd8(rt, r[n]);
                set_t(ctx, old == 0u);
                wr8(rt, r[n], static_cast<std::uint8_t>(old | 0x80u));
                return Kind::Normal;
            }
            // STS.L / STC.L (pre-decrement stores)
            case 0x02: r[n] -= 4u; wr32(rt, r[n], ctx.mach); return Kind::Normal;
            case 0x12: r[n] -= 4u; wr32(rt, r[n], ctx.macl); return Kind::Normal;
            case 0x22: r[n] -= 4u; wr32(rt, r[n], ctx.pr); return Kind::Normal;
            case 0x52: r[n] -= 4u; wr32(rt, r[n], ctx.fpul); return Kind::Normal;
            case 0x62: r[n] -= 4u; wr32(rt, r[n], ctx.fpscr); return Kind::Normal;
            case 0x03: r[n] -= 4u; wr32(rt, r[n], ctx.sr); return Kind::Normal;
            case 0x13: r[n] -= 4u; wr32(rt, r[n], ctx.gbr); return Kind::Normal;
            case 0x23: r[n] -= 4u; wr32(rt, r[n], ctx.vbr); return Kind::Normal;
            case 0x33: r[n] -= 4u; wr32(rt, r[n], ctx.ssr); return Kind::Normal;
            case 0x43: r[n] -= 4u; wr32(rt, r[n], ctx.spc); return Kind::Normal;
            case 0x32: r[n] -= 4u; wr32(rt, r[n], ctx.sgr); return Kind::Normal;
            case 0xF2: r[n] -= 4u; wr32(rt, r[n], ctx.dbr); return Kind::Normal;
            // LDS.L / LDC.L (post-increment loads; register field is Rm in bits 11-8)
            case 0x06: ctx.mach = rd32(rt, r[n]); r[n] += 4u; return Kind::Normal;
            case 0x16: ctx.macl = rd32(rt, r[n]); r[n] += 4u; return Kind::Normal;
            case 0x26: ctx.pr = rd32(rt, r[n]); r[n] += 4u; return Kind::Normal;
            case 0x56: ctx.fpul = rd32(rt, r[n]); r[n] += 4u; return Kind::Normal;
            case 0x66: { const std::uint32_t v = rd32(rt, r[n]); dc_write_fpscr(ctx, v); r[n] += 4u; return Kind::Normal; }
            case 0x07: { const std::uint32_t v = rd32(rt, r[n]); r[n] += 4u; dc_write_sr(ctx, v); return Kind::Normal; }
            case 0x17: ctx.gbr = rd32(rt, r[n]); r[n] += 4u; return Kind::Normal;
            case 0x27: ctx.vbr = rd32(rt, r[n]); r[n] += 4u; return Kind::Normal;
            case 0x37: ctx.ssr = rd32(rt, r[n]); r[n] += 4u; return Kind::Normal;
            case 0x47: ctx.spc = rd32(rt, r[n]); r[n] += 4u; return Kind::Normal;
            case 0x36: ctx.sgr = rd32(rt, r[n]); r[n] += 4u; return Kind::Normal;
            case 0xF6: ctx.dbr = rd32(rt, r[n]); r[n] += 4u; return Kind::Normal;
            // LDS / LDC (register forms)
            case 0x0A: ctx.mach = r[n]; return Kind::Normal;
            case 0x1A: ctx.macl = r[n]; return Kind::Normal;
            case 0x2A: ctx.pr = r[n]; return Kind::Normal;
            case 0x5A: ctx.fpul = r[n]; return Kind::Normal;
            case 0x6A: dc_write_fpscr(ctx, r[n]); return Kind::Normal;
            case 0x0E: dc_write_sr(ctx, r[n]); return Kind::Normal;
            case 0x1E: ctx.gbr = r[n]; return Kind::Normal;
            case 0x2E: ctx.vbr = r[n]; return Kind::Normal;
            case 0x3E: ctx.ssr = r[n]; return Kind::Normal;
            case 0x4E: ctx.spc = r[n]; return Kind::Normal;
            case 0x3A: ctx.sgr = r[n]; return Kind::Normal;
            case 0xFA: ctx.dbr = r[n]; return Kind::Normal;
            case 0x0B: // JSR @Rn
                if (in_delay_slot) break;
                c.kind = Transfer::Call; c.target = r[n]; c.return_pc = pc + 4u;
                ctx.pr = c.return_pc;
                return Kind::Delayed;
            case 0x2B: // JMP @Rn
                if (in_delay_slot) break;
                c.kind = Transfer::Jump; c.target = r[n];
                return Kind::Delayed;
            default:
                break;
        }
        switch (op & 0xFu) {
            case 0xC: { // SHAD Rm,Rn
                const std::uint32_t amount = r[m], old = r[n];
                if ((amount & 0x80000000u) == 0u) { r[n] = old << (amount & 0x1Fu); }
                else {
                    const std::uint32_t count = ((~amount) & 0x1Fu) + 1u;
                    const std::int32_t s = static_cast<std::int32_t>(old);
                    r[n] = (count >= 32u) ? (s < 0 ? 0xFFFFFFFFu : 0u) : static_cast<std::uint32_t>(s >> count);
                }
                return Kind::Normal;
            }
            case 0xD: { // SHLD Rm,Rn
                const std::uint32_t amount = r[m], old = r[n];
                if ((amount & 0x80000000u) == 0u) { r[n] = old << (amount & 0x1Fu); }
                else {
                    const std::uint32_t count = ((~amount) & 0x1Fu) + 1u;
                    r[n] = (count >= 32u) ? 0u : (old >> count);
                }
                return Kind::Normal;
            }
            case 0xF: { // MAC.W @Rm+,@Rn+
                const std::uint32_t vn = rd16(rt, r[n]); r[n] += 2u;
                const std::uint32_t vm = rd16(rt, r[m]); r[m] += 2u;
                const std::int32_t product = static_cast<std::int32_t>(static_cast<std::int16_t>(vn)) *
                                             static_cast<std::int32_t>(static_cast<std::int16_t>(vm));
                if ((ctx.sr & kSrS) != 0u) {
                    std::int64_t acc = static_cast<std::int64_t>(static_cast<std::int32_t>(ctx.macl)) + product;
                    if (acc > 0x7FFFFFFFll) { acc = 0x7FFFFFFFll; ctx.mach |= 1u; }
                    else if (acc < -0x80000000ll) { acc = -0x80000000ll; ctx.mach |= 1u; }
                    ctx.macl = static_cast<std::uint32_t>(static_cast<std::int32_t>(acc));
                } else {
                    std::uint64_t mac = (static_cast<std::uint64_t>(ctx.mach) << 32u) | ctx.macl;
                    mac += static_cast<std::uint64_t>(static_cast<std::int64_t>(product));
                    ctx.mach = static_cast<std::uint32_t>(mac >> 32u);
                    ctx.macl = static_cast<std::uint32_t>(mac);
                }
                return Kind::Normal;
            }
            default: break;
        }
        if ((op & 0x8Fu) == 0x83u) { r[n] -= 4u; wr32(rt, r[n], ctx.r_bank[(op >> 4u) & 7u]); return Kind::Normal; }          // STC.L Rm_BANK,@-Rn
        if ((op & 0x8Fu) == 0x87u) { ctx.r_bank[(op >> 4u) & 7u] = rd32(rt, r[n]); r[n] += 4u; return Kind::Normal; }          // LDC.L @Rm+,Rn_BANK
        if ((op & 0x8Fu) == 0x8Eu) { ctx.r_bank[(op >> 4u) & 7u] = r[n]; return Kind::Normal; }                                 // LDC Rm,Rn_BANK
        break;

    case 0x5: // MOV.L @(disp,Rm),Rn
        r[n] = rd32(rt, r[m] + (op & 0xFu) * 4u);
        return Kind::Normal;

    case 0x6:
        switch (op & 0xFu) {
            case 0x0: r[n] = sext8(rd8(rt, r[m])); return Kind::Normal;
            case 0x1: r[n] = sext16(rd16(rt, r[m])); return Kind::Normal;
            case 0x2: r[n] = rd32(rt, r[m]); return Kind::Normal;
            case 0x3: r[n] = r[m]; return Kind::Normal;
            case 0x4: { const std::uint32_t v = sext8(rd8(rt, r[m])); if (n != m) r[m] += 1u; r[n] = v; return Kind::Normal; }
            case 0x5: { const std::uint32_t v = sext16(rd16(rt, r[m])); if (n != m) r[m] += 2u; r[n] = v; return Kind::Normal; }
            case 0x6: { const std::uint32_t v = rd32(rt, r[m]); if (n != m) r[m] += 4u; r[n] = v; return Kind::Normal; }
            case 0x7: r[n] = ~r[m]; return Kind::Normal;
            case 0x8: { const std::uint32_t v = r[m]; r[n] = (v & 0xFFFF0000u) | ((v & 0xFFu) << 8u) | ((v & 0xFF00u) >> 8u); return Kind::Normal; }
            case 0x9: { const std::uint32_t v = r[m]; r[n] = (v << 16u) | (v >> 16u); return Kind::Normal; }
            case 0xA: { // NEGC
                const std::uint32_t tmp = 0u - r[m];
                const std::uint32_t result = tmp - (ctx.sr & 1u);
                r[n] = result; set_t(ctx, (0u < tmp) || (tmp < result));
                return Kind::Normal;
            }
            case 0xB: r[n] = 0u - r[m]; return Kind::Normal;
            case 0xC: r[n] = r[m] & 0xFFu; return Kind::Normal;
            case 0xD: r[n] = r[m] & 0xFFFFu; return Kind::Normal;
            case 0xE: r[n] = sext8(r[m]); return Kind::Normal;
            case 0xF: r[n] = sext16(r[m]); return Kind::Normal;
            default: break;
        }
        break;

    case 0x7: // ADD #imm,Rn
        r[n] += sext8(op);
        return Kind::Normal;

    case 0x8:
        switch ((op >> 8u) & 0xFu) {
            case 0x0: wr8(rt, r[m] + (op & 0xFu), r[0]); return Kind::Normal;             // MOV.B R0,@(disp,Rn)
            case 0x1: wr16(rt, r[m] + (op & 0xFu) * 2u, r[0]); return Kind::Normal;       // MOV.W R0,@(disp,Rn)
            case 0x4: r[0] = sext8(rd8(rt, r[m] + (op & 0xFu))); return Kind::Normal;      // MOV.B @(disp,Rm),R0
            case 0x5: r[0] = sext16(rd16(rt, r[m] + (op & 0xFu) * 2u)); return Kind::Normal; // MOV.W @(disp,Rm),R0
            case 0x8: set_t(ctx, r[0] == sext8(op)); return Kind::Normal;                 // CMP/EQ #imm,R0
            case 0x9: case 0xB: // BT / BF
                if (in_delay_slot) break;
                c.kind = Transfer::CondJump;
                c.target = pc + 4u + sext8(op) * 2u;
                c.taken = (((op >> 8u) & 0xFu) == 0x9u) ? get_t(ctx) : !get_t(ctx);
                return Kind::CondBranch;
            case 0xD: case 0xF: // BT/S / BF/S: T is sampled before the delay slot
                if (in_delay_slot) break;
                c.kind = Transfer::CondJump;
                c.target = pc + 4u + sext8(op) * 2u;
                c.taken = (((op >> 8u) & 0xFu) == 0xDu) ? get_t(ctx) : !get_t(ctx);
                return Kind::Delayed;
            default: break;
        }
        break;

    case 0x9: { // MOV.W @(disp,PC),Rn
        if (in_delay_slot) ++st.stats.pc_relative_in_delay_slot;
        r[n] = sext16(rd16(rt, pc + 4u + (op & 0xFFu) * 2u));
        return Kind::Normal;
    }

    case 0xA: // BRA
        if (in_delay_slot) break;
        c.kind = Transfer::Jump;
        c.target = pc + 4u + sext12(op) * 2u;
        return Kind::Delayed;

    case 0xB: // BSR
        if (in_delay_slot) break;
        c.kind = Transfer::Call;
        c.target = pc + 4u + sext12(op) * 2u;
        c.return_pc = pc + 4u;
        ctx.pr = c.return_pc;
        return Kind::Delayed;

    case 0xC: {
        const std::uint32_t imm = op & 0xFFu;
        switch ((op >> 8u) & 0xFu) {
            case 0x0: wr8(rt, ctx.gbr + imm, r[0]); return Kind::Normal;             // MOV.B R0,@(disp,GBR)
            case 0x1: wr16(rt, ctx.gbr + imm * 2u, r[0]); return Kind::Normal;       // MOV.W R0,@(disp,GBR)
            case 0x2: wr32(rt, ctx.gbr + imm * 4u, r[0]); return Kind::Normal;       // MOV.L R0,@(disp,GBR)
            case 0x3: // TRAPA #imm
                if (in_delay_slot) break;
                c.trap_imm = imm;
                return Kind::Trap;
            case 0x4: r[0] = sext8(rd8(rt, ctx.gbr + imm)); return Kind::Normal;      // MOV.B @(disp,GBR),R0
            case 0x5: r[0] = sext16(rd16(rt, ctx.gbr + imm * 2u)); return Kind::Normal; // MOV.W @(disp,GBR),R0
            case 0x6: r[0] = rd32(rt, ctx.gbr + imm * 4u); return Kind::Normal;      // MOV.L @(disp,GBR),R0
            case 0x7: // MOVA @(disp,PC),R0
                if (in_delay_slot) ++st.stats.pc_relative_in_delay_slot;
                r[0] = ((pc + 4u) & ~3u) + imm * 4u;
                return Kind::Normal;
            case 0x8: set_t(ctx, (r[0] & imm) == 0u); return Kind::Normal;           // TST #imm,R0
            case 0x9: r[0] &= imm; return Kind::Normal;
            case 0xA: r[0] ^= imm; return Kind::Normal;
            case 0xB: r[0] |= imm; return Kind::Normal;
            case 0xC: set_t(ctx, (rd8(rt, ctx.gbr + r[0]) & imm) == 0u); return Kind::Normal; // TST.B #imm,@(R0,GBR)
            case 0xD: { const std::uint32_t a = ctx.gbr + r[0]; wr8(rt, a, rd8(rt, a) & imm); return Kind::Normal; } // AND.B
            case 0xE: { const std::uint32_t a = ctx.gbr + r[0]; wr8(rt, a, rd8(rt, a) ^ imm); return Kind::Normal; } // XOR.B
            case 0xF: { const std::uint32_t a = ctx.gbr + r[0]; wr8(rt, a, rd8(rt, a) | imm); return Kind::Normal; } // OR.B
            default: break;
        }
        break;
    }

    case 0xD: { // MOV.L @(disp,PC),Rn
        if (in_delay_slot) ++st.stats.pc_relative_in_delay_slot;
        r[n] = rd32(rt, ((pc + 4u) & ~3u) + (op & 0xFFu) * 4u);
        return Kind::Normal;
    }

    case 0xE: // MOV #imm,Rn
        r[n] = sext8(op);
        return Kind::Normal;

    case 0xF:
        return exec_fpu(ctx, rt, st, pc, op);

    default:
        break;
    }

    if (in_delay_slot) illegal(pc, op, "illegal slot instruction");
    illegal(pc, op, "unsupported or illegal SH-4 instruction");
}

bool target_is_compiled(InterpState& st, DCRuntime& rt, std::uint32_t target) {
    auto& memo = st.target_memo[(target >> 1u) & (kTargetMemoSize - 1u)];
    if (memo.address == target) return memo.registered;
    const std::uint32_t canonical = dc_canonical_code_target(rt, target);
    const auto it = rt.targets.find(canonical);
    memo.address = target;
    memo.registered = it != rt.targets.end() && it->second != nullptr &&
                      it->second != &dc_sh4_interp_function;
    return memo.registered;
}

void load_seed_file(InterpState& st) {
    if (st.seeds_loaded) return;
    st.seeds_loaded = true;
    if (st.seeds_path.empty()) return;
    std::ifstream in(st.seeds_path);
    if (!in) return;
    std::string line;
    while (std::getline(in, line)) {
        // Both active seeds ("0x...") and reported runtime-code lines
        // ("# runtime-code 0x...") are de-duplicated across runs.
        const auto pos = line.find("0x");
        if (pos == std::string::npos) continue;
        if (line.find('#') != std::string::npos && line.find("runtime-code") == std::string::npos &&
            line.find('#') < pos)
            continue;
        try {
            st.known_entries.insert(static_cast<std::uint32_t>(std::stoul(line.substr(pos), nullptr, 16)));
        } catch (...) {
        }
    }
}

void append_seed_line(InterpState& st, const std::string& line, bool header_needed) {
    if (st.seeds_path.empty() || st.seeds_write_failed) return;
    std::ofstream out(st.seeds_path, std::ios::app);
    if (!out) {
        st.seeds_write_failed = true;
        std::cerr << "[DCR SH4-INTERP] WARNING: cannot write seed file " << st.seeds_path << "\n";
        return;
    }
    if (header_needed) {
        out << "# DreamcastRecompiled - SH-4 entrypoints learned by the interpreter fallback.\n"
            << "# Active lines (0x...) match the boot image and are passed to the recompiler\n"
            << "# as --seed-file on the next SH-4 preparation. '# runtime-code' lines only\n"
            << "# exist in memory at runtime (overlays/generated code) and stay interpreted.\n";
    }
    out << line << "\n";
}

// Only capture addresses that are reachable via the interpreter. Never treat
// odd/unmapped targets as seeds. Limit disk usage and reject partial writes.
void capture_dynamic_snapshot(InterpState& st, DCRuntime& rt, std::uint32_t entry) {
    if (!st.dynamic_capture || st.dynamic_dir.empty() || st.dynamic_snapshots >= kMaxDynamicSnapshotsPerRun)
        return;
    bool ok=false;
    const auto offset=main_ram_offset(entry,ok);
    if (!ok || (entry & 1u) || st.dynamic_variants[entry]>=kMaxDynamicVariantsPerEntry)
        return;
    const std::uint32_t base=entry & ~0xFFFu;
    bool base_ok=false;
    const auto start=main_ram_offset(base,base_ok);
    if (!base_ok || start+kDynamicSnapshotBytes>DCRuntime::kMainRamSize) return;
    const auto* code=rt.main_ram.data()+start;
    const auto hash=fnv64(code,kDynamicSnapshotBytes);
    std::ostringstream name;
    name << "snapshot_" << std::uppercase << std::hex << std::setfill('0')
         << std::setw(8) << entry << "_" << std::setw(8) << base
         << "_" << std::setw(16) << hash << ".bin";
    try {
        namespace fs=std::filesystem;
        const fs::path folder(st.dynamic_dir);
        std::error_code ec;
        fs::create_directories(folder,ec);
        if (ec) return;
        const auto path=folder/name.str();
        if (fs::exists(path)) return;
        const auto temp=fs::path(path.string()+".tmp");
        {
            std::ofstream out(temp,std::ios::binary|std::ios::trunc);
            if (!out || !out.write(reinterpret_cast<const char*>(code),kDynamicSnapshotBytes)) return;
        }
        fs::rename(temp,path,ec);
        if (ec) { fs::remove(temp); return; }
        ++st.dynamic_snapshots;
        ++st.dynamic_variants[entry];
        std::cerr << "[DCR SH4-DYNAMIC] captured entry=" << hex8(entry)
                  << " base=" << hex8(base) << " bytes=" << kDynamicSnapshotBytes
                  << " file=" << path.filename().string() << "\n";
    } catch (const std::exception& e) {
        if (st.trace) std::cerr << "[DCR SH4-DYNAMIC] capture failed: " << e.what() << "\n";
    }
}

bool reference_matches(const InterpState& st, DCRuntime& rt, std::uint32_t target) {
    if (!st.has_reference) return false;
    bool ok = false;
    const std::uint32_t target_off = main_ram_offset(target, ok);
    if (!ok) return false;
    bool base_ok = false;
    const std::uint32_t base_off = main_ram_offset(st.reference_base, base_ok);
    if (!base_ok || target_off < base_off) return false;
    const std::size_t image_off = target_off - base_off;
    if (image_off >= st.reference_image.size()) return false;
    const std::size_t len = std::min<std::size_t>(kSeedVerifyBytes, st.reference_image.size() - image_off);
    if (len < 2u || target_off + len > DCRuntime::kMainRamSize) return false;
    return std::memcmp(rt.main_ram.data() + target_off, st.reference_image.data() + image_off, len) == 0;
}

} // namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool dc_sh4_interp_enabled(DCRuntime& runtime) {
    return state_for(runtime).enabled;
}

bool dc_sh4_interp_accepts(DCRuntime& runtime, std::uint32_t target) {
    // Only guest main RAM holds game code. A jump into the BIOS/flash/MMIO
    // apertures or an unmapped address is almost certainly a corrupted
    // pointer; keep the historical, more informative missing-target error.
    if (!state_for(runtime).enabled || (target & 1u) != 0u) return false;
    bool ok = false;
    (void)main_ram_offset(target, ok);
    return ok;
}

void dc_sh4_interp_configure(DCRuntime& runtime, bool enabled, const std::string& seeds_path, bool trace) {
    auto& st = state_for(runtime);
    st.configured = true;
    st.enabled = enabled;
    st.seeds_path = seeds_path;
    st.trace = trace;
    st.seeds_loaded = false;
    st.seeds_write_failed = false;
}

void dc_sh4_interp_configure_dynamic(DCRuntime& runtime,bool enabled,const std::string& directory) {
    auto& st=state_for(runtime);
    st.dynamic_capture=enabled;
    st.dynamic_dir=directory;
}

void dc_sh4_interp_reset(DCRuntime& runtime) {
    auto& states = registry().states;
    states.erase(&runtime);
    if (g_last_owner == &runtime) {
        g_last_owner = nullptr;
        g_last_state = nullptr;
    }
}

DCSh4InterpStats dc_sh4_interp_stats(DCRuntime& runtime) {
    return state_for(runtime).stats;
}

void dc_sh4_interp_print_stats(DCRuntime& runtime) {
    const auto& s = state_for(runtime).stats;
    std::cerr << "[DCR SH4-INTERP] entries=" << s.unique_entries << " seeds-written=" << s.seeds_written
              << " runtime-code=" << s.runtime_code_entries << " calls=" << s.function_entries
              << " instructions=" << s.instructions << " aot-calls=" << s.aot_calls
              << " aot-tail=" << s.aot_tail_branches << " irq=" << s.interrupts
              << " sleep=" << s.sleeps << " trapa=" << s.traps << "\n";
}

void dc_sh4_interp_set_reference_image(DCRuntime& runtime, std::uint32_t base,
                                       const std::uint8_t* data, std::size_t size) {
    auto& st = state_for(runtime);
    st.reference_base = base;
    st.reference_image.assign(data, data + size);
    st.has_reference = size != 0u;
}

void dc_sh4_interp_note_entry(DCRuntime& runtime, std::uint32_t target, std::uint32_t caller_pc) {
    auto& st = state_for(runtime);
    if (st.continuations.contains(target)) return;
    load_seed_file(st);
    // Dynamic variants can change while the address remains constant. Do not
    // short-circuit snapshot capture just because the entry was seen earlier.
    if (!reference_matches(st,runtime,target)) capture_dynamic_snapshot(st,runtime,target);
    ++st.stats.unique_entries;
    if (!st.known_entries.insert(target).second) return;

    bool ram_ok = false;
    const std::uint32_t off = main_ram_offset(target, ram_ok);
    // The recompiler registers commercial code through P1 aliases.
    const std::uint32_t seed = ram_ok ? (0x80000000u | (DCRuntime::kMainRamPhysicalBase + off)) : target;
    const bool verified = ram_ok && reference_matches(st, runtime, target);
    const bool header_needed = !st.seeds_path.empty() && !std::ifstream(st.seeds_path).good();
    std::ostringstream line;
    if (verified) {
        ++st.stats.seeds_written;
        line << hex8(seed) << "  # caller=" << hex8(caller_pc);
    } else {
        ++st.stats.runtime_code_entries;
        line << "# runtime-code " << hex8(target) << " caller=" << hex8(caller_pc)
             << (st.has_reference ? " (bytes differ from boot image)" : " (no boot image reference)");
    }
    append_seed_line(st, line.str(), header_needed);

    if (st.trace || st.verbose_entries < kVerboseEntryLimit) {
        ++st.verbose_entries;
        std::cerr << "[DCR SH4-INTERP] " << (verified ? "new entry " : "runtime code ") << hex8(target)
                  << " from " << hex8(caller_pc)
                  << (verified ? " -> interpreted now, AOT seed saved" : " -> interpreted (not in boot image)") << "\n";
        if (!st.trace && st.verbose_entries == kVerboseEntryLimit)
            std::cerr << "[DCR SH4-INTERP] further new entries are logged only to the seed file"
                      << (st.seeds_path.empty() ? "" : (" " + st.seeds_path)) << "\n";
    }
}

namespace {
void interpret(SH4Context& ctx, DCRuntime& runtime, InterpState& st);
} // namespace

void dc_sh4_interp_function(SH4Context& ctx, DCRuntime& runtime) {
    InterpState& st = state_for(runtime);
    const std::uint32_t entry = ctx.pc;
    const std::uint32_t caller = runtime.current_pc;
    try {
        interpret(ctx, runtime, st);
    } catch (const IllegalInstruction& e) {
        // Only the innermost interpreter frame adds context.
        if (std::string(e.what()).find(" | entry ") != std::string::npos) throw;
        std::ostringstream out;
        out << e.what() << " | entry " << hex8(entry) << " reached from " << hex8(caller)
            << " (interpreted; the guest may have jumped through a corrupted pointer)";
        throw IllegalInstruction(out.str());
    }
}

namespace {
void interpret(SH4Context& ctx, DCRuntime& runtime, InterpState& st) {
    ++st.stats.function_entries;

    std::uint32_t pc = ctx.pc;
    std::uint64_t cycles = 0u;
    unsigned straight = 0u;

    const auto flush_cycles = [&]() {
        runtime.sh4_tick_pending_cycles += cycles;
        cycles = 0u;
    };
    // Mirrors a generated basic-block header: publish PC, account time, and
    // return immediately if the scheduler redirected execution (IRQ).
    const auto block_tick = [&](std::uint32_t at) -> bool {
        ctx.pc = at;
        runtime.current_pc = at;
        const std::uint64_t spent = cycles != 0u ? cycles : 1u;
        cycles = 0u;
        straight = 0u;
        if (dc_runtime_tick_fast(ctx, runtime, spent)) {
            st.continuations.insert(at);
            ++st.stats.interrupts;
            return true;
        }
        return false;
    };
    // Continue interpreting at `target`, or hand the rest of the path to AOT.
    // Returns true when this interpreter invocation is finished.
    const auto jump_to = [&](std::uint32_t from, std::uint32_t target) -> bool {
        if (target_is_compiled(st, runtime, target)) {
            ++st.stats.aot_tail_branches;
            flush_cycles();
            ctx.pc = from;
            call_recompiled(ctx, runtime, target);
            return true;
        }
        pc = target;
        return block_tick(pc);
    };

    if (block_tick(pc)) return;
    for (;;) {
        if (straight >= kMaxStraightLine && block_tick(pc)) return;
        const std::uint16_t op = fetch16(runtime, pc);
#if defined(DCR_SH4_INTERP_DEBUG_TRACE)
        std::fprintf(stderr, "IT %08X", pc);
        for (unsigned ri = 0; ri < 16u; ++ri) std::fprintf(stderr, " %08X", ctx.r[ri]);
        std::fprintf(stderr, " T%u\n", ctx.sr & 1u);
#endif
        ++straight;
        ++cycles;
        ++st.stats.instructions;

        Control c{};
        const Kind kind = exec_one(ctx, runtime, st, pc, op, false, c);
        if (kind == Kind::Normal) {
            pc += 2u;
            continue;
        }

        if (kind == Kind::CondBranch) {
            if (c.taken) {
                if (jump_to(pc, c.target)) return;
            } else {
                pc += 2u;
                if (block_tick(pc)) return;
            }
            continue;
        }

        if (kind == Kind::Sleep) {
            const std::uint32_t resume = pc + 2u;
            ++st.stats.sleeps;
            flush_cycles();
            st.continuations.insert(resume);
            for (unsigned slice = 0u; slice < kSleepMaxSlices; ++slice) {
                ctx.pc = resume;
                runtime.current_pc = resume;
                if (dc_runtime_tick_fast(ctx, runtime, kSleepSliceCycles)) {
                    ++st.stats.interrupts;
                    return;
                }
            }
            pc = resume;
            if (block_tick(pc)) return;
            continue;
        }

        if (kind == Kind::Trap) {
            // TRAPA: architectural exception entry, unwound like an interrupt.
            const std::uint32_t resume = pc + 2u;
            ++st.stats.traps;
            flush_cycles();
            ctx.ssr = ctx.sr;
            ctx.spc = resume;
            ctx.sgr = ctx.r[15];
            dc_write32(runtime, 0xFF000020u, c.trap_imm << 2u); // TRA
            dc_write32(runtime, 0xFF000024u, 0x160u);           // EXPEVT
            dc_write_sr(ctx, ctx.sr | kSrMd | kSrRb | kSrBl);
            ctx.pc = ctx.vbr + 0x100u;
            runtime.current_pc = ctx.pc;
            runtime.sh4_async_redirect = true;
            st.continuations.insert(resume);
            return;
        }

        // Delayed transfer: execute the slot, then transfer.
        const std::uint32_t slot_pc = pc + 2u;
        const std::uint16_t slot_op = fetch16(runtime, slot_pc);
#if defined(DCR_SH4_INTERP_DEBUG_TRACE)
        std::fprintf(stderr, "IT %08X", slot_pc);
        for (unsigned ri = 0; ri < 16u; ++ri) std::fprintf(stderr, " %08X", ctx.r[ri]);
        std::fprintf(stderr, " T%u\n", ctx.sr & 1u);
#endif
        ++cycles;
        ++st.stats.instructions;
        Control unused{};
        if (exec_one(ctx, runtime, st, slot_pc, slot_op, true, unused) != Kind::Normal)
            illegal(slot_pc, slot_op, "illegal slot instruction");

        switch (c.kind) {
            case Transfer::Return:
            case Transfer::Rte:
                flush_cycles();
                ctx.pc = c.target;
                runtime.current_pc = c.target;
                return;
            case Transfer::Jump:
                if (jump_to(pc, c.target)) return;
                continue;
            case Transfer::CondJump:
                if (c.taken) {
                    if (jump_to(pc, c.target)) return;
                } else {
                    pc += 4u;
                    if (block_tick(pc)) return;
                }
                continue;
            case Transfer::Call: {
                const std::uint32_t ret = c.return_pc;
                st.continuations.insert(ret);
                flush_cycles();
                if (target_is_compiled(st, runtime, c.target)) ++st.stats.aot_calls;
                ctx.pc = pc;
                ctx.pr = ret;
                call_recompiled(ctx, runtime, c.target);
                if (runtime.sh4_async_redirect || ctx.pc != ret) return;
                pc = ret;
                if (block_tick(pc)) return;
                continue;
            }
        }
    }
}
} // namespace

} // namespace dcrecomp_generated
