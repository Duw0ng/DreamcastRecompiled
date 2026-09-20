#include "dcrecomp/arm7.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

class FlatBus final : public dcrecomp::ARM7Bus {
public:
    explicit FlatBus(std::size_t size = 0x10000) : memory_(size, 0) {}

    std::uint8_t read8(std::uint32_t address) override {
        check(address, 1);
        return memory_[address];
    }

    std::uint16_t read16(std::uint32_t address) override {
        check(address, 2);
        return static_cast<std::uint16_t>(memory_[address]) |
               static_cast<std::uint16_t>(memory_[address + 1]) << 8;
    }

    std::uint32_t read32(std::uint32_t address) override {
        check(address, 4);
        return static_cast<std::uint32_t>(memory_[address]) |
               static_cast<std::uint32_t>(memory_[address + 1]) << 8 |
               static_cast<std::uint32_t>(memory_[address + 2]) << 16 |
               static_cast<std::uint32_t>(memory_[address + 3]) << 24;
    }

    void write8(std::uint32_t address, std::uint8_t value) override {
        check(address, 1);
        memory_[address] = value;
    }

    void write16(std::uint32_t address, std::uint16_t value) override {
        check(address, 2);
        memory_[address] = static_cast<std::uint8_t>(value);
        memory_[address + 1] = static_cast<std::uint8_t>(value >> 8);
    }

    void write32(std::uint32_t address, std::uint32_t value) override {
        check(address, 4);
        memory_[address] = static_cast<std::uint8_t>(value);
        memory_[address + 1] = static_cast<std::uint8_t>(value >> 8);
        memory_[address + 2] = static_cast<std::uint8_t>(value >> 16);
        memory_[address + 3] = static_cast<std::uint8_t>(value >> 24);
    }

private:
    void check(std::uint32_t address, std::size_t width) const {
        if (static_cast<std::size_t>(address) + width > memory_.size())
            throw std::out_of_range("ARM7 flat-bus access outside test memory");
    }

    std::vector<std::uint8_t> memory_;
};

void test_arithmetic_conditions() {
    FlatBus bus;
    // MOV r0,#1; ADD r0,#41; CMP r0,#42; BNE fail; MOV r1,#7; B end;
    // fail: MOV r1,#255; end: MOV r2,r2 (NOP).
    const std::array<std::uint32_t, 8> code{
        0xE3A00001u, 0xE2800029u, 0xE350002Au, 0x1A000001u,
        0xE3A01007u, 0xEA000000u, 0xE3A010FFu, 0xE1A02002u,
    };
    for (std::size_t i = 0; i < code.size(); ++i) bus.write32(static_cast<std::uint32_t>(i * 4), code[i]);

    dcrecomp::ARM7Context ctx;
    dcrecomp::arm7_reset(ctx);
    dcrecomp::ARM7StepResult last;
    require(dcrecomp::arm7_run(ctx, bus, 6, &last) == 6, "ARM7 executes arithmetic/condition sequence");
    require(ctx.r[0] == 42u, "ADD produces 42");
    require(ctx.r[1] == 7u, "failed BNE condition does not take the error path");
    require((ctx.cpsr & (1u << 30)) != 0u, "CMP sets Z");
    require(ctx.r[15] == 0x1Cu, "unconditional branch lands at end label");
}

void test_load_store() {
    FlatBus bus;
    bus.write32(0x00, 0xE5810004u); // STR r0,[r1,#4]
    bus.write32(0x04, 0xE5912004u); // LDR r2,[r1,#4]

    dcrecomp::ARM7Context ctx;
    dcrecomp::arm7_reset(ctx);
    ctx.r[0] = 0x12345678u;
    ctx.r[1] = 0x100u;
    require(dcrecomp::arm7_run(ctx, bus, 2) == 2, "ARM7 executes STR/LDR pair");
    require(bus.read32(0x104u) == 0x12345678u, "STR writes little-endian word to bus");
    require(ctx.r[2] == 0x12345678u, "LDR reloads stored word");
}


void test_unaligned_word_load_rotation() {
    FlatBus bus;
    bus.write32(0x00, 0xE5912002u); // LDR r2,[r1,#2]
    bus.write32(0x100, 0x11223344u);

    dcrecomp::ARM7Context ctx;
    dcrecomp::arm7_reset(ctx);
    ctx.r[1] = 0x100u;
    require(dcrecomp::arm7_run(ctx, bus, 1) == 1, "unaligned ARM7 LDR executes");
    require(ctx.r[2] == 0x33441122u, "ARM7TDMI unaligned word LDR rotates aligned word by address low bits");
}

