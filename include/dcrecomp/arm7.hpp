#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace dcrecomp {

class ARM7Bus {
public:
    virtual ~ARM7Bus() = default;
    virtual std::uint8_t read8(std::uint32_t address) = 0;
    virtual std::uint16_t read16(std::uint32_t address) = 0;
    virtual std::uint32_t read32(std::uint32_t address) = 0;
    virtual void write8(std::uint32_t address, std::uint8_t value) = 0;
    virtual void write16(std::uint32_t address, std::uint16_t value) = 0;
    virtual void write32(std::uint32_t address, std::uint32_t value) = 0;
};

struct ARM7Context {
    std::array<std::uint32_t, 16> r{};
    std::uint32_t cpsr{0x000000D3u}; // SVC, IRQ/FIQ masked; ARM state.

    // Banked ARM7 registers. These are intentionally explicit so the AICA
    // bootstrap can switch SVC/FIQ modes like real firmware startup code.
    std::array<std::uint32_t, 5> common_r8_r12{};
    std::array<std::uint32_t, 7> fiq_r8_r14{};
    std::array<std::uint32_t, 2> usr_r13_r14{};
    std::array<std::uint32_t, 2> irq_r13_r14{};
    std::array<std::uint32_t, 2> svc_r13_r14{};
    std::array<std::uint32_t, 2> abt_r13_r14{};
    std::array<std::uint32_t, 2> und_r13_r14{};

    std::uint32_t spsr_fiq{};
    std::uint32_t spsr_irq{};
    std::uint32_t spsr_svc{};
    std::uint32_t spsr_abt{};
    std::uint32_t spsr_und{};

    std::uint64_t instructions{};
    std::uint64_t irq_exceptions{};
    std::uint64_t fiq_exceptions{};
    bool irq_line{};
    bool fiq_line{};
    bool halted{};
};

enum class ARM7StepStatus {
    Executed,
    Halted,
    Unsupported,
    Fault
};

struct ARM7StepResult {
    ARM7StepStatus status{ARM7StepStatus::Executed};
    std::uint32_t pc{};
    std::uint32_t opcode{};
    std::string detail;
};

void arm7_reset(ARM7Context& ctx, std::uint32_t pc = 0u);
void arm7_set_cpsr(ARM7Context& ctx, std::uint32_t value, std::uint32_t field_mask = 0xFu);
std::uint32_t arm7_get_spsr(const ARM7Context& ctx);
void arm7_set_spsr(ARM7Context& ctx, std::uint32_t value, std::uint32_t field_mask = 0xFu);
void arm7_set_irq_line(ARM7Context& ctx, bool asserted);
void arm7_set_fiq_line(ARM7Context& ctx, bool asserted);

ARM7StepResult arm7_step(ARM7Context& ctx, ARM7Bus& bus);
std::uint64_t arm7_run(ARM7Context& ctx, ARM7Bus& bus, std::uint64_t max_instructions,
                       ARM7StepResult* last_result = nullptr);

} // namespace dcrecomp
