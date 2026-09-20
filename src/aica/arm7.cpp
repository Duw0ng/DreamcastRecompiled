#include "dcrecomp/arm7.hpp"

#include <bit>
#include <limits>
#include <sstream>
#include <utility>

namespace dcrecomp {
namespace {

constexpr std::uint32_t N = 1u << 31;
constexpr std::uint32_t Z = 1u << 30;
constexpr std::uint32_t C = 1u << 29;
constexpr std::uint32_t V = 1u << 28;
constexpr std::uint32_t I = 1u << 7;
constexpr std::uint32_t F = 1u << 6;
constexpr std::uint32_t T = 1u << 5;
constexpr std::uint32_t MODE_MASK = 0x1Fu;
constexpr std::uint32_t MODE_USR = 0x10u;
constexpr std::uint32_t MODE_FIQ = 0x11u;
constexpr std::uint32_t MODE_IRQ = 0x12u;
constexpr std::uint32_t MODE_SVC = 0x13u;
constexpr std::uint32_t MODE_ABT = 0x17u;
constexpr std::uint32_t MODE_UND = 0x1Bu;
constexpr std::uint32_t MODE_SYS = 0x1Fu;

bool valid_mode(std::uint32_t mode) {
    return mode == MODE_USR || mode == MODE_FIQ || mode == MODE_IRQ || mode == MODE_SVC ||
           mode == MODE_ABT || mode == MODE_UND || mode == MODE_SYS;
}

std::array<std::uint32_t, 2>* sp_lr_bank(ARM7Context& ctx, std::uint32_t mode) {
    switch (mode) {
        case MODE_USR:
        case MODE_SYS: return &ctx.usr_r13_r14;
        case MODE_IRQ: return &ctx.irq_r13_r14;
        case MODE_SVC: return &ctx.svc_r13_r14;
        case MODE_ABT: return &ctx.abt_r13_r14;
        case MODE_UND: return &ctx.und_r13_r14;
        default: return nullptr;
    }
}

void save_bank(ARM7Context& ctx, std::uint32_t mode) {
    if (mode == MODE_FIQ) {
        for (unsigned i = 0; i < 7; ++i) ctx.fiq_r8_r14[i] = ctx.r[8 + i];
        return;
    }
    for (unsigned i = 0; i < 5; ++i) ctx.common_r8_r12[i] = ctx.r[8 + i];
    if (auto* bank = sp_lr_bank(ctx, mode)) {
        (*bank)[0] = ctx.r[13];
        (*bank)[1] = ctx.r[14];
    }
}

void load_bank(ARM7Context& ctx, std::uint32_t mode) {
    if (mode == MODE_FIQ) {
        for (unsigned i = 0; i < 7; ++i) ctx.r[8 + i] = ctx.fiq_r8_r14[i];
        return;
    }
    for (unsigned i = 0; i < 5; ++i) ctx.r[8 + i] = ctx.common_r8_r12[i];
    if (auto* bank = sp_lr_bank(ctx, mode)) {
        ctx.r[13] = (*bank)[0];
        ctx.r[14] = (*bank)[1];
    }
}

std::uint32_t apply_psr_fields(std::uint32_t old_value, std::uint32_t value, std::uint32_t fields) {
    std::uint32_t mask = 0u;
    if (fields & 1u) mask |= 0x000000FFu; // control
    if (fields & 2u) mask |= 0x0000FF00u; // extension
    if (fields & 4u) mask |= 0x00FF0000u; // status
    if (fields & 8u) mask |= 0xFF000000u; // flags
    return (old_value & ~mask) | (value & mask);
}

bool condition_passed(std::uint32_t cpsr, unsigned cond) {
    const bool n = (cpsr & N) != 0u;
    const bool z = (cpsr & Z) != 0u;
    const bool c = (cpsr & C) != 0u;
    const bool v = (cpsr & V) != 0u;
    switch (cond) {
        case 0x0: return z;
        case 0x1: return !z;
        case 0x2: return c;
        case 0x3: return !c;
        case 0x4: return n;
        case 0x5: return !n;
        case 0x6: return v;
        case 0x7: return !v;
        case 0x8: return c && !z;
        case 0x9: return !c || z;
        case 0xA: return n == v;
        case 0xB: return n != v;
        case 0xC: return !z && (n == v);
        case 0xD: return z || (n != v);
        case 0xE: return true;
        default: return false; // NV is reserved in ARMv4T.
    }
}

std::uint32_t read_reg(const ARM7Context& ctx, unsigned reg, std::uint32_t pc) {
    return reg == 15u ? pc + 8u : ctx.r[reg];
}

std::uint32_t read_user_reg(const ARM7Context& ctx, unsigned reg, std::uint32_t pc) {
    if (reg == 15u) return pc + 8u;
    if (reg < 8u) return ctx.r[reg];
    const std::uint32_t mode = ctx.cpsr & MODE_MASK;
    if (reg < 13u) {
        // R8-R12 are banked only in FIQ mode. Outside FIQ, the live registers
        // already are the User/System bank.
        return mode == MODE_FIQ ? ctx.common_r8_r12[reg - 8u] : ctx.r[reg];
    }
    if (reg < 15u) {
        // R13/R14 have a dedicated User/System bank in every exception mode.
        return (mode == MODE_USR || mode == MODE_SYS) ? ctx.r[reg]
                                                       : ctx.usr_r13_r14[reg - 13u];
    }
    return ctx.r[reg];
}

void write_user_reg(ARM7Context& ctx, unsigned reg, std::uint32_t value) {
    if (reg < 8u) { ctx.r[reg] = value; return; }
    const std::uint32_t mode = ctx.cpsr & MODE_MASK;
    if (reg < 13u) {
        if (mode == MODE_FIQ) ctx.common_r8_r12[reg - 8u] = value;
        else ctx.r[reg] = value;
        return;
    }
    if (reg < 15u) {
        if (mode == MODE_USR || mode == MODE_SYS) ctx.r[reg] = value;
        else ctx.usr_r13_r14[reg - 13u] = value;
    }
}

bool mode_has_spsr(std::uint32_t mode) {
    return mode == MODE_FIQ || mode == MODE_IRQ || mode == MODE_SVC ||
           mode == MODE_ABT || mode == MODE_UND;
}

void set_nz(ARM7Context& ctx, std::uint32_t result) {
    ctx.cpsr = (ctx.cpsr & ~(N | Z)) | (result & N) | (result == 0u ? Z : 0u);
}

void set_logic_flags(ARM7Context& ctx, std::uint32_t result, bool carry_valid, bool carry) {
    set_nz(ctx, result);
    if (carry_valid) ctx.cpsr = (ctx.cpsr & ~C) | (carry ? C : 0u);
}

std::uint32_t add_with_carry(ARM7Context& ctx, std::uint32_t a, std::uint32_t b,
                             std::uint32_t carry_in, bool set_flags) {
    const std::uint64_t wide = static_cast<std::uint64_t>(a) + b + carry_in;
    const std::uint32_t result = static_cast<std::uint32_t>(wide);
    if (set_flags) {
        set_nz(ctx, result);
        ctx.cpsr = (ctx.cpsr & ~C) | ((wide >> 32) ? C : 0u);
        const bool overflow = ((~(a ^ b) & (a ^ result)) & 0x80000000u) != 0u;
        ctx.cpsr = (ctx.cpsr & ~V) | (overflow ? V : 0u);
    }
    return result;
}

std::uint32_t sub_with_flags(ARM7Context& ctx, std::uint32_t a, std::uint32_t b,
                             std::uint32_t borrow, bool set_flags) {
    const std::uint64_t rhs = static_cast<std::uint64_t>(b) + borrow;
    const std::uint32_t result = static_cast<std::uint32_t>(static_cast<std::uint64_t>(a) - rhs);
    if (set_flags) {
        set_nz(ctx, result);
        const bool no_borrow = static_cast<std::uint64_t>(a) >= rhs;
        ctx.cpsr = (ctx.cpsr & ~C) | (no_borrow ? C : 0u);
        const std::uint32_t effective_b = b + borrow;
        const bool overflow = (((a ^ effective_b) & (a ^ result)) & 0x80000000u) != 0u;
        ctx.cpsr = (ctx.cpsr & ~V) | (overflow ? V : 0u);
    }
    return result;
}

struct ShifterResult {
    std::uint32_t value{};
    bool carry{};
    bool carry_valid{};
};

ShifterResult shift_value(std::uint32_t value, unsigned type, unsigned amount,
                          bool old_carry, bool immediate_form) {
    ShifterResult r{value, old_carry, false};
    switch (type) {
        case 0: // LSL
            if (amount == 0) return r;
            if (amount < 32) { r.carry = ((value >> (32 - amount)) & 1u) != 0u; r.value = value << amount; }
            else if (amount == 32) { r.carry = (value & 1u) != 0u; r.value = 0u; }
            else { r.carry = false; r.value = 0u; }
            r.carry_valid = true;
            return r;
        case 1: // LSR
            if (amount == 0 && immediate_form) amount = 32;
            if (amount == 0) return r;
            if (amount < 32) { r.carry = ((value >> (amount - 1)) & 1u) != 0u; r.value = value >> amount; }
            else if (amount == 32) { r.carry = (value >> 31) != 0u; r.value = 0u; }
            else { r.carry = false; r.value = 0u; }
            r.carry_valid = true;
            return r;
        case 2: // ASR
            if (amount == 0 && immediate_form) amount = 32;
            if (amount == 0) return r;
            if (amount >= 32) { r.carry = (value >> 31) != 0u; r.value = r.carry ? 0xFFFFFFFFu : 0u; }
            else { r.carry = ((value >> (amount - 1)) & 1u) != 0u; r.value = static_cast<std::uint32_t>(static_cast<std::int32_t>(value) >> amount); }
            r.carry_valid = true;
            return r;
        default: // ROR / RRX
            if (amount == 0 && immediate_form) {
                r.carry = (value & 1u) != 0u;
                r.value = (old_carry ? 0x80000000u : 0u) | (value >> 1);
                r.carry_valid = true;
                return r;
            }
            if (amount == 0) return r;
            amount &= 31u;
            if (amount == 0) { r.carry = (value >> 31) != 0u; r.value = value; }
            else { r.value = std::rotr(value, static_cast<int>(amount)); r.carry = (r.value >> 31) != 0u; }
            r.carry_valid = true;
            return r;
    }
}

ShifterResult decode_operand2(const ARM7Context& ctx, std::uint32_t op, std::uint32_t pc) {
    const bool old_carry = (ctx.cpsr & C) != 0u;
    if (op & (1u << 25)) {
        const std::uint32_t imm = op & 0xFFu;
        const unsigned rot = ((op >> 8) & 0xFu) * 2u;
        if (rot == 0) return {imm, old_carry, false};
        const std::uint32_t value = std::rotr(imm, static_cast<int>(rot));
        return {value, (value >> 31) != 0u, true};
    }

    const unsigned rm = op & 0xFu;
    const std::uint32_t value = read_reg(ctx, rm, pc);
    const unsigned type = (op >> 5) & 3u;
    if ((op & (1u << 4)) == 0u) {
        const unsigned amount = (op >> 7) & 0x1Fu;
        return shift_value(value, type, amount, old_carry, true);
    }
    const unsigned rs = (op >> 8) & 0xFu;
    const unsigned amount = read_reg(ctx, rs, pc) & 0xFFu;
    return shift_value(value, type, amount, old_carry, false);
}

ARM7StepResult unsupported(ARM7Context& ctx, std::uint32_t pc, std::uint32_t op, std::string detail) {
    ctx.halted = true;
    return {ARM7StepStatus::Unsupported, pc, op, std::move(detail)};
}

ARM7StepResult fault(ARM7Context& ctx, std::uint32_t pc, std::uint32_t op, const std::string& detail) {
    ctx.halted = true;
    return {ARM7StepStatus::Fault, pc, op, detail};
}

void enter_exception(ARM7Context& ctx, std::uint32_t mode, std::uint32_t vector, bool mask_fiq) {
    const std::uint32_t old_cpsr = ctx.cpsr;
    const std::uint32_t old_mode = old_cpsr & MODE_MASK;
    const std::uint32_t resume_pc = ctx.r[15] & ((old_cpsr & T) ? ~1u : ~3u);

    if (old_mode != mode) {
        save_bank(ctx, old_mode);
        load_bank(ctx, mode);
    }

    if (mode == MODE_FIQ) ctx.spsr_fiq = old_cpsr;
    else if (mode == MODE_IRQ) ctx.spsr_irq = old_cpsr;

    ctx.cpsr = (old_cpsr & ~(MODE_MASK | T)) | mode | I | (mask_fiq ? F : 0u);
    ctx.r[14] = resume_pc + 4u;
    ctx.r[15] = vector;
}

} // namespace

void arm7_reset(ARM7Context& ctx, std::uint32_t pc) {
    ctx = ARM7Context{};
    ctx.r[15] = pc & ~3u;
    ctx.cpsr = 0xD3u;
}

void arm7_set_cpsr(ARM7Context& ctx, std::uint32_t value, std::uint32_t field_mask) {
    const std::uint32_t old_mode = ctx.cpsr & MODE_MASK;
    std::uint32_t next = apply_psr_fields(ctx.cpsr, value, field_mask);
    std::uint32_t new_mode = next & MODE_MASK;
    if (!valid_mode(new_mode)) {
        next = (next & ~MODE_MASK) | old_mode;
        new_mode = old_mode;
    }
    if (new_mode != old_mode) {
        save_bank(ctx, old_mode);
        load_bank(ctx, new_mode);
    }
    ctx.cpsr = next;
}

std::uint32_t arm7_get_spsr(const ARM7Context& ctx) {
    switch (ctx.cpsr & MODE_MASK) {
        case MODE_FIQ: return ctx.spsr_fiq;
        case MODE_IRQ: return ctx.spsr_irq;
        case MODE_SVC: return ctx.spsr_svc;
        case MODE_ABT: return ctx.spsr_abt;
        case MODE_UND: return ctx.spsr_und;
        default: return ctx.cpsr;
    }
}

void arm7_set_spsr(ARM7Context& ctx, std::uint32_t value, std::uint32_t field_mask) {
    std::uint32_t* spsr = nullptr;
    switch (ctx.cpsr & MODE_MASK) {
        case MODE_FIQ: spsr = &ctx.spsr_fiq; break;
        case MODE_IRQ: spsr = &ctx.spsr_irq; break;
        case MODE_SVC: spsr = &ctx.spsr_svc; break;
        case MODE_ABT: spsr = &ctx.spsr_abt; break;
        case MODE_UND: spsr = &ctx.spsr_und; break;
        default: return;
    }
    *spsr = apply_psr_fields(*spsr, value, field_mask);
}

void arm7_set_irq_line(ARM7Context& ctx, bool asserted) {
    ctx.irq_line = asserted;
}

void arm7_set_fiq_line(ARM7Context& ctx, bool asserted) {
    ctx.fiq_line = asserted;
}

ARM7StepResult arm7_step(ARM7Context& ctx, ARM7Bus& bus) {
    if (ctx.halted) return {ARM7StepStatus::Halted, ctx.r[15], 0u, "ARM7 halted"};

    // ARM7TDMI samples FIQ before IRQ at instruction boundaries. Exception
    // entry itself is represented as an executed scheduler step but does not
    // increment the fetched-instruction counter.
    if (ctx.fiq_line && (ctx.cpsr & F) == 0u) {
        const std::uint32_t pc = ctx.r[15];
        enter_exception(ctx, MODE_FIQ, 0x1Cu, true);
        ++ctx.fiq_exceptions;
        return {ARM7StepStatus::Executed, pc, 0u, "FIQ"};
    }
    if (ctx.irq_line && (ctx.cpsr & I) == 0u) {
        const std::uint32_t pc = ctx.r[15];
        enter_exception(ctx, MODE_IRQ, 0x18u, false);
        ++ctx.irq_exceptions;
        return {ARM7StepStatus::Executed, pc, 0u, "IRQ"};
    }

    if (ctx.cpsr & T) return unsupported(ctx, ctx.r[15], 0u, "Thumb state is not implemented in the bootstrap ARM7 core");

    const std::uint32_t pc = ctx.r[15] & ~3u;
    std::uint32_t op = 0u;
    try {
        op = bus.read32(pc);
    } catch (const std::exception& e) {
        return fault(ctx, pc, 0u, e.what());
    }
    ++ctx.instructions;

    const unsigned cond = op >> 28;
    if (!condition_passed(ctx.cpsr, cond)) {
        ctx.r[15] = pc + 4u;
        return {ARM7StepStatus::Executed, pc, op, {}};
    }

    // BX Rm (ARMv4T)
    if ((op & 0x0FFFFFF0u) == 0x012FFF10u) {
        const std::uint32_t target = read_reg(ctx, op & 0xFu, pc);
        ctx.cpsr = (ctx.cpsr & ~T) | ((target & 1u) ? T : 0u);
        ctx.r[15] = target & ((target & 1u) ? ~1u : ~3u);
        return {ARM7StepStatus::Executed, pc, op, {}};
    }

    // MRS CPSR/SPSR, Rd
    if ((op & 0x0FBF0FFFu) == 0x010F0000u) {
        const bool use_spsr = (op & (1u << 22)) != 0u;
        const unsigned rd = (op >> 12) & 0xFu;
        const std::uint32_t value = use_spsr ? arm7_get_spsr(ctx) : ctx.cpsr;
        if (rd == 15u) ctx.r[15] = value & ~3u;
        else { ctx.r[rd] = value; ctx.r[15] = pc + 4u; }
        return {ARM7StepStatus::Executed, pc, op, {}};
    }

    // MSR CPSR/SPSR_<fields>, Rm or rotated immediate.
    // Bit 25 (immediate/register) is intentionally outside the fixed mask.
    if ((op & 0x0DB0F000u) == 0x0120F000u) {
        const bool immediate = (op & (1u << 25)) != 0u;
        const bool use_spsr = (op & (1u << 22)) != 0u;
        const std::uint32_t fields = (op >> 16) & 0xFu;
        std::uint32_t value = 0u;
        if (immediate) {
            const unsigned rot = ((op >> 8) & 0xFu) * 2u;
            value = std::rotr(op & 0xFFu, static_cast<int>(rot));
        } else {
            value = read_reg(ctx, op & 0xFu, pc);
        }
        if (use_spsr) arm7_set_spsr(ctx, value, fields);
        else arm7_set_cpsr(ctx, value, fields);
        ctx.r[15] = pc + 4u;
        return {ARM7StepStatus::Executed, pc, op, {}};
    }

    // Multiply / multiply-accumulate.
    if ((op & 0x0FC000F0u) == 0x00000090u) {
        const bool accumulate = (op & (1u << 21)) != 0u;
        const bool set_flags = (op & (1u << 20)) != 0u;
        const unsigned rd = (op >> 16) & 0xFu;
        const unsigned rn = (op >> 12) & 0xFu;
        const unsigned rs = (op >> 8) & 0xFu;
        const unsigned rm = op & 0xFu;
        std::uint32_t result = read_reg(ctx, rm, pc) * read_reg(ctx, rs, pc);
        if (accumulate) result += read_reg(ctx, rn, pc);
        if (rd == 15u) return unsupported(ctx, pc, op, "MUL/MLA with R15 destination");
        ctx.r[rd] = result;
        if (set_flags) set_nz(ctx, result);
        ctx.r[15] = pc + 4u;
        return {ARM7StepStatus::Executed, pc, op, {}};
    }

    // Halfword and signed data transfer.
    if ((op & 0x0E000090u) == 0x00000090u && (op & 0x60u) != 0u) {
        const bool pre = (op & (1u << 24)) != 0u;
        const bool up = (op & (1u << 23)) != 0u;
        const bool immediate = (op & (1u << 22)) != 0u;
        const bool writeback = (op & (1u << 21)) != 0u;
        const bool load = (op & (1u << 20)) != 0u;
        const unsigned rn = (op >> 16) & 0xFu;
        const unsigned rd = (op >> 12) & 0xFu;
        std::uint32_t offset = immediate ? (((op >> 4) & 0xF0u) | (op & 0xFu)) : read_reg(ctx, op & 0xFu, pc);
        const std::uint32_t base = read_reg(ctx, rn, pc);
        const std::uint32_t indexed = up ? base + offset : base - offset;
        const std::uint32_t address = pre ? indexed : base;
        const unsigned kind = (op >> 5) & 3u;
        try {
            if (load) {
                std::uint32_t value = 0u;
                if (kind == 1u) value = bus.read16(address);
                else if (kind == 2u) value = static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int8_t>(bus.read8(address))));
                else if (kind == 3u) value = static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(bus.read16(address))));
                else return unsupported(ctx, pc, op, "unsupported halfword transfer kind");
                if (rd == 15u) ctx.r[15] = value & ~3u;
                else ctx.r[rd] = value;
            } else {
                if (kind != 1u) return unsupported(ctx, pc, op, "signed halfword store encoding");
                bus.write16(address, static_cast<std::uint16_t>(read_reg(ctx, rd, pc)));
            }
        } catch (const std::exception& e) { return fault(ctx, pc, op, e.what()); }
        if (!pre || writeback) {
            if (rn == 15u) return unsupported(ctx, pc, op, "writeback to R15");
            ctx.r[rn] = indexed;
        }
        if (!(load && rd == 15u)) ctx.r[15] = pc + 4u;
        return {ARM7StepStatus::Executed, pc, op, {}};
    }

    // Single data transfer LDR/STR/LDRB/STRB.
    if ((op & 0x0C000000u) == 0x04000000u) {
        const bool reg_offset = (op & (1u << 25)) != 0u;
        const bool pre = (op & (1u << 24)) != 0u;
        const bool up = (op & (1u << 23)) != 0u;
        const bool byte = (op & (1u << 22)) != 0u;
        const bool writeback = (op & (1u << 21)) != 0u;
        const bool load = (op & (1u << 20)) != 0u;
        const unsigned rn = (op >> 16) & 0xFu;
        const unsigned rd = (op >> 12) & 0xFu;
        std::uint32_t offset = op & 0xFFFu;
        if (reg_offset) {
            if (op & (1u << 4)) return unsupported(ctx, pc, op, "register-shifted register offset in LDR/STR");
            const unsigned rm = op & 0xFu;
            const unsigned type = (op >> 5) & 3u;
            const unsigned amount = (op >> 7) & 0x1Fu;
            offset = shift_value(read_reg(ctx, rm, pc), type, amount, (ctx.cpsr & C) != 0u, true).value;
        }
        const std::uint32_t base = read_reg(ctx, rn, pc);
        const std::uint32_t indexed = up ? base + offset : base - offset;
        const std::uint32_t address = pre ? indexed : base;
        try {
            if (load) {
                std::uint32_t value = 0u;
                if (byte) {
                    value = bus.read8(address);
                } else {
                    // ARM7TDMI / ARMv4T word loads from an unaligned address read the
                    // aligned 32-bit word and rotate it right by 8 * addr[1:0].
                    // Old ARM code relies on this for packed 16-bit tables.
                    value = bus.read32(address & ~3u);
                    const unsigned rotate = (address & 3u) * 8u;
                    if (rotate != 0u) value = std::rotr(value, static_cast<int>(rotate));
                }
                if (rd == 15u) ctx.r[15] = value & ~3u;
                else ctx.r[rd] = value;
            } else {
                const std::uint32_t value = read_reg(ctx, rd, pc);
                if (byte) bus.write8(address, static_cast<std::uint8_t>(value));
                else bus.write32(address & ~3u, value);
            }
        } catch (const std::exception& e) { return fault(ctx, pc, op, e.what()); }
        if (!pre || writeback) {
            if (rn == 15u) return unsupported(ctx, pc, op, "writeback to R15");
            ctx.r[rn] = indexed;
        }
        if (!(load && rd == 15u)) ctx.r[15] = pc + 4u;
        return {ARM7StepStatus::Executed, pc, op, {}};
    }

    // Block data transfer LDM/STM.
    if ((op & 0x0E000000u) == 0x08000000u) {
        const bool pre = (op & (1u << 24)) != 0u;
        const bool up = (op & (1u << 23)) != 0u;
        const bool psr_user = (op & (1u << 22)) != 0u;
        const bool writeback = (op & (1u << 21)) != 0u;
        const bool load = (op & (1u << 20)) != 0u;
        const unsigned rn = (op >> 16) & 0xFu;
        const std::uint32_t list = op & 0xFFFFu;
        if (rn == 15u) return unsupported(ctx, pc, op, "LDM/STM base R15");
        const unsigned count = std::popcount(list);
        if (count == 0u) return unsupported(ctx, pc, op, "empty LDM/STM register list");

        const bool list_has_pc = (list & (1u << 15)) != 0u;
        const std::uint32_t mode = ctx.cpsr & MODE_MASK;
        // ARM's S/^ bit has two distinct meanings for block transfers:
        //   * without PC in the list, access the User/System register bank;
        //   * LDM with PC in the list, restore CPSR from the current SPSR.
        const bool exception_return = psr_user && load && list_has_pc;
        const bool user_bank_transfer = psr_user && !exception_return;
        if (exception_return && !mode_has_spsr(mode))
            return unsupported(ctx, pc, op, "LDM^ PC exception return without SPSR");

        const std::uint32_t base = ctx.r[rn];
        std::uint32_t address;
        if (up) address = base + (pre ? 4u : 0u);
        else address = base - (pre ? 4u * count : 4u * (count - 1u));
        bool loaded_pc = false;
        std::uint32_t loaded_pc_value = 0u;
        try {
            for (unsigned reg = 0; reg < 16; ++reg) {
                if ((list & (1u << reg)) == 0u) continue;
                if (load) {
                    const auto value = bus.read32(address);
                    if (reg == 15u) {
                        loaded_pc_value = value;
                        loaded_pc = true;
                    } else if (user_bank_transfer) {
                        write_user_reg(ctx, reg, value);
                    } else {
                        ctx.r[reg] = value;
                    }
                } else {
                    // ARM7TDMI block stores expose R15 as the address of the
                    // STM instruction plus 12 bytes (two pipeline words plus
                    // the additional block-transfer store offset).  Using the
                    // generic PC+8 operand value here breaks hand-written call
                    // stubs such as `STMDB sp!, {pc}` / `LDM sp!, {pc}`.
                    const auto value = reg == 15u ? pc + 12u
                        : (user_bank_transfer ? read_user_reg(ctx, reg, pc)
                                              : read_reg(ctx, reg, pc));
                    bus.write32(address, value);
                }
                address += 4u;
            }
        } catch (const std::exception& e) { return fault(ctx, pc, op, e.what()); }
        if (writeback) ctx.r[rn] = up ? base + 4u * count : base - 4u * count;
        if (exception_return) {
            const std::uint32_t restored = arm7_get_spsr(ctx);
            arm7_set_cpsr(ctx, restored, 0xFu);
            ctx.r[15] = loaded_pc_value & ((ctx.cpsr & T) ? ~1u : ~3u);
        } else if (loaded_pc) {
            ctx.r[15] = loaded_pc_value & ~3u;
        } else {
            ctx.r[15] = pc + 4u;
        }
        return {ARM7StepStatus::Executed, pc, op, {}};
    }

    // Branch / branch with link.
    if ((op & 0x0E000000u) == 0x0A000000u) {
        const bool link = (op & (1u << 24)) != 0u;
        std::int32_t disp = static_cast<std::int32_t>((op & 0x00FFFFFFu) << 8) >> 6;
        if (link) ctx.r[14] = pc + 4u;
        ctx.r[15] = static_cast<std::uint32_t>(pc + 8u + disp);
        return {ARM7StepStatus::Executed, pc, op, {}};
    }

    // Data processing / compare.
    if ((op & 0x0C000000u) == 0u) {
        const unsigned opcode = (op >> 21) & 0xFu;
        const bool set_flags_bit = (op & (1u << 20)) != 0u;
        const unsigned rn = (op >> 16) & 0xFu;
        const unsigned rd = (op >> 12) & 0xFu;
        const auto sh = decode_operand2(ctx, op, pc);
        const std::uint32_t a = read_reg(ctx, rn, pc);
        const bool test_only = opcode >= 8u && opcode <= 11u;
        std::uint32_t result = 0u;
        bool arithmetic_flags_done = false;
        switch (opcode) {
            case 0x0: result = a & sh.value; break;
            case 0x1: result = a ^ sh.value; break;
            case 0x2: result = sub_with_flags(ctx, a, sh.value, 0u, set_flags_bit || test_only); arithmetic_flags_done = true; break;
            case 0x3: result = sub_with_flags(ctx, sh.value, a, 0u, set_flags_bit || test_only); arithmetic_flags_done = true; break;
            case 0x4: result = add_with_carry(ctx, a, sh.value, 0u, set_flags_bit || test_only); arithmetic_flags_done = true; break;
            case 0x5: result = add_with_carry(ctx, a, sh.value, (ctx.cpsr & C) ? 1u : 0u, set_flags_bit || test_only); arithmetic_flags_done = true; break;
            case 0x6: result = sub_with_flags(ctx, a, sh.value, (ctx.cpsr & C) ? 0u : 1u, set_flags_bit || test_only); arithmetic_flags_done = true; break;
            case 0x7: result = sub_with_flags(ctx, sh.value, a, (ctx.cpsr & C) ? 0u : 1u, set_flags_bit || test_only); arithmetic_flags_done = true; break;
            case 0x8: result = a & sh.value; break; // TST
            case 0x9: result = a ^ sh.value; break; // TEQ
            case 0xA: result = sub_with_flags(ctx, a, sh.value, 0u, true); arithmetic_flags_done = true; break; // CMP
            case 0xB: result = add_with_carry(ctx, a, sh.value, 0u, true); arithmetic_flags_done = true; break; // CMN
            case 0xC: result = a | sh.value; break;
            case 0xD: result = sh.value; break;
            case 0xE: result = a & ~sh.value; break;
            case 0xF: result = ~sh.value; break;
        }
        if ((set_flags_bit || test_only) && !arithmetic_flags_done)
            set_logic_flags(ctx, result, sh.carry_valid, sh.carry);
        if (!test_only) {
            if (rd == 15u) {
                if (set_flags_bit) arm7_set_cpsr(ctx, arm7_get_spsr(ctx));
                ctx.r[15] = result & ~3u;
            } else {
                ctx.r[rd] = result;
                ctx.r[15] = pc + 4u;
            }
        } else {
            ctx.r[15] = pc + 4u;
        }
        return {ARM7StepStatus::Executed, pc, op, {}};
    }

    std::ostringstream detail;
    detail << "unsupported ARMv4T opcode 0x" << std::hex << op;
    return unsupported(ctx, pc, op, detail.str());
}

std::uint64_t arm7_run(ARM7Context& ctx, ARM7Bus& bus, std::uint64_t max_instructions,
                       ARM7StepResult* last_result) {
    std::uint64_t executed = 0u;
    ARM7StepResult last{};
    while (executed < max_instructions && !ctx.halted) {
        last = arm7_step(ctx, bus);
        if (last.status != ARM7StepStatus::Executed) break;
        ++executed;
    }
    if (last_result) *last_result = last;
    return executed;
}

} // namespace dcrecomp