void test_branch_link_and_bx() {
    FlatBus bus;
    bus.write32(0x00, 0xEB000002u); // BL 0x10
    bus.write32(0x10, 0xE3A00005u); // MOV r0,#5
    bus.write32(0x14, 0xE12FFF1Eu); // BX lr

    dcrecomp::ARM7Context ctx;
    dcrecomp::arm7_reset(ctx);
    require(dcrecomp::arm7_run(ctx, bus, 3) == 3, "ARM7 executes BL/return sequence");
    require(ctx.r[0] == 5u, "branch target executed");
    require(ctx.r[14] == 4u, "BL stores ARM return address in LR");
    require(ctx.r[15] == 4u, "BX LR returns to instruction after BL");
}

void test_mrs_msr_and_banked_sp() {
    FlatBus bus;
    // Same mode-switch idiom used by ARM firmware startup code.
    bus.write32(0x00, 0xE10F0000u); // MRS r0,CPSR
    bus.write32(0x04, 0xE3C0001Fu); // BIC r0,r0,#0x1f
    bus.write32(0x08, 0xE3800012u); // ORR r0,r0,#IRQ
    bus.write32(0x0C, 0xE121F000u); // MSR CPSR_c,r0
    bus.write32(0x10, 0xE321F0D3u); // MSR CPSR_c,#0xd3 (SVC)

    dcrecomp::ARM7Context ctx;
    dcrecomp::arm7_reset(ctx);
    ctx.r[13] = 0x1111u;
    require(dcrecomp::arm7_run(ctx, bus, 4) == 4, "MRS/BIC/ORR/MSR changes ARM mode");
    require((ctx.cpsr & 0x1Fu) == 0x12u, "MSR enters IRQ mode");
    require(ctx.r[13] == 0u, "IRQ mode exposes its own banked SP");
    require(ctx.svc_r13_r14[0] == 0x1111u, "SVC SP is saved during mode switch");

    ctx.r[13] = 0x2222u;
    require(dcrecomp::arm7_run(ctx, bus, 1) == 1, "immediate MSR encoding is decoded");
    require((ctx.cpsr & 0x1Fu) == 0x13u, "immediate MSR returns to SVC mode");
    require(ctx.r[13] == 0x1111u, "SVC banked SP is restored");
    require(ctx.irq_r13_r14[0] == 0x2222u, "IRQ SP is preserved in its bank");
}

void test_block_transfer() {
    FlatBus bus;
    bus.write32(0x00, 0xE8A840FFu); // STMIA r8!,{r0-r7,lr}
    bus.write32(0x04, 0xE8B840FFu); // LDMIA r8!,{r0-r7,lr}

    dcrecomp::ARM7Context ctx;
    dcrecomp::arm7_reset(ctx);
    for (unsigned i = 0; i < 8; ++i) ctx.r[i] = 0x1000u + i;
    ctx.r[14] = 0xDEADBEEFu;
    ctx.r[8] = 0x200u;
    require(dcrecomp::arm7_run(ctx, bus, 1) == 1, "STMIA executes");
    require(ctx.r[8] == 0x224u, "STMIA writeback advances base by register count");
    require(bus.read32(0x200u) == 0x1000u && bus.read32(0x220u) == 0xDEADBEEFu,
            "STMIA writes low-to-high register list");

    for (unsigned i = 0; i < 8; ++i) ctx.r[i] = 0u;
    ctx.r[14] = 0u;
    ctx.r[8] = 0x200u;
    require(dcrecomp::arm7_run(ctx, bus, 1) == 1, "LDMIA executes");
    require(ctx.r[0] == 0x1000u && ctx.r[7] == 0x1007u && ctx.r[14] == 0xDEADBEEFu,
            "LDMIA restores register list");
    require(ctx.r[8] == 0x224u, "LDMIA writeback advances base");

    // ARM7TDMI exposes PC+12 when R15 is stored by STM.  Crazy Taxi 2's
    // AICA driver uses this exact idiom as a tiny call/return convention:
    // STMDB sp!,{pc}; ...; B target; target returns with LDM sp!,{pc}.
    bus.write32(0x100u, 0xE92D8000u); // STMDB sp!,{pc}
    ctx.r[15] = 0x100u;
    ctx.r[13] = 0x304u;
    require(dcrecomp::arm7_run(ctx, bus, 1) == 1, "STMDB sp!,{pc} executes");
    require(ctx.r[13] == 0x300u, "STMDB sp!,{pc} decrements SP");
    require(bus.read32(0x300u) == 0x10Cu, "STM stores ARM7TDMI PC+12 value");
}



