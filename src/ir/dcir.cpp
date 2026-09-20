#include "dcrecomp/dcir.hpp"

#include <algorithm>
#include <iomanip>
#include <optional>
#include <sstream>
#include <unordered_set>

namespace dcrecomp {
namespace {

const LiteralReference* literal_for(const FunctionAnalysis& a, std::uint32_t address) {
    for (const auto& literal : a.literals) {
        if (literal.instruction_address == address) return &literal;
    }
    return nullptr;
}

const CallReference* call_for(const FunctionAnalysis& a, std::uint32_t address) {
    for (const auto& call : a.calls) {
        if (call.instruction_address == address) return &call;
    }
    return nullptr;
}

DCIRInstruction lower_one(const FunctionAnalysis& analysis, const sh4::Instruction& i) {
    using sh4::Opcode;
    DCIRInstruction out;
    out.source_address = i.address;

    switch (i.opcode) {
        case Opcode::Nop:
            out.op = DCIROp::Nop;
            break;
        case Opcode::ClrT:
            out.op = DCIROp::ClearT;
            break;
        case Opcode::SetT:
            out.op = DCIROp::SetT;
            break;
        case Opcode::ClrS:
            out.op = DCIROp::ClearS;
            break;
        case Opcode::SetS:
            out.op = DCIROp::SetS;
            break;
        case Opcode::Sleep:
            out.op = DCIROp::Sleep;
            break;
        case Opcode::LdTlb:
            out.op = DCIROp::LdTlb;
            break;
        case Opcode::Trapa:
            out.op = DCIROp::Trapa; out.immediate = i.immediate;
            break;
        case Opcode::Pref:
            out.op = DCIROp::Pref; out.dst = i.rn;
            break;
        case Opcode::MovcaL:
            out.op = DCIROp::MovcaL; out.dst = i.rn;
            break;
        case Opcode::Ocbi:
            out.op = DCIROp::Ocbi; out.dst = i.rn;
            break;
        case Opcode::Ocbp:
            out.op = DCIROp::Ocbp; out.dst = i.rn;
            break;
        case Opcode::Ocbwb:
            out.op = DCIROp::Ocbwb; out.dst = i.rn;
            break;
        case Opcode::ClrMac:
            out.op = DCIROp::ClearMac;
            break;
        case Opcode::MovImm:
            out.op = DCIROp::MovImm;
            out.dst = i.rn;
            out.immediate = i.immediate;
            break;
        case Opcode::MovReg:
            out.op = DCIROp::MovReg;
            out.dst = i.rn;
            out.src = i.rm;
            break;
        case Opcode::Mova:
            out.op = DCIROp::Mova; out.dst = 0; out.value = i.effective_address; break;
        case Opcode::MovLPcRel:
        case Opcode::MovWPcRel: {
            out.op = i.opcode == Opcode::MovLPcRel ? DCIROp::LoadLiteral32 : DCIROp::LoadLiteral16;
            out.dst = i.rn;
            if (const auto* literal = literal_for(analysis, i.address)) {
                out.value = literal->value;
                out.target = literal->storage_address;
                out.symbol = literal->symbol;
                out.section = literal->section;
            }
            break;
        }
        case Opcode::MovBLoad:
            out.op = DCIROp::Load8; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::MovWLoad:
            out.op = DCIROp::Load16; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::MovLLoad:
            out.op = DCIROp::Load32; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::MovBPostinc:
            out.op = DCIROp::Load8PostInc; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::MovWPostinc:
            out.op = DCIROp::Load16PostInc; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::MovLPostinc:
            out.op = DCIROp::Load32PostInc; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::MovBStore:
            out.op = DCIROp::Store8; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::MovWStore:
            out.op = DCIROp::Store16; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::MovLStore:
            out.op = DCIROp::Store32; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::MovBPredec:
            out.op = DCIROp::Store8PreDec; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::MovWPredec:
            out.op = DCIROp::Store16PreDec; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::MovLPredec:
            out.op = DCIROp::Store32PreDec; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::MovBDispLoad:
            out.op = DCIROp::Load8Disp; out.dst = 0; out.src = i.rm; out.immediate = i.immediate; break;
        case Opcode::MovWDispLoad:
            out.op = DCIROp::Load16Disp; out.dst = 0; out.src = i.rm; out.immediate = i.immediate; break;
        case Opcode::MovLDispLoad:
            out.op = DCIROp::Load32Disp; out.dst = i.rn; out.src = i.rm; out.immediate = i.immediate * 4; break;
        case Opcode::MovBDispStore:
            out.op = DCIROp::Store8Disp; out.dst = i.rn; out.src = 0; out.immediate = i.immediate; break;
        case Opcode::MovWDispStore:
            out.op = DCIROp::Store16Disp; out.dst = i.rn; out.src = 0; out.immediate = i.immediate; break;
        case Opcode::MovLDispStore:
            out.op = DCIROp::Store32Disp; out.dst = i.rn; out.src = i.rm; out.immediate = i.immediate * 4; break;
        case Opcode::MovBIndexedLoad:
            out.op = DCIROp::Load8Indexed; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::MovWIndexedLoad:
            out.op = DCIROp::Load16Indexed; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::MovLIndexedLoad:
            out.op = DCIROp::Load32Indexed; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::MovBIndexedStore:
            out.op = DCIROp::Store8Indexed; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::MovWIndexedStore:
            out.op = DCIROp::Store16Indexed; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::MovLIndexedStore:
            out.op = DCIROp::Store32Indexed; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::Fadd:
            out.op = DCIROp::Fadd; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::Fsub:
            out.op = DCIROp::Fsub; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::Fmul:
            out.op = DCIROp::Fmul; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::Fdiv:
            out.op = DCIROp::Fdiv; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::FcmpEq:
            out.op = DCIROp::FcmpEq; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::FcmpGt:
            out.op = DCIROp::FcmpGt; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::Fmac:
            out.op = DCIROp::Fmac; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::Fsts:
            out.op = DCIROp::Fsts; out.dst = i.rn; break;
        case Opcode::Flds:
            out.op = DCIROp::Flds; out.src = i.rn; break;
        case Opcode::Float:
            out.op = DCIROp::Float; out.dst = i.rn; break;
        case Opcode::Ftrc:
            out.op = DCIROp::Ftrc; out.src = i.rn; break;
        case Opcode::Fneg:
            out.op = DCIROp::Fneg; out.dst = i.rn; break;
        case Opcode::Fabs:
            out.op = DCIROp::Fabs; out.dst = i.rn; break;
        case Opcode::Fsqrt:
            out.op = DCIROp::Fsqrt; out.dst = i.rn; break;
        case Opcode::Fsrra:
            out.op = DCIROp::Fsrra; out.dst = i.rn; break;
        case Opcode::Fldi0:
            out.op = DCIROp::Fldi0; out.dst = i.rn; break;
        case Opcode::Fldi1:
            out.op = DCIROp::Fldi1; out.dst = i.rn; break;
        case Opcode::Fcnvsd:
            out.op = DCIROp::Fcnvsd; out.dst = i.rn; break;
        case Opcode::Fcnvds:
            out.op = DCIROp::Fcnvds; out.src = i.rn; break;
        case Opcode::Fipr:
            out.op = DCIROp::Fipr; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::Ftrv:
            out.op = DCIROp::Ftrv; out.dst = i.rn; break;
        case Opcode::Fsca:
            out.op = DCIROp::Fsca; out.dst = i.rn; break;
        case Opcode::Frchg:
            out.op = DCIROp::Frchg; break;
        case Opcode::Fschg:
            out.op = DCIROp::Fschg; break;
        case Opcode::FmovLoad:
            out.op = DCIROp::FmovLoad; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::FmovLoadPostInc:
            out.op = DCIROp::FmovLoadPostInc; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::FmovStore:
            out.op = DCIROp::FmovStore; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::FmovStorePreDec:
            out.op = DCIROp::FmovStorePreDec; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::FmovReg:
            out.op = DCIROp::FmovReg; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::FmovIndexedLoad:
            out.op = DCIROp::FmovIndexedLoad; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::FmovIndexedStore:
            out.op = DCIROp::FmovIndexedStore; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::AddImm:
            out.op = DCIROp::AddImm;
            out.dst = i.rn;
            out.immediate = i.immediate;
            break;
        case Opcode::AddReg:
            out.op = DCIROp::AddReg;
            out.dst = i.rn;
            out.src = i.rm;
            break;
        case Opcode::Addc:
            out.op = DCIROp::Addc; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::Addv:
            out.op = DCIROp::Addv; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::SubReg:
            out.op = DCIROp::SubReg; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::Subc:
            out.op = DCIROp::Subc; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::Subv:
            out.op = DCIROp::Subv; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::Neg:
            out.op = DCIROp::Neg; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::Negc:
            out.op = DCIROp::Negc; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::AndReg:
            out.op = DCIROp::AndReg; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::XorReg:
            out.op = DCIROp::XorReg; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::OrReg:
            out.op = DCIROp::OrReg; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::Not:
            out.op = DCIROp::NotReg; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::MulL:
            out.op = DCIROp::MulL; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::MuluW:
            out.op = DCIROp::MuluW; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::MulsW:
            out.op = DCIROp::MulsW; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::DmuluL:
            out.op = DCIROp::DmuluL; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::DmulsL:
            out.op = DCIROp::DmulsL; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::SwapB:
            out.op = DCIROp::SwapB; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::SwapW:
            out.op = DCIROp::SwapW; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::Xtrct:
            out.op = DCIROp::Xtrct; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::AndImm:
            out.op = DCIROp::AndImm; out.dst = 0; out.immediate = i.immediate; break;
        case Opcode::XorImm:
            out.op = DCIROp::XorImm; out.dst = 0; out.immediate = i.immediate; break;
        case Opcode::OrImm:
            out.op = DCIROp::OrImm; out.dst = 0; out.immediate = i.immediate; break;
        case Opcode::ExtuB:
            out.op = DCIROp::ExtuB; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::ExtuW:
            out.op = DCIROp::ExtuW; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::ExtsB:
            out.op = DCIROp::ExtsB; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::ExtsW:
            out.op = DCIROp::ExtsW; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::Shll: out.op = DCIROp::Shll; out.dst = i.rn; break;
        case Opcode::Shlr: out.op = DCIROp::Shlr; out.dst = i.rn; break;
        case Opcode::Shal: out.op = DCIROp::Shal; out.dst = i.rn; break;
        case Opcode::Shar: out.op = DCIROp::Shar; out.dst = i.rn; break;
        case Opcode::Shll2: out.op = DCIROp::Shll2; out.dst = i.rn; break;
        case Opcode::Shlr2: out.op = DCIROp::Shlr2; out.dst = i.rn; break;
        case Opcode::Shll8: out.op = DCIROp::Shll8; out.dst = i.rn; break;
        case Opcode::Shlr8: out.op = DCIROp::Shlr8; out.dst = i.rn; break;
        case Opcode::Shll16: out.op = DCIROp::Shll16; out.dst = i.rn; break;
        case Opcode::Shlr16: out.op = DCIROp::Shlr16; out.dst = i.rn; break;
        case Opcode::Rotl: out.op = DCIROp::Rotl; out.dst = i.rn; break;
        case Opcode::Rotr: out.op = DCIROp::Rotr; out.dst = i.rn; break;
        case Opcode::Rotcl: out.op = DCIROp::Rotcl; out.dst = i.rn; break;
        case Opcode::Rotcr: out.op = DCIROp::Rotcr; out.dst = i.rn; break;
        case Opcode::Shld: out.op = DCIROp::Shld; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::Shad: out.op = DCIROp::Shad; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::CmpEqImm:
            out.op = DCIROp::CmpEqImm;
            out.immediate = i.immediate;
            break;
        case Opcode::CmpEq:
            out.op = DCIROp::CmpEq; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::CmpStr:
            out.op = DCIROp::CmpStr; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::CmpHs:
            out.op = DCIROp::CmpHs; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::CmpGe:
            out.op = DCIROp::CmpGe; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::CmpHi:
            out.op = DCIROp::CmpHi; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::CmpGt:
            out.op = DCIROp::CmpGt; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::CmpPz:
            out.op = DCIROp::CmpPz; out.dst = i.rn; break;
        case Opcode::CmpPl:
            out.op = DCIROp::CmpPl; out.dst = i.rn; break;
        case Opcode::TstImm:
            out.op = DCIROp::TstImm; out.immediate = i.immediate; break;
        case Opcode::TstReg:
            out.op = DCIROp::TstReg; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::Dt:
            out.op = DCIROp::Dt; out.dst = i.rn; break;
        case Opcode::Div0U:
            out.op = DCIROp::Div0U; break;
        case Opcode::Div0S:
            out.op = DCIROp::Div0S; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::Div1:
            out.op = DCIROp::Div1; out.dst = i.rn; out.src = i.rm; break;
        case Opcode::Movt:
            out.op = DCIROp::MovT; out.dst = i.rn; break;
        case Opcode::TasB:
            out.op = DCIROp::TasB; out.dst = i.rn; break;
        case Opcode::StsMach:
            out.op = DCIROp::StsMach; out.dst = i.rn; break;
        case Opcode::StsMacl:
            out.op = DCIROp::StsMacl; out.dst = i.rn; break;
        case Opcode::StsFpul:
            out.op = DCIROp::StsFpul; out.dst = i.rn; break;
        case Opcode::StsFpscr:
            out.op = DCIROp::StsFpscr; out.dst = i.rn; break;
        case Opcode::LdsMach:
            out.op = DCIROp::LdsMach; out.src = i.rm; break;
        case Opcode::LdsMacl:
            out.op = DCIROp::LdsMacl; out.src = i.rm; break;
        case Opcode::LdsFpul:
            out.op = DCIROp::LdsFpul; out.src = i.rm; break;
        case Opcode::LdsFpscr:
            out.op = DCIROp::LdsFpscr; out.src = i.rm; break;

        case Opcode::StcSr: out.op = DCIROp::StcSr; out.dst = i.rn; break;
        case Opcode::StcGbr: out.op = DCIROp::StcGbr; out.dst = i.rn; break;
        case Opcode::StcVbr: out.op = DCIROp::StcVbr; out.dst = i.rn; break;
        case Opcode::StcSsr: out.op = DCIROp::StcSsr; out.dst = i.rn; break;
        case Opcode::StcSpc: out.op = DCIROp::StcSpc; out.dst = i.rn; break;
        case Opcode::StcSgr: out.op = DCIROp::StcSgr; out.dst = i.rn; break;
        case Opcode::StcDbr: out.op = DCIROp::StcDbr; out.dst = i.rn; break;
        case Opcode::StcBank: out.op = DCIROp::StcBank; out.dst = i.rn; out.immediate = i.immediate; break;
        case Opcode::StcLSr: out.op = DCIROp::StcLSr; out.dst = i.rn; break;
        case Opcode::StcLGbr: out.op = DCIROp::StcLGbr; out.dst = i.rn; break;
        case Opcode::StcLVbr: out.op = DCIROp::StcLVbr; out.dst = i.rn; break;
        case Opcode::StcLSsr: out.op = DCIROp::StcLSsr; out.dst = i.rn; break;
        case Opcode::StcLSpc: out.op = DCIROp::StcLSpc; out.dst = i.rn; break;
        case Opcode::StcLSgr: out.op = DCIROp::StcLSgr; out.dst = i.rn; break;
        case Opcode::StcLDbr: out.op = DCIROp::StcLDbr; out.dst = i.rn; break;
        case Opcode::StcLBank: out.op = DCIROp::StcLBank; out.dst = i.rn; out.immediate = i.immediate; break;
        case Opcode::LdcSr: out.op = DCIROp::LdcSr; out.src = i.rm; break;
        case Opcode::LdcGbr: out.op = DCIROp::LdcGbr; out.src = i.rm; break;
        case Opcode::LdcVbr: out.op = DCIROp::LdcVbr; out.src = i.rm; break;
        case Opcode::LdcSsr: out.op = DCIROp::LdcSsr; out.src = i.rm; break;
        case Opcode::LdcSpc: out.op = DCIROp::LdcSpc; out.src = i.rm; break;
        case Opcode::LdcSgr: out.op = DCIROp::LdcSgr; out.src = i.rm; break;
        case Opcode::LdcDbr: out.op = DCIROp::LdcDbr; out.src = i.rm; break;
        case Opcode::LdcBank: out.op = DCIROp::LdcBank; out.src = i.rm; out.immediate = i.immediate; break;
        case Opcode::LdcLSr: out.op = DCIROp::LdcLSr; out.src = i.rm; break;
        case Opcode::LdcLGbr: out.op = DCIROp::LdcLGbr; out.src = i.rm; break;
        case Opcode::LdcLVbr: out.op = DCIROp::LdcLVbr; out.src = i.rm; break;
        case Opcode::LdcLSsr: out.op = DCIROp::LdcLSsr; out.src = i.rm; break;
        case Opcode::LdcLSpc: out.op = DCIROp::LdcLSpc; out.src = i.rm; break;
        case Opcode::LdcLSgr: out.op = DCIROp::LdcLSgr; out.src = i.rm; break;
        case Opcode::LdcLDbr: out.op = DCIROp::LdcLDbr; out.src = i.rm; break;
        case Opcode::LdcLBank: out.op = DCIROp::LdcLBank; out.src = i.rm; out.immediate = i.immediate; break;
        case Opcode::StsLMach: out.op = DCIROp::StsLMach; out.dst = i.rn; break;
        case Opcode::StsLMacl: out.op = DCIROp::StsLMacl; out.dst = i.rn; break;
        case Opcode::StsLFpul: out.op = DCIROp::StsLFpul; out.dst = i.rn; break;
        case Opcode::StsLFpscr: out.op = DCIROp::StsLFpscr; out.dst = i.rn; break;
        case Opcode::LdsLMach: out.op = DCIROp::LdsLMach; out.src = i.rm; break;
        case Opcode::LdsLMacl: out.op = DCIROp::LdsLMacl; out.src = i.rm; break;
        case Opcode::LdsLFpul: out.op = DCIROp::LdsLFpul; out.src = i.rm; break;
        case Opcode::LdsLFpscr: out.op = DCIROp::LdsLFpscr; out.src = i.rm; break;

        case Opcode::Rte:
            out.op = DCIROp::Rte;
            break;
        case Opcode::StsPr:
            out.op = DCIROp::StsPr; out.dst = i.rn; break;
        case Opcode::LdsPr:
            out.op = DCIROp::LdsPr; out.src = i.rm; break;
        case Opcode::StsLPr:
            out.op = DCIROp::PushPR;
            out.dst = i.rn;
            break;
        case Opcode::LdsLPr:
            out.op = DCIROp::PopPR;
            out.src = i.rm;
            break;
        case Opcode::Jsr: {
            // JSR @Rn is architecturally dynamic even when data-flow analysis can
            // prove one observed target. Retail Katana code reuses the same call
            // site with different function pointers across runtime iterations, so
            // baking a single analyzed target into native code is incorrect.
            out.op = DCIROp::DynamicCall;
            out.src = i.rm;
            break;
        }
        case Opcode::Bsr:
            out.op = DCIROp::Call; out.target = i.target; break;
        case Opcode::Rts:
            out.op = DCIROp::Return;
            break;
        case Opcode::Jmp:
            out.op = DCIROp::DynamicBranch; out.src = i.rm; break;
        case Opcode::Bra:
            out.op = DCIROp::Branch;
            out.target = i.target;
            break;
        case Opcode::Braf:
            out.op = DCIROp::DynamicBranch;
            out.src = i.rn;
            break;
        case Opcode::Bsrf:
            out.op = DCIROp::DynamicCall;
            out.src = i.rn;
            break;
        case Opcode::Bt:
        case Opcode::BtS:
            out.op = DCIROp::BranchIfTrue;
            out.target = i.target;
            break;
        case Opcode::Bf:
        case Opcode::BfS:
            out.op = DCIROp::BranchIfFalse;
            out.target = i.target;
            break;
        default:
            out.op = DCIROp::RawSH4;
            out.text = sh4::to_string(i);
            break;
    }
    return out;
}

bool has_delay_slot(const sh4::Instruction& i) { return i.has_delay_slot; }

} // namespace

const char* to_string(DCIROp op) {
    switch (op) {
        case DCIROp::Nop: return "NOP";
        case DCIROp::ClearT: return "CLEAR_T";
        case DCIROp::SetT: return "SET_T";
        case DCIROp::ClearS: return "CLEAR_S";
        case DCIROp::SetS: return "SET_S";
        case DCIROp::ClearMac: return "CLEAR_MAC";
        case DCIROp::Sleep: return "SLEEP";
        case DCIROp::LdTlb: return "LDTLB";
        case DCIROp::Trapa: return "TRAPA";
        case DCIROp::Pref: return "PREF";
        case DCIROp::MovcaL: return "MOVCA_L";
        case DCIROp::Ocbi: return "OCBI";
        case DCIROp::Ocbp: return "OCBP";
        case DCIROp::Ocbwb: return "OCBWB";
        case DCIROp::MovImm: return "MOV_IMM";
        case DCIROp::MovReg: return "MOV_REG";
        case DCIROp::Mova: return "MOVA";
        case DCIROp::LoadLiteral16: return "LOAD_LITERAL16";
        case DCIROp::LoadLiteral32: return "LOAD_LITERAL32";
        case DCIROp::Load8: return "LOAD8";
        case DCIROp::Load16: return "LOAD16";
        case DCIROp::Load32: return "LOAD32";
        case DCIROp::Load8PostInc: return "LOAD8_POSTINC";
        case DCIROp::Load16PostInc: return "LOAD16_POSTINC";
        case DCIROp::Load32PostInc: return "LOAD32_POSTINC";
        case DCIROp::Store8: return "STORE8";
        case DCIROp::Store16: return "STORE16";
        case DCIROp::Store32: return "STORE32";
        case DCIROp::Store8PreDec: return "STORE8_PREDEC";
        case DCIROp::Store16PreDec: return "STORE16_PREDEC";
        case DCIROp::Store32PreDec: return "STORE32_PREDEC";
        case DCIROp::Load8Disp: return "LOAD8_DISP";
        case DCIROp::Load16Disp: return "LOAD16_DISP";
        case DCIROp::Load32Disp: return "LOAD32_DISP";
        case DCIROp::Store8Disp: return "STORE8_DISP";
        case DCIROp::Store16Disp: return "STORE16_DISP";
        case DCIROp::Store32Disp: return "STORE32_DISP";
        case DCIROp::Load8Indexed: return "LOAD8_INDEXED";
        case DCIROp::Load16Indexed: return "LOAD16_INDEXED";
        case DCIROp::Load32Indexed: return "LOAD32_INDEXED";
        case DCIROp::Store8Indexed: return "STORE8_INDEXED";
        case DCIROp::Store16Indexed: return "STORE16_INDEXED";
        case DCIROp::Store32Indexed: return "STORE32_INDEXED";
        case DCIROp::Fadd: return "FADD";
        case DCIROp::Fsub: return "FSUB";
        case DCIROp::Fmul: return "FMUL";
        case DCIROp::Fdiv: return "FDIV";
        case DCIROp::FcmpEq: return "FCMP_EQ";
        case DCIROp::FcmpGt: return "FCMP_GT";
        case DCIROp::Fmac: return "FMAC";
        case DCIROp::Fsts: return "FSTS";
        case DCIROp::Flds: return "FLDS";
        case DCIROp::Float: return "FLOAT";
        case DCIROp::Ftrc: return "FTRC";
        case DCIROp::Fneg: return "FNEG";
        case DCIROp::Fabs: return "FABS";
        case DCIROp::Fsqrt: return "FSQRT";
        case DCIROp::Fsrra: return "FSRRA";
        case DCIROp::Fldi0: return "FLDI0";
        case DCIROp::Fldi1: return "FLDI1";
        case DCIROp::Fcnvsd: return "FCNVSD";
        case DCIROp::Fcnvds: return "FCNVDS";
        case DCIROp::Fipr: return "FIPR";
        case DCIROp::Ftrv: return "FTRV";
        case DCIROp::Fsca: return "FSCA";
        case DCIROp::Frchg: return "FRCHG";
        case DCIROp::Fschg: return "FSCHG";
        case DCIROp::FmovLoad: return "FMOV_LOAD";
        case DCIROp::FmovLoadPostInc: return "FMOV_LOAD_POSTINC";
        case DCIROp::FmovStore: return "FMOV_STORE";
        case DCIROp::FmovStorePreDec: return "FMOV_STORE_PREDEC";
        case DCIROp::FmovReg: return "FMOV_REG";
        case DCIROp::FmovIndexedLoad: return "FMOV_INDEXED_LOAD";
        case DCIROp::FmovIndexedStore: return "FMOV_INDEXED_STORE";
        case DCIROp::AddImm: return "ADD_IMM";
        case DCIROp::AddReg: return "ADD_REG";
        case DCIROp::Addc: return "ADDC";
        case DCIROp::Addv: return "ADDV";
        case DCIROp::SubReg: return "SUB_REG";
        case DCIROp::Subc: return "SUBC";
        case DCIROp::Subv: return "SUBV";
        case DCIROp::Neg: return "NEG";
        case DCIROp::Negc: return "NEGC";
        case DCIROp::AndReg: return "AND_REG";
        case DCIROp::XorReg: return "XOR_REG";
        case DCIROp::OrReg: return "OR_REG";
        case DCIROp::NotReg: return "NOT_REG";
        case DCIROp::MulL: return "MUL_L";
        case DCIROp::MuluW: return "MULU_W";
        case DCIROp::MulsW: return "MULS_W";
        case DCIROp::DmuluL: return "DMULU_L";
        case DCIROp::DmulsL: return "DMULS_L";
        case DCIROp::SwapB: return "SWAP_B";
        case DCIROp::SwapW: return "SWAP_W";
        case DCIROp::Xtrct: return "XTRCT";
        case DCIROp::AndImm: return "AND_IMM";
        case DCIROp::XorImm: return "XOR_IMM";
        case DCIROp::OrImm: return "OR_IMM";
        case DCIROp::ExtuB: return "EXTU_B";
        case DCIROp::ExtuW: return "EXTU_W";
        case DCIROp::ExtsB: return "EXTS_B";
        case DCIROp::ExtsW: return "EXTS_W";
        case DCIROp::Shll: return "SHLL";
        case DCIROp::Shlr: return "SHLR";
        case DCIROp::Shal: return "SHAL";
        case DCIROp::Shar: return "SHAR";
        case DCIROp::Shll2: return "SHLL2";
        case DCIROp::Shlr2: return "SHLR2";
        case DCIROp::Shll8: return "SHLL8";
        case DCIROp::Shlr8: return "SHLR8";
        case DCIROp::Shll16: return "SHLL16";
        case DCIROp::Shlr16: return "SHLR16";
        case DCIROp::Rotl: return "ROTL";
        case DCIROp::Rotr: return "ROTR";
        case DCIROp::Rotcl: return "ROTCL";
        case DCIROp::Rotcr: return "ROTCR";
        case DCIROp::Shld: return "SHLD";
        case DCIROp::Shad: return "SHAD";
        case DCIROp::CmpEqImm: return "CMP_EQ_IMM";
        case DCIROp::CmpEq: return "CMP_EQ";
        case DCIROp::CmpStr: return "CMP_STR";
        case DCIROp::CmpHs: return "CMP_HS";
        case DCIROp::CmpGe: return "CMP_GE";
        case DCIROp::CmpHi: return "CMP_HI";
        case DCIROp::CmpGt: return "CMP_GT";
        case DCIROp::CmpPz: return "CMP_PZ";
        case DCIROp::CmpPl: return "CMP_PL";
        case DCIROp::TstImm: return "TST_IMM";
        case DCIROp::TstReg: return "TST_REG";
        case DCIROp::Dt: return "DT";
        case DCIROp::Div0U: return "DIV0U";
        case DCIROp::Div0S: return "DIV0S";
        case DCIROp::Div1: return "DIV1";
        case DCIROp::MovT: return "MOV_T";
        case DCIROp::TasB: return "TAS_B";
        case DCIROp::StsPr: return "STS_PR";
        case DCIROp::LdsPr: return "LDS_PR";
        case DCIROp::StsMach: return "STS_MACH";
        case DCIROp::StsMacl: return "STS_MACL";
        case DCIROp::StsFpul: return "STS_FPUL";
        case DCIROp::StsFpscr: return "STS_FPSCR";
        case DCIROp::LdsMach: return "LDS_MACH";
        case DCIROp::LdsMacl: return "LDS_MACL";
        case DCIROp::LdsFpul: return "LDS_FPUL";
        case DCIROp::LdsFpscr: return "LDS_FPSCR";
        case DCIROp::StcSr: return "STC_SR";
        case DCIROp::StcGbr: return "STC_GBR";
        case DCIROp::StcVbr: return "STC_VBR";
        case DCIROp::StcSsr: return "STC_SSR";
        case DCIROp::StcSpc: return "STC_SPC";
        case DCIROp::StcSgr: return "STC_SGR";
        case DCIROp::StcDbr: return "STC_DBR";
        case DCIROp::StcBank: return "STC_BANK";
        case DCIROp::StcLSr: return "STC_L_SR";
        case DCIROp::StcLGbr: return "STC_L_GBR";
        case DCIROp::StcLVbr: return "STC_L_VBR";
        case DCIROp::StcLSsr: return "STC_L_SSR";
        case DCIROp::StcLSpc: return "STC_L_SPC";
        case DCIROp::StcLSgr: return "STC_L_SGR";
        case DCIROp::StcLDbr: return "STC_L_DBR";
        case DCIROp::StcLBank: return "STC_L_BANK";
        case DCIROp::LdcSr: return "LDC_SR";
        case DCIROp::LdcGbr: return "LDC_GBR";
        case DCIROp::LdcVbr: return "LDC_VBR";
        case DCIROp::LdcSsr: return "LDC_SSR";
        case DCIROp::LdcSpc: return "LDC_SPC";
        case DCIROp::LdcSgr: return "LDC_SGR";
        case DCIROp::LdcDbr: return "LDC_DBR";
        case DCIROp::LdcBank: return "LDC_BANK";
        case DCIROp::LdcLSr: return "LDC_L_SR";
        case DCIROp::LdcLGbr: return "LDC_L_GBR";
        case DCIROp::LdcLVbr: return "LDC_L_VBR";
        case DCIROp::LdcLSsr: return "LDC_L_SSR";
        case DCIROp::LdcLSpc: return "LDC_L_SPC";
        case DCIROp::LdcLSgr: return "LDC_L_SGR";
        case DCIROp::LdcLDbr: return "LDC_L_DBR";
        case DCIROp::LdcLBank: return "LDC_L_BANK";
        case DCIROp::StsLMach: return "STS_L_MACH";
        case DCIROp::StsLMacl: return "STS_L_MACL";
        case DCIROp::StsLFpul: return "STS_L_FPUL";
        case DCIROp::StsLFpscr: return "STS_L_FPSCR";
        case DCIROp::LdsLMach: return "LDS_L_MACH";
        case DCIROp::LdsLMacl: return "LDS_L_MACL";
        case DCIROp::LdsLFpul: return "LDS_L_FPUL";
        case DCIROp::LdsLFpscr: return "LDS_L_FPSCR";
        case DCIROp::PushPR: return "PUSH_PR";
        case DCIROp::PopPR: return "POP_PR";
        case DCIROp::Call: return "CALL";
        case DCIROp::Return: return "RETURN";
        case DCIROp::Branch: return "BRANCH";
        case DCIROp::SaveDynamicAbsoluteTarget: return "SAVE_DYNAMIC_ABS_TARGET";
        case DCIROp::SaveDynamicRelativeTarget: return "SAVE_DYNAMIC_REL_TARGET";
        case DCIROp::DynamicBranch: return "DYNAMIC_BRANCH";
        case DCIROp::DynamicCall: return "DYNAMIC_CALL";
        case DCIROp::PrepareRte: return "PREPARE_RTE";
        case DCIROp::Rte: return "RTE";
        case DCIROp::SaveT: return "SAVE_T";
        case DCIROp::BranchIfTrue: return "BRANCH_IF_TRUE";
        case DCIROp::BranchIfFalse: return "BRANCH_IF_FALSE";
        case DCIROp::BranchIfSavedTrue: return "BRANCH_IF_SAVED_TRUE";
        case DCIROp::BranchIfSavedFalse: return "BRANCH_IF_SAVED_FALSE";
        case DCIROp::RawSH4: return "RAW_SH4";
    }
    return "UNKNOWN";
}

DCIRFunction lower_to_dcir(const Elf32Image& elf,
                           const FunctionAnalysis& analysis,
                           const ControlFlowGraph& cfg) {
    (void)elf;
    DCIRFunction function;
    function.name = analysis.name;
    function.entry = analysis.start_address;

    for (const auto& block : cfg.blocks) {
        DCIRBlock out_block;
        out_block.start_address = block.start_address;

        for (std::size_t i = 0; i < block.instructions.size(); ++i) {
            const auto& insn = block.instructions[i];

            // SH-4 executes the delay-slot instruction before the control transfer,
            // but the control instruction itself can sample architectural state first.
            // In particular BT/S and BF/S sample T *before* the slot. If the slot
            // changes T (GCC does this in KallistiOS memTestAddressBus), simply
            // lowering slot -> branch changes the branch decision. Preserve the
            // sampled T explicitly in DCIR.
            if (has_delay_slot(insn)) {
                // A delay-slot instruction can also be a legal basic-block entry.
                // Example: a BT/S may branch directly to the instruction that is
                // simultaneously the delay slot of a fall-through JMP @Rn. In that
                // shape CFG construction must split the slot into its own block, but
                // the delayed control transfer still has to execute it locally and
                // snapshot any dynamic target before the slot runs.
                const sh4::Instruction* slot_ptr = nullptr;
                bool slot_is_next_in_block = false;
                if (i + 1 < block.instructions.size() &&
                    block.instructions[i + 1].address == insn.address + 2) {
                    slot_ptr = &block.instructions[i + 1];
                    slot_is_next_in_block = true;
                } else {
                    const auto slot_it = std::find_if(analysis.instructions.begin(), analysis.instructions.end(),
                        [&](const sh4::Instruction& candidate) { return candidate.address == insn.address + 2; });
                    if (slot_it != analysis.instructions.end()) slot_ptr = &*slot_it;
                }
                if (slot_ptr) {
                const auto& slot = *slot_ptr;

                const bool delayed_bt = insn.opcode == sh4::Opcode::BtS;
                const bool delayed_bf = insn.opcode == sh4::Opcode::BfS;
                const bool delayed_rte = insn.opcode == sh4::Opcode::Rte;
                const bool dynamic_relative = insn.opcode == sh4::Opcode::Braf || insn.opcode == sh4::Opcode::Bsrf;
                // JSR @Rn samples its dynamic register target before the delay slot
                // just like JMP @Rn. Preserve that target unconditionally; a resolved
                // analysis target is only a closure hint, never a runtime constant.
                const bool dynamic_jsr = insn.opcode == sh4::Opcode::Jsr;
                const bool dynamic_absolute = insn.opcode == sh4::Opcode::Jmp || dynamic_jsr;
                if (delayed_rte) {
                    // SH-4 RTE restores SR from SSR before the delay slot executes,
                    // while the return target comes from SPC. Preserve both facts
                    // explicitly instead of treating RTE like a normal delayed branch.
                    DCIRInstruction prepare_rte;
                    prepare_rte.op = DCIROp::PrepareRte;
                    prepare_rte.source_address = insn.address;
                    out_block.instructions.push_back(std::move(prepare_rte));
                }
                if (dynamic_relative || dynamic_absolute) {
                    DCIRInstruction save_target;
                    save_target.op = dynamic_relative ? DCIROp::SaveDynamicRelativeTarget : DCIROp::SaveDynamicAbsoluteTarget;
                    save_target.source_address = insn.address;
                    save_target.src = dynamic_relative ? insn.rn : insn.rm;
                    out_block.instructions.push_back(std::move(save_target));
                }
                if (delayed_bt || delayed_bf) {
                    DCIRInstruction save_t;
                    save_t.op = DCIROp::SaveT;
                    save_t.source_address = insn.address;
                    out_block.instructions.push_back(std::move(save_t));
                }

                auto lowered_slot = lower_one(analysis, slot);
                // NOP delay slots add no semantics to the IR, so omit them.
                if (lowered_slot.op != DCIROp::Nop) {
                    out_block.instructions.push_back(std::move(lowered_slot));
                }

                auto lowered_control = lower_one(analysis, insn);
                if (delayed_bt) lowered_control.op = DCIROp::BranchIfSavedTrue;
                if (delayed_bf) lowered_control.op = DCIROp::BranchIfSavedFalse;
                out_block.instructions.push_back(std::move(lowered_control));
                if (slot_is_next_in_block) ++i;
                continue;
                }
            }

            out_block.instructions.push_back(lower_one(analysis, insn));
        }

        function.blocks.push_back(std::move(out_block));
    }

    return function;
}

std::string format_dcir(const DCIRInstruction& i) {
    std::ostringstream out;
    out << std::left << std::setw(24) << to_string(i.op);

    auto r = [&](std::uint8_t reg) { out << "R" << static_cast<unsigned>(reg); };
    auto fr = [&](std::uint8_t reg) { out << "FR" << static_cast<unsigned>(reg); };
    auto hex32 = [&](std::uint32_t value) {
        out << "0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << value
            << std::setfill(' ') << std::dec;
    };

    switch (i.op) {
        case DCIROp::MovImm:
            r(i.dst); out << ", " << i.immediate;
            break;
        case DCIROp::MovReg:
            r(i.dst); out << ", "; r(i.src);
            break;
        case DCIROp::Mova:
            r(i.dst); out << ", "; hex32(i.value);
            break;
        case DCIROp::LoadLiteral16:
        case DCIROp::LoadLiteral32:
            r(i.dst); out << ", "; hex32(i.value);
            if (!i.symbol.empty()) out << "  ; " << i.symbol;
            else if (!i.section.empty()) out << "  ; " << i.section;
            break;
        case DCIROp::Load8:
        case DCIROp::Load16:
        case DCIROp::Load32:
            r(i.dst); out << ", @"; r(i.src);
            break;
        case DCIROp::Load8PostInc:
        case DCIROp::Load16PostInc:
        case DCIROp::Load32PostInc:
            r(i.dst); out << ", @"; r(i.src); out << "+";
            break;
        case DCIROp::Store8:
        case DCIROp::Store16:
        case DCIROp::Store32:
            out << "@"; r(i.dst); out << ", "; r(i.src);
            break;
        case DCIROp::Store8PreDec:
        case DCIROp::Store16PreDec:
        case DCIROp::Store32PreDec:
            out << "@-"; r(i.dst); out << ", "; r(i.src);
            break;
        case DCIROp::Load8Disp:
        case DCIROp::Load16Disp:
        case DCIROp::Load32Disp:
            r(i.dst); out << ", @(" << i.immediate << ","; r(i.src); out << ")";
            break;
        case DCIROp::Store8Disp:
        case DCIROp::Store16Disp:
        case DCIROp::Store32Disp:
            out << "@(" << i.immediate << ","; r(i.dst); out << "), "; r(i.src);
            break;
        case DCIROp::Load8Indexed:
        case DCIROp::Load16Indexed:
        case DCIROp::Load32Indexed:
            r(i.dst); out << ", @(R0,"; r(i.src); out << ")";
            break;
        case DCIROp::Store8Indexed:
        case DCIROp::Store16Indexed:
        case DCIROp::Store32Indexed:
            out << "@(R0,"; r(i.dst); out << "), "; r(i.src);
            break;
        case DCIROp::Fadd:
        case DCIROp::Fsub:
        case DCIROp::Fmul:
        case DCIROp::Fdiv:
        case DCIROp::FcmpEq:
        case DCIROp::FcmpGt:
            fr(i.dst); out << ", "; fr(i.src);
            break;
        case DCIROp::Fmac:
            fr(i.dst); out << ", FR0, "; fr(i.src);
            break;
        case DCIROp::Fsts:
            fr(i.dst); out << ", FPUL";
            break;
        case DCIROp::Flds:
            out << "FPUL, "; fr(i.src);
            break;
        case DCIROp::Float:
            fr(i.dst); out << ", FPUL";
            break;
        case DCIROp::Ftrc:
            out << "FPUL, "; fr(i.src);
            break;
        case DCIROp::Fneg:
        case DCIROp::Fabs:
        case DCIROp::Fsqrt:
        case DCIROp::Fsrra:
        case DCIROp::Fldi0:
        case DCIROp::Fldi1:
            fr(i.dst);
            break;
        case DCIROp::Fcnvsd:
            fr(i.dst); out << ", FPUL";
            break;
        case DCIROp::Fcnvds:
            out << "FPUL, "; fr(i.src);
            break;
        case DCIROp::Fipr:
            out << "FV" << static_cast<unsigned>(i.src) << ", FV" << static_cast<unsigned>(i.dst);
            break;
        case DCIROp::Ftrv:
            out << "XMTRX, FV" << static_cast<unsigned>(i.dst);
            break;
        case DCIROp::Fsca:
            fr(i.dst); out << ", FR" << static_cast<unsigned>(i.dst + 1u) << ", FPUL";
            break;
        case DCIROp::Frchg:
        case DCIROp::Fschg:
            break;
        case DCIROp::FmovLoad:
            fr(i.dst); out << ", @"; r(i.src);
            break;
        case DCIROp::FmovLoadPostInc:
            fr(i.dst); out << ", @"; r(i.src); out << "+";
            break;
        case DCIROp::FmovStore:
            out << "@"; r(i.dst); out << ", "; fr(i.src);
            break;
        case DCIROp::FmovStorePreDec:
            out << "@-"; r(i.dst); out << ", "; fr(i.src);
            break;
        case DCIROp::FmovReg:
            fr(i.dst); out << ", "; fr(i.src);
            break;
        case DCIROp::FmovIndexedLoad:
            fr(i.dst); out << ", @(R0,"; r(i.src); out << ")";
            break;
        case DCIROp::FmovIndexedStore:
            out << "@(R0,"; r(i.dst); out << "), "; fr(i.src);
            break;
        case DCIROp::AddImm:
            r(i.dst); out << ", " << i.immediate;
            break;
        case DCIROp::AddReg:
        case DCIROp::Addc:
        case DCIROp::Addv:
        case DCIROp::SubReg:
        case DCIROp::Subc:
        case DCIROp::Subv:
        case DCIROp::Neg:
        case DCIROp::Negc:
        case DCIROp::AndReg:
        case DCIROp::XorReg:
        case DCIROp::OrReg:
        case DCIROp::NotReg:
        case DCIROp::SwapB:
        case DCIROp::SwapW:
        case DCIROp::Xtrct:
        case DCIROp::Shld:
        case DCIROp::Shad:
        case DCIROp::MulL:
        case DCIROp::MuluW:
        case DCIROp::MulsW:
        case DCIROp::DmuluL:
        case DCIROp::DmulsL:
        case DCIROp::ExtuB:
        case DCIROp::ExtuW:
        case DCIROp::ExtsB:
        case DCIROp::ExtsW:
        case DCIROp::CmpEq:
        case DCIROp::CmpStr:
        case DCIROp::CmpHs:
        case DCIROp::CmpGe:
        case DCIROp::CmpHi:
        case DCIROp::CmpGt:
        case DCIROp::TstReg:
        case DCIROp::Div0S:
        case DCIROp::Div1:
            r(i.dst); out << ", "; r(i.src);
            break;
        case DCIROp::AndImm:
        case DCIROp::XorImm:
        case DCIROp::OrImm:
            out << "R0, " << i.immediate;
            break;
        case DCIROp::Shll:
        case DCIROp::Shlr:
        case DCIROp::Shal:
        case DCIROp::Shar:
        case DCIROp::Shll2:
        case DCIROp::Shlr2:
        case DCIROp::Shll8:
        case DCIROp::Shlr8:
        case DCIROp::Shll16:
        case DCIROp::Shlr16:
        case DCIROp::Rotl:
        case DCIROp::Rotr:
        case DCIROp::Rotcl:
        case DCIROp::Rotcr:
            r(i.dst);
            break;
        case DCIROp::CmpEqImm:
            out << "R0, " << i.immediate;
            break;
        case DCIROp::CmpPz:
        case DCIROp::CmpPl:
        case DCIROp::Dt:
        case DCIROp::MovT:
            r(i.dst);
            break;
        case DCIROp::TstImm:
            out << "R0, " << i.immediate;
            break;
        case DCIROp::SaveT:
            out << "T";
            break;
        case DCIROp::TasB:
            out << "@"; r(i.dst);
            break;
        case DCIROp::Pref:
        case DCIROp::MovcaL:
        case DCIROp::Ocbi:
        case DCIROp::Ocbp:
        case DCIROp::Ocbwb:
            out << "@"; r(i.dst);
            break;
        case DCIROp::Trapa:
            out << "#" << i.immediate;
            break;
        case DCIROp::StsPr:
        case DCIROp::StsMach:
        case DCIROp::StsMacl:
        case DCIROp::StsFpul:
        case DCIROp::StsFpscr:
        case DCIROp::StcSr:
        case DCIROp::StcGbr:
        case DCIROp::StcVbr:
        case DCIROp::StcSsr:
        case DCIROp::StcSpc:
        case DCIROp::StcSgr:
        case DCIROp::StcDbr:
        case DCIROp::StcLSr:
        case DCIROp::StcLGbr:
        case DCIROp::StcLVbr:
        case DCIROp::StcLSsr:
        case DCIROp::StcLSpc:
        case DCIROp::StcLSgr:
        case DCIROp::StcLDbr:
        case DCIROp::StsLMach:
        case DCIROp::StsLMacl:
        case DCIROp::StsLFpul:
        case DCIROp::StsLFpscr:
            r(i.dst);
            break;
        case DCIROp::StcBank:
        case DCIROp::StcLBank:
            r(i.dst); out << ", bank" << i.immediate;
            break;
        case DCIROp::LdcBank:
        case DCIROp::LdcLBank:
            r(i.src); out << ", bank" << i.immediate;
            break;
        case DCIROp::LdsPr:
        case DCIROp::LdsMach:
        case DCIROp::LdsMacl:
        case DCIROp::LdsFpul:
        case DCIROp::LdsFpscr:
        case DCIROp::LdcSr:
        case DCIROp::LdcGbr:
        case DCIROp::LdcVbr:
        case DCIROp::LdcSsr:
        case DCIROp::LdcSpc:
        case DCIROp::LdcSgr:
        case DCIROp::LdcDbr:
        case DCIROp::LdcLSr:
        case DCIROp::LdcLGbr:
        case DCIROp::LdcLVbr:
        case DCIROp::LdcLSsr:
        case DCIROp::LdcLSpc:
        case DCIROp::LdcLSgr:
        case DCIROp::LdcLDbr:
        case DCIROp::LdsLMach:
        case DCIROp::LdsLMacl:
        case DCIROp::LdsLFpul:
        case DCIROp::LdsLFpscr:
            r(i.src);
            break;
        case DCIROp::PushPR:
            out << "@-R" << static_cast<unsigned>(i.dst);
            break;
        case DCIROp::PopPR:
            out << "@R" << static_cast<unsigned>(i.src) << "+";
            break;
        case DCIROp::Call:
        case DCIROp::Branch:
        case DCIROp::BranchIfTrue:
        case DCIROp::BranchIfFalse:
        case DCIROp::BranchIfSavedTrue:
        case DCIROp::BranchIfSavedFalse:
            if (i.target) hex32(i.target); else out << "<unresolved>";
            if (!i.symbol.empty()) out << "  ; " << i.symbol;
            else if (!i.section.empty()) out << "  ; " << i.section;
            break;
        case DCIROp::SaveDynamicAbsoluteTarget:
        case DCIROp::SaveDynamicRelativeTarget:
        case DCIROp::DynamicBranch:
        case DCIROp::DynamicCall:
            r(i.src);
            break;
        case DCIROp::RawSH4:
            out << i.text;
            break;
        case DCIROp::Nop:
        case DCIROp::ClearT:
        case DCIROp::SetT:
        case DCIROp::ClearS:
        case DCIROp::SetS:
        case DCIROp::ClearMac:
        case DCIROp::Sleep:
        case DCIROp::LdTlb:
        case DCIROp::Div0U:
        case DCIROp::PrepareRte:
        case DCIROp::Rte:
        case DCIROp::Return:
            break;
    }
    return out.str();
}

} // namespace dcrecomp
