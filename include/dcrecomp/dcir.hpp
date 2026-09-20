#pragma once

#include "dcrecomp/cfg.hpp"
#include "dcrecomp/elf32.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace dcrecomp {

enum class DCIROp {
    Nop,
    ClearT,
    SetT,
    ClearS,
    SetS,
    ClearMac,
    Sleep,
    LdTlb,
    Trapa,
    Pref,
    MovcaL,
    Ocbi,
    Ocbp,
    Ocbwb,
    MovImm,
    MovReg,
    Mova,
    LoadLiteral16,
    LoadLiteral32,

    // Memory operations. Loads preserve SH-4 MOV.B/MOV.W sign-extension
    // semantics; post-increment/pre-decrement variants update the address register.
    Load8,
    Load16,
    Load32,
    Load8PostInc,
    Load16PostInc,
    Load32PostInc,
    Store8,
    Store16,
    Store32,
    Store8PreDec,
    Store16PreDec,
    Store32PreDec,
    Load8Disp,
    Load16Disp,
    Load32Disp,
    Store8Disp,
    Store16Disp,
    Store32Disp,
    Load8Indexed,
    Load16Indexed,
    Load32Indexed,
    Store8Indexed,
    Store16Indexed,
    Store32Indexed,

    // FPU arithmetic / compare. The generated runtime selects the active bank
    // through FPSCR.FR and chooses FR (single) vs DR (double) through FPSCR.PR.
    // FMAC is architecturally single precision only.
    Fadd,
    Fsub,
    Fmul,
    Fdiv,
    FcmpEq,
    FcmpGt,
    Fmac,

    // FPU unary/conversion/vector operations.
    Fsts,
    Flds,
    Float,
    Ftrc,
    Fneg,
    Fabs,
    Fsqrt,
    Fsrra,
    Fldi0,
    Fldi1,
    Fcnvsd,
    Fcnvds,
    Fipr,
    Ftrv,
    Fsca,
    Frchg,
    Fschg,

    // FPU move operations. These preserve the raw IEEE-754 bit pattern; the
    // generated runtime selects the active FR/XF bank from FPSCR.FR. 0.0.27
    // intentionally guards the FPSCR.SZ=1 64-bit transfer mode until the
    // paired DR/XD register semantics are implemented.
    FmovLoad,
    FmovLoadPostInc,
    FmovStore,
    FmovStorePreDec,
    FmovReg,
    FmovIndexedLoad,
    FmovIndexedStore,

    AddImm,
    AddReg,
    Addc,
    Addv,
    SubReg,
    Subc,
    Subv,
    Neg,
    Negc,
    AndReg,
    XorReg,
    OrReg,
    NotReg,
    MulL,
    MuluW,
    MulsW,
    DmuluL,
    DmulsL,
    SwapB,
    SwapW,
    Xtrct,
    AndImm,
    XorImm,
    OrImm,
    ExtuB,
    ExtuW,
    ExtsB,
    ExtsW,
    Shll,
    Shlr,
    Shal,
    Shar,
    Shll2,
    Shlr2,
    Shll8,
    Shlr8,
    Shll16,
    Shlr16,
    Rotl,
    Rotr,
    Rotcl,
    Rotcr,
    Shld,
    Shad,

    // SH-4 T-bit / compare operations used by conditional control flow.
    CmpEqImm,
    CmpEq,
    CmpStr,
    CmpHs,
    CmpGe,
    CmpHi,
    CmpGt,
    CmpPz,
    CmpPl,
    TstImm,
    TstReg,
    Dt,
    Div0U,
    Div0S,
    Div1,
    MovT,
    TasB,

    // Special-register transfers.
    StsPr,
    LdsPr,
    StsMach,
    StsMacl,
    StsFpul,
    StsFpscr,
    LdsMach,
    LdsMacl,
    LdsFpul,
    LdsFpscr,

    StcSr, StcGbr, StcVbr, StcSsr, StcSpc, StcSgr, StcDbr, StcBank,
    StcLSr, StcLGbr, StcLVbr, StcLSsr, StcLSpc, StcLSgr, StcLDbr, StcLBank,
    LdcSr, LdcGbr, LdcVbr, LdcSsr, LdcSpc, LdcSgr, LdcDbr, LdcBank,
    LdcLSr, LdcLGbr, LdcLVbr, LdcLSsr, LdcLSpc, LdcLSgr, LdcLDbr, LdcLBank,
    StsLMach, StsLMacl, StsLFpul, StsLFpscr,
    LdsLMach, LdsLMacl, LdsLFpul, LdsLFpscr,

    PushPR,
    PopPR,
    Call,
    Return,
    Branch,
    SaveDynamicAbsoluteTarget,
    SaveDynamicRelativeTarget,
    DynamicBranch,
    DynamicCall,
    PrepareRte,
    Rte,
    // Delayed conditional branches (BT/S, BF/S) sample T before the delay slot.
    // SaveT + BranchIfSaved* preserves that architectural ordering even when
    // the delay-slot instruction itself changes T.
    SaveT,
    BranchIfTrue,
    BranchIfFalse,
    BranchIfSavedTrue,
    BranchIfSavedFalse,
    RawSH4,
};

struct DCIRInstruction {
    DCIROp op{DCIROp::RawSH4};
    std::uint32_t source_address{};
    std::uint8_t dst{};
    std::uint8_t src{};
    std::int32_t immediate{};
    std::uint32_t value{};
    std::uint32_t target{};
    std::string symbol;
    std::string section;
    std::string text;
};

struct DCIRBlock {
    std::uint32_t start_address{};
    std::vector<DCIRInstruction> instructions;
};

struct DCIRFunction {
    std::string name;
    std::uint32_t entry{};
    std::vector<DCIRBlock> blocks;
};

DCIRFunction lower_to_dcir(const Elf32Image& elf,
                           const FunctionAnalysis& analysis,
                           const ControlFlowGraph& cfg);

const char* to_string(DCIROp op);
std::string format_dcir(const DCIRInstruction& instruction);

} // namespace dcrecomp