void test_block_transfer_user_bank_and_exception_return() {
    FlatBus bus;
    // STMIA r0!,{r13,r14}^ ; LDMIA r0!,{r13,r14}^
    bus.write32(0x00, 0xE8E06000u);
    bus.write32(0x04, 0xE8F06000u);
    // LDMIA r0!,{r1,pc}^ -- exception return form.
    bus.write32(0x08, 0xE8F08002u);

    dcrecomp::ARM7Context ctx;
    dcrecomp::arm7_reset(ctx);
    ctx.r[13] = 0xAAAA1111u; // live SVC bank
    ctx.r[14] = 0xBBBB2222u;
    ctx.usr_r13_r14 = {0x11112222u, 0x33334444u};
    ctx.r[0] = 0x300u;

    require(dcrecomp::arm7_run(ctx, bus, 1) == 1, "STM^ user-bank transfer executes");
    require(bus.read32(0x300u) == 0x11112222u && bus.read32(0x304u) == 0x33334444u,
            "STM^ stores User/System SP/LR rather than the active SVC bank");
    require(ctx.r[13] == 0xAAAA1111u && ctx.r[14] == 0xBBBB2222u,
            "STM^ leaves active exception-bank SP/LR unchanged");

    bus.write32(0x320u, 0x55556666u);
    bus.write32(0x324u, 0x77778888u);
    ctx.r[15] = 0x04u;
    ctx.r[0] = 0x320u;
    require(dcrecomp::arm7_run(ctx, bus, 1) == 1, "LDM^ user-bank transfer executes");
    require(ctx.usr_r13_r14[0] == 0x55556666u && ctx.usr_r13_r14[1] == 0x77778888u,
            "LDM^ loads User/System SP/LR while privileged");
    require(ctx.r[13] == 0xAAAA1111u && ctx.r[14] == 0xBBBB2222u,
            "LDM^ user-bank transfer preserves active SVC SP/LR");

    // Exception return: the register transfer uses the current bank and then
    // CPSR is restored from SPSR, which exposes the User bank.
    bus.write32(0x340u, 0xCAFEBABEu);
    bus.write32(0x344u, 0x00000101u);
    ctx.r[15] = 0x08u;
    ctx.r[0] = 0x340u;
    dcrecomp::arm7_set_spsr(ctx, 0x60000030u); // User + Thumb, flags preserved.
    require(dcrecomp::arm7_run(ctx, bus, 1) == 1, "LDM^ with PC performs exception return");
    require(ctx.r[1] == 0xCAFEBABEu, "exception-return LDM loads ordinary registers");
    require((ctx.cpsr & 0x1Fu) == 0x10u && (ctx.cpsr & (1u << 5)) != 0u,
            "exception-return LDM restores CPSR from SPSR");
    require(ctx.r[15] == 0x100u, "exception-return LDM aligns PC according to restored Thumb state");
}

void test_fiq_exception_entry_and_return() {
    FlatBus bus;
    bus.write32(0x00, 0xE3A00001u); // MOV r0,#1
    bus.write32(0x04, 0xE3A01007u); // MOV r1,#7 (resume point)
    bus.write32(0x1C, 0xE2800001u); // FIQ: ADD r0,r0,#1
    bus.write32(0x20, 0xE25EF004u); // SUBS pc,lr,#4

    dcrecomp::ARM7Context ctx;
    dcrecomp::arm7_reset(ctx);
    // Reset starts with FIQ masked. Clear only F while remaining in SVC.
    dcrecomp::arm7_set_cpsr(ctx, ctx.cpsr & ~(1u << 6));
    require(dcrecomp::arm7_run(ctx, bus, 1) == 1, "ARM7 executes instruction before FIQ");
    require(ctx.r[0] == 1u && ctx.r[15] == 4u, "pre-FIQ instruction completed");

    dcrecomp::arm7_set_fiq_line(ctx, true);
    auto entry = dcrecomp::arm7_step(ctx, bus);
    require(entry.status == dcrecomp::ARM7StepStatus::Executed, "FIQ exception entry is scheduled");
    require((ctx.cpsr & 0x1Fu) == 0x11u, "FIQ enters banked FIQ mode");
    require((ctx.cpsr & (1u << 6)) != 0u && (ctx.cpsr & (1u << 7)) != 0u,
            "FIQ masks FIQ and IRQ on exception entry");
    require(ctx.r[14] == 8u && ctx.r[15] == 0x1Cu, "FIQ LR/vector use ARM7 exception return convention");
    require(ctx.fiq_exceptions == 1u, "FIQ exception counter increments");

    // Model a level source being acknowledged while the handler runs.
    dcrecomp::arm7_set_fiq_line(ctx, false);
    require(dcrecomp::arm7_run(ctx, bus, 2) == 2, "FIQ handler executes and returns");
    require(ctx.r[0] == 2u, "FIQ handler can modify shared low registers");
    require(ctx.r[15] == 4u, "SUBS pc,lr,#4 resumes interrupted instruction");
    require((ctx.cpsr & 0x1Fu) == 0x13u && (ctx.cpsr & (1u << 6)) == 0u,
            "exception return restores SPSR/CPSR and SVC mode");
    require(dcrecomp::arm7_run(ctx, bus, 1) == 1 && ctx.r[1] == 7u,
            "execution continues at the interrupted ARM instruction");
}

void test_irq_exception_entry_and_masking() {
    FlatBus bus;
    bus.write32(0x00, 0xE1A00000u); // NOP
    bus.write32(0x18, 0xE25EF004u); // IRQ: SUBS pc,lr,#4

    dcrecomp::ARM7Context ctx;
    dcrecomp::arm7_reset(ctx);
    dcrecomp::arm7_set_irq_line(ctx, true);
    auto masked = dcrecomp::arm7_step(ctx, bus);
    require(masked.status == dcrecomp::ARM7StepStatus::Executed && ctx.r[15] == 4u,
            "masked IRQ does not preempt normal ARM execution");
    require(ctx.irq_exceptions == 0u, "masked IRQ is not counted as an exception");

    ctx.r[15] = 0u;
    dcrecomp::arm7_set_cpsr(ctx, ctx.cpsr & ~(1u << 7));
    auto entry = dcrecomp::arm7_step(ctx, bus);
    require(entry.status == dcrecomp::ARM7StepStatus::Executed && ctx.r[15] == 0x18u,
            "unmasked IRQ vectors to 0x18");
    require((ctx.cpsr & 0x1Fu) == 0x12u && ctx.irq_exceptions == 1u,
            "IRQ enters IRQ bank and updates statistics");
}

void test_thumb_transition_is_explicitly_reported() {
    FlatBus bus(0x1000);
    bus.write32(0x00, 0xE12FFF10u); // BX r0
    dcrecomp::ARM7Context ctx;
    dcrecomp::arm7_reset(ctx);
    ctx.r[0] = 0x101u;
    auto first = dcrecomp::arm7_step(ctx, bus);
    require(first.status == dcrecomp::ARM7StepStatus::Executed, "BX to odd address enters Thumb state");
    require((ctx.cpsr & (1u << 5)) != 0u && ctx.r[15] == 0x100u, "BX sets T bit and aligns Thumb PC");
    auto second = dcrecomp::arm7_step(ctx, bus);
    require(second.status == dcrecomp::ARM7StepStatus::Unsupported, "bootstrap core reports Thumb as unsupported instead of mis-decoding it as ARM");
    require(ctx.halted, "unsupported instruction halts bootstrap interpreter deterministically");
}

} // namespace

int main() {
    test_arithmetic_conditions();
    test_load_store();
    test_unaligned_word_load_rotation();
    test_branch_link_and_bx();
    test_mrs_msr_and_banked_sp();
    test_block_transfer();
    test_block_transfer_user_bank_and_exception_return();
    test_fiq_exception_entry_and_return();
    test_irq_exception_entry_and_masking();
    test_thumb_transition_is_explicitly_reported();
    std::cout << "ARM7 bootstrap tests passed\n";
    return 0;
}
