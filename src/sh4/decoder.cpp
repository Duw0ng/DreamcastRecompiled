#include "dcrecomp/sh4_decoder.hpp"

#include <iomanip>
#include <sstream>

namespace dcrecomp::sh4 {
namespace {

std::int32_t sign_extend8(std::uint32_t value) {
    return static_cast<std::int8_t>(value & 0xFF);
}

std::int32_t sign_extend12(std::uint32_t value) {
    value &= 0x0FFF;
    if (value & 0x0800) {
        value |= 0xFFFFF000;
    }
    return static_cast<std::int32_t>(value);
}

std::uint32_t branch_target8(std::uint32_t address, std::uint16_t raw) {
    const auto disp = sign_extend8(raw);
    return static_cast<std::uint32_t>(static_cast<std::int64_t>(address) + 4 +
                                      static_cast<std::int64_t>(disp) * 2);
}

std::uint32_t branch_target12(std::uint32_t address, std::uint16_t raw) {
    const auto disp = sign_extend12(raw);
    return static_cast<std::uint32_t>(static_cast<std::int64_t>(address) + 4 +
                                      static_cast<std::int64_t>(disp) * 2);
}

void set_nm(Instruction& out, std::uint16_t raw) {
    out.rn = static_cast<std::uint8_t>((raw >> 8) & 0xF);
    out.rm = static_cast<std::uint8_t>((raw >> 4) & 0xF);
}

} // namespace

Instruction decode(std::uint16_t raw, std::uint32_t address) {
    Instruction out{};
    out.raw = raw;
    out.address = address;

    // Fixed encodings.
    if (raw == 0x0009) { out.opcode = Opcode::Nop; return out; }
    if (raw == 0x000B) { out.opcode = Opcode::Rts; out.has_delay_slot = true; return out; }
    if (raw == 0x002B) { out.opcode = Opcode::Rte; out.has_delay_slot = true; return out; }
    if (raw == 0x0008) { out.opcode = Opcode::ClrT; return out; }
    if (raw == 0x0018) { out.opcode = Opcode::SetT; return out; }
    if (raw == 0x0048) { out.opcode = Opcode::ClrS; return out; }
    if (raw == 0x0058) { out.opcode = Opcode::SetS; return out; }
    if (raw == 0x0028) { out.opcode = Opcode::ClrMac; return out; }
    if (raw == 0x0019) { out.opcode = Opcode::Div0U; return out; }
    if (raw == 0x001B) { out.opcode = Opcode::Sleep; return out; }
    if (raw == 0x0038) { out.opcode = Opcode::LdTlb; return out; }

    // SH-4 cache/store-queue control.
    if ((raw & 0xF0FF) == 0x0083) { out.opcode = Opcode::Pref;   out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x0093) { out.opcode = Opcode::Ocbi;   out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x00A3) { out.opcode = Opcode::Ocbp;   out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x00B3) { out.opcode = Opcode::Ocbwb;  out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x00C3) { out.opcode = Opcode::MovcaL; out.rn = (raw >> 8) & 0xF; return out; }

    // TRAPA #imm. Exception-vector dispatch is represented explicitly in DCIR.
    if ((raw & 0xFF00) == 0xC300) { out.opcode = Opcode::Trapa; out.immediate = raw & 0xFF; return out; }

    // Control-register stores (STC / STC.L).
    if ((raw & 0xF0FF) == 0x0002) { out.opcode = Opcode::StcSr;  out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x0012) { out.opcode = Opcode::StcGbr; out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x0022) { out.opcode = Opcode::StcVbr; out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x0032) { out.opcode = Opcode::StcSsr; out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x0042) { out.opcode = Opcode::StcSpc; out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x003A) { out.opcode = Opcode::StcSgr; out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x00FA) { out.opcode = Opcode::StcDbr; out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF08F) == 0x0082) { out.opcode = Opcode::StcBank; out.rn = (raw >> 8) & 0xF; out.immediate = (raw >> 4) & 7; return out; }

    if ((raw & 0xF0FF) == 0x4003) { out.opcode = Opcode::StcLSr;  out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4013) { out.opcode = Opcode::StcLGbr; out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4023) { out.opcode = Opcode::StcLVbr; out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4033) { out.opcode = Opcode::StcLSsr; out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4043) { out.opcode = Opcode::StcLSpc; out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4032) { out.opcode = Opcode::StcLSgr; out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x40F2) { out.opcode = Opcode::StcLDbr; out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF08F) == 0x4083) { out.opcode = Opcode::StcLBank; out.rn = (raw >> 8) & 0xF; out.immediate = (raw >> 4) & 7; return out; }

    // Control-register loads (LDC / LDC.L).
    if ((raw & 0xF0FF) == 0x400E) { out.opcode = Opcode::LdcSr;  out.rm = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x401E) { out.opcode = Opcode::LdcGbr; out.rm = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x402E) { out.opcode = Opcode::LdcVbr; out.rm = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x403E) { out.opcode = Opcode::LdcSsr; out.rm = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x404E) { out.opcode = Opcode::LdcSpc; out.rm = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x403A) { out.opcode = Opcode::LdcSgr; out.rm = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x40FA) { out.opcode = Opcode::LdcDbr; out.rm = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF08F) == 0x408E) { out.opcode = Opcode::LdcBank; out.rm = (raw >> 8) & 0xF; out.immediate = (raw >> 4) & 7; return out; }

    if ((raw & 0xF0FF) == 0x4007) { out.opcode = Opcode::LdcLSr;  out.rm = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4017) { out.opcode = Opcode::LdcLGbr; out.rm = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4027) { out.opcode = Opcode::LdcLVbr; out.rm = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4037) { out.opcode = Opcode::LdcLSsr; out.rm = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4047) { out.opcode = Opcode::LdcLSpc; out.rm = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4036) { out.opcode = Opcode::LdcLSgr; out.rm = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x40F6) { out.opcode = Opcode::LdcLDbr; out.rm = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF08F) == 0x4087) { out.opcode = Opcode::LdcLBank; out.rm = (raw >> 8) & 0xF; out.immediate = (raw >> 4) & 7; return out; }

    // Stack forms of STS/LDS for special registers.
    if ((raw & 0xF0FF) == 0x4002) { out.opcode = Opcode::StsLMach;  out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4012) { out.opcode = Opcode::StsLMacl;  out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4052) { out.opcode = Opcode::StsLFpul;  out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4062) { out.opcode = Opcode::StsLFpscr; out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4006) { out.opcode = Opcode::LdsLMach;  out.rm = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4016) { out.opcode = Opcode::LdsLMacl;  out.rm = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4056) { out.opcode = Opcode::LdsLFpul;  out.rm = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4066) { out.opcode = Opcode::LdsLFpscr; out.rm = (raw >> 8) & 0xF; return out; }

    // MOV #imm,Rn
    if ((raw & 0xF000) == 0xE000) {
        out.opcode = Opcode::MovImm;
        out.rn = static_cast<std::uint8_t>((raw >> 8) & 0xF);
        out.immediate = sign_extend8(raw);
        return out;
    }

    // ADD #imm,Rn
    if ((raw & 0xF000) == 0x7000) {
        out.opcode = Opcode::AddImm;
        out.rn = static_cast<std::uint8_t>((raw >> 8) & 0xF);
        out.immediate = sign_extend8(raw);
        return out;
    }

    // BRA / BSR disp12
    if ((raw & 0xF000) == 0xA000) {
        out.opcode = Opcode::Bra;
        out.immediate = sign_extend12(raw);
        out.target = branch_target12(address, raw);
        out.has_delay_slot = true;
        return out;
    }
    if ((raw & 0xF000) == 0xB000) {
        out.opcode = Opcode::Bsr;
        out.immediate = sign_extend12(raw);
        out.target = branch_target12(address, raw);
        out.has_delay_slot = true;
        return out;
    }

    // BT/BF and delayed variants.
    if ((raw & 0xFF00) == 0x8900) {
        out.opcode = Opcode::Bt;
        out.immediate = sign_extend8(raw);
        out.target = branch_target8(address, raw);
        return out;
    }
    if ((raw & 0xFF00) == 0x8B00) {
        out.opcode = Opcode::Bf;
        out.immediate = sign_extend8(raw);
        out.target = branch_target8(address, raw);
        return out;
    }
    if ((raw & 0xFF00) == 0x8D00) {
        out.opcode = Opcode::BtS;
        out.immediate = sign_extend8(raw);
        out.target = branch_target8(address, raw);
        out.has_delay_slot = true;
        return out;
    }
    if ((raw & 0xFF00) == 0x8F00) {
        out.opcode = Opcode::BfS;
        out.immediate = sign_extend8(raw);
        out.target = branch_target8(address, raw);
        out.has_delay_slot = true;
        return out;
    }

    // PC-relative loads and MOVA.
    if ((raw & 0xF000) == 0x9000) {
        out.opcode = Opcode::MovWPcRel;
        out.rn = static_cast<std::uint8_t>((raw >> 8) & 0xF);
        out.immediate = raw & 0xFF;
        out.effective_address = address + 4 + static_cast<std::uint32_t>(out.immediate * 2);
        return out;
    }
    if ((raw & 0xF000) == 0xD000) {
        out.opcode = Opcode::MovLPcRel;
        out.rn = static_cast<std::uint8_t>((raw >> 8) & 0xF);
        out.immediate = raw & 0xFF;
        out.effective_address = ((address + 4) & ~3u) + static_cast<std::uint32_t>(out.immediate * 4);
        return out;
    }
    if ((raw & 0xFF00) == 0xC700) {
        out.opcode = Opcode::Mova;
        out.immediate = raw & 0xFF;
        out.effective_address = ((address + 4) & ~3u) + static_cast<std::uint32_t>(out.immediate * 4);
        return out;
    }

    // Immediate logical ops on R0.
    if ((raw & 0xFF00) == 0xC800) { out.opcode = Opcode::TstImm; out.immediate = raw & 0xFF; return out; }
    if ((raw & 0xFF00) == 0xC900) { out.opcode = Opcode::AndImm; out.immediate = raw & 0xFF; return out; }
    if ((raw & 0xFF00) == 0xCA00) { out.opcode = Opcode::XorImm; out.immediate = raw & 0xFF; return out; }
    if ((raw & 0xFF00) == 0xCB00) { out.opcode = Opcode::OrImm;  out.immediate = raw & 0xFF; return out; }

    // CMP/EQ #imm,R0
    if ((raw & 0xFF00) == 0x8800) {
        out.opcode = Opcode::CmpEqImm;
        out.immediate = sign_extend8(raw);
        return out;
    }

    // Register-register data transfers.
    if ((raw & 0xF00F) == 0x6003) { out.opcode = Opcode::MovReg; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x2000) { out.opcode = Opcode::MovBStore; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x2001) { out.opcode = Opcode::MovWStore; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x2002) { out.opcode = Opcode::MovLStore; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x6000) { out.opcode = Opcode::MovBLoad; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x6001) { out.opcode = Opcode::MovWLoad; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x6002) { out.opcode = Opcode::MovLLoad; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x2004) { out.opcode = Opcode::MovBPredec; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x2005) { out.opcode = Opcode::MovWPredec; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x2006) { out.opcode = Opcode::MovLPredec; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x6004) { out.opcode = Opcode::MovBPostinc; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x6005) { out.opcode = Opcode::MovWPostinc; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x6006) { out.opcode = Opcode::MovLPostinc; set_nm(out, raw); return out; }

    // Byte/word displacement forms using R0. These encodings dedicate the
    // second nibble to the operation and scale the 4-bit displacement by width.
    //   1000 0000 mmmm dddd  mov.b R0,@(disp,Rm)
    //   1000 0001 mmmm dddd  mov.w R0,@(disp,Rm)
    //   1000 0100 mmmm dddd  mov.b @(disp,Rm),R0
    //   1000 0101 mmmm dddd  mov.w @(disp,Rm),R0
    if ((raw & 0xFF00) == 0x8000) {
        out.opcode = Opcode::MovBDispStore;
        out.rn = static_cast<std::uint8_t>((raw >> 4) & 0xF); // base register
        out.rm = 0; // source R0
        out.immediate = raw & 0xF;
        return out;
    }
    if ((raw & 0xFF00) == 0x8100) {
        out.opcode = Opcode::MovWDispStore;
        out.rn = static_cast<std::uint8_t>((raw >> 4) & 0xF); // base register
        out.rm = 0;
        out.immediate = (raw & 0xF) * 2;
        return out;
    }
    if ((raw & 0xFF00) == 0x8400) {
        out.opcode = Opcode::MovBDispLoad;
        out.rm = static_cast<std::uint8_t>((raw >> 4) & 0xF); // base register
        out.rn = 0; // destination R0
        out.immediate = raw & 0xF;
        return out;
    }
    if ((raw & 0xFF00) == 0x8500) {
        out.opcode = Opcode::MovWDispLoad;
        out.rm = static_cast<std::uint8_t>((raw >> 4) & 0xF); // base register
        out.rn = 0;
        out.immediate = (raw & 0xF) * 2;
        return out;
    }

    // Indexed memory forms using R0 as the index register.
    if ((raw & 0xF00F) == 0x0004) { out.opcode = Opcode::MovBIndexedStore; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x0005) { out.opcode = Opcode::MovWIndexedStore; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x0006) { out.opcode = Opcode::MovLIndexedStore; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x000C) { out.opcode = Opcode::MovBIndexedLoad;  set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x000D) { out.opcode = Opcode::MovWIndexedLoad;  set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x000E) { out.opcode = Opcode::MovLIndexedLoad;  set_nm(out, raw); return out; }

    // MOV.L Rm,@(disp,Rn) / MOV.L @(disp,Rm),Rn (disp4, scaled by 4).
    if ((raw & 0xF000) == 0x1000) {
        out.opcode = Opcode::MovLDispStore;
        set_nm(out, raw);
        out.immediate = raw & 0xF;
        return out;
    }
    if ((raw & 0xF000) == 0x5000) {
        out.opcode = Opcode::MovLDispLoad;
        set_nm(out, raw);
        out.immediate = raw & 0xF;
        return out;
    }

    // Integer arithmetic / compare / logical.
    if ((raw & 0xF00F) == 0x300C) { out.opcode = Opcode::AddReg; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x300E) { out.opcode = Opcode::Addc; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x300F) { out.opcode = Opcode::Addv; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x3008) { out.opcode = Opcode::SubReg; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x300A) { out.opcode = Opcode::Subc; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x300B) { out.opcode = Opcode::Subv; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x600B) { out.opcode = Opcode::Neg; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x600A) { out.opcode = Opcode::Negc; set_nm(out, raw); return out; }

    // Multiply / byte-order helpers.
    if ((raw & 0xF00F) == 0x0007) { out.opcode = Opcode::MulL;   set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x200E) { out.opcode = Opcode::MuluW;  set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x200F) { out.opcode = Opcode::MulsW;  set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x3005) { out.opcode = Opcode::DmuluL; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x300D) { out.opcode = Opcode::DmulsL; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x6008) { out.opcode = Opcode::SwapB;  set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x6009) { out.opcode = Opcode::SwapW;  set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x200D) { out.opcode = Opcode::Xtrct;  set_nm(out, raw); return out; }

    if ((raw & 0xF00F) == 0x3000) { out.opcode = Opcode::CmpEq; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x3002) { out.opcode = Opcode::CmpHs; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x3003) { out.opcode = Opcode::CmpGe; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x3006) { out.opcode = Opcode::CmpHi; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x3007) { out.opcode = Opcode::CmpGt; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x200C) { out.opcode = Opcode::CmpStr; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x2007) { out.opcode = Opcode::Div0S; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x3004) { out.opcode = Opcode::Div1; set_nm(out, raw); return out; }

    if ((raw & 0xF00F) == 0x2008) { out.opcode = Opcode::TstReg; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x2009) { out.opcode = Opcode::AndReg; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x200A) { out.opcode = Opcode::XorReg; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x200B) { out.opcode = Opcode::OrReg;  set_nm(out, raw); return out; }
    // NOT Rm,Rn: bitwise complement. GCC emits this for C's ~ operator.
    if ((raw & 0xF00F) == 0x6007) { out.opcode = Opcode::Not; set_nm(out, raw); return out; }

    // Extension ops.
    if ((raw & 0xF00F) == 0x600C) { out.opcode = Opcode::ExtuB; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x600D) { out.opcode = Opcode::ExtuW; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x600E) { out.opcode = Opcode::ExtsB; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x600F) { out.opcode = Opcode::ExtsW; set_nm(out, raw); return out; }

    // Single-register compare/loop/shift/rotate.
    if ((raw & 0xF0FF) == 0x4011) { out.opcode = Opcode::CmpPz; out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4015) { out.opcode = Opcode::CmpPl; out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4010) { out.opcode = Opcode::Dt;    out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4000) { out.opcode = Opcode::Shll;  out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4001) { out.opcode = Opcode::Shlr;  out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4020) { out.opcode = Opcode::Shal;  out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4021) { out.opcode = Opcode::Shar;  out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4008) { out.opcode = Opcode::Shll2; out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4009) { out.opcode = Opcode::Shlr2; out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4018) { out.opcode = Opcode::Shll8; out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4019) { out.opcode = Opcode::Shlr8; out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4028) { out.opcode = Opcode::Shll16; out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4029) { out.opcode = Opcode::Shlr16; out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4004) { out.opcode = Opcode::Rotl;  out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4005) { out.opcode = Opcode::Rotr;  out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4024) { out.opcode = Opcode::Rotcl; out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4025) { out.opcode = Opcode::Rotcr; out.rn = (raw >> 8) & 0xF; return out; }

    // Dynamic shifts. The sign of Rm chooses left vs right.
    if ((raw & 0xF00F) == 0x400D) { out.opcode = Opcode::Shld; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0x400C) { out.opcode = Opcode::Shad; set_nm(out, raw); return out; }

    // SH-4 FPU arithmetic / compare. FPSCR.PR selects FR (single) or DR
    // (double) semantics at runtime. FMAC is architecturally PR=0 only.
    if ((raw & 0xF00F) == 0xF000) { out.opcode = Opcode::Fadd; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0xF001) { out.opcode = Opcode::Fsub; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0xF002) { out.opcode = Opcode::Fmul; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0xF003) { out.opcode = Opcode::Fdiv; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0xF004) { out.opcode = Opcode::FcmpEq; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0xF005) { out.opcode = Opcode::FcmpGt; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0xF00E) { out.opcode = Opcode::Fmac; set_nm(out, raw); return out; }

    // SH-4 FPU unary/conversion/vector operations (FnmD family).
    // Exact state-changing encodings first.
    if (raw == 0xFBFD) { out.opcode = Opcode::Frchg; return out; }
    if (raw == 0xF3FD) { out.opcode = Opcode::Fschg; return out; }

    // FSCA FPUL,DRn: nnn0 in the register nibble.
    if ((raw & 0xF1FF) == 0xF0FD) {
        out.opcode = Opcode::Fsca;
        out.rn = static_cast<std::uint8_t>((raw >> 8) & 0xEu);
        return out;
    }
    // FTRV XMTRX,FVn: second nibble is nn01, vector base = nn * 4.
    if ((raw & 0xF3FF) == 0xF1FD) {
        out.opcode = Opcode::Ftrv;
        const std::uint8_t packed = static_cast<std::uint8_t>((raw >> 8) & 0xFu);
        out.rn = static_cast<std::uint8_t>(((packed & 0xCu) >> 2) * 4u);
        return out;
    }
    // FIPR FVm,FVn: second nibble packs nnmm, each selecting a 4-FR vector.
    if ((raw & 0xF0FF) == 0xF0ED) {
        out.opcode = Opcode::Fipr;
        const std::uint8_t packed = static_cast<std::uint8_t>((raw >> 8) & 0xFu);
        out.rn = static_cast<std::uint8_t>(((packed & 0xCu) >> 2) * 4u);
        out.rm = static_cast<std::uint8_t>((packed & 0x3u) * 4u);
        return out;
    }

    if ((raw & 0xF00F) == 0xF00D) {
        out.rn = static_cast<std::uint8_t>((raw >> 8) & 0xFu);
        switch ((raw >> 4) & 0xFu) {
            case 0x0: out.opcode = Opcode::Fsts; return out;
            case 0x1: out.opcode = Opcode::Flds; return out;
            case 0x2: out.opcode = Opcode::Float; return out;
            case 0x3: out.opcode = Opcode::Ftrc; return out;
            case 0x4: out.opcode = Opcode::Fneg; return out;
            case 0x5: out.opcode = Opcode::Fabs; return out;
            case 0x6: out.opcode = Opcode::Fsqrt; return out;
            case 0x7: out.opcode = Opcode::Fsrra; return out;
            case 0x8: out.opcode = Opcode::Fldi0; return out;
            case 0x9: out.opcode = Opcode::Fldi1; return out;
            case 0xA:
                if ((out.rn & 1u) == 0u) { out.opcode = Opcode::Fcnvsd; return out; }
                break;
            case 0xB:
                if ((out.rn & 1u) == 0u) { out.opcode = Opcode::Fcnvds; return out; }
                break;
            default: break;
        }
    }

    // SH-4 FPU moves. These opcodes share the FnmX layout. The transfer
    // width is controlled architecturally by FPSCR.SZ at runtime; the decoder
    // only identifies the encoded addressing form.
    if ((raw & 0xF00F) == 0xF008) { out.opcode = Opcode::FmovLoad; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0xF009) { out.opcode = Opcode::FmovLoadPostInc; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0xF00A) { out.opcode = Opcode::FmovStore; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0xF00B) { out.opcode = Opcode::FmovStorePreDec; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0xF00C) { out.opcode = Opcode::FmovReg; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0xF006) { out.opcode = Opcode::FmovIndexedLoad; set_nm(out, raw); return out; }
    if ((raw & 0xF00F) == 0xF007) { out.opcode = Opcode::FmovIndexedStore; set_nm(out, raw); return out; }

    // Atomic test-and-set byte.
    if ((raw & 0xF0FF) == 0x401B) { out.opcode = Opcode::TasB; out.rn = (raw >> 8) & 0xF; return out; }

    // Register-relative branches. Target = address + 4 + Rn.
    if ((raw & 0xF0FF) == 0x0023) {
        out.opcode = Opcode::Braf;
        out.rn = static_cast<std::uint8_t>((raw >> 8) & 0xF);
        out.has_delay_slot = true;
        return out;
    }
    if ((raw & 0xF0FF) == 0x0003) {
        out.opcode = Opcode::Bsrf;
        out.rn = static_cast<std::uint8_t>((raw >> 8) & 0xF);
        out.has_delay_slot = true;
        return out;
    }

    // Indirect calls/jumps.
    if ((raw & 0xF0FF) == 0x402B) {
        out.opcode = Opcode::Jmp;
        out.rm = static_cast<std::uint8_t>((raw >> 8) & 0xF);
        out.has_delay_slot = true;
        return out;
    }
    if ((raw & 0xF0FF) == 0x400B) {
        out.opcode = Opcode::Jsr;
        out.rm = static_cast<std::uint8_t>((raw >> 8) & 0xF);
        out.has_delay_slot = true;
        return out;
    }

    // Special-register transfers used heavily by GCC/KallistiOS.
    if ((raw & 0xF0FF) == 0x000A) { out.opcode = Opcode::StsMach;  out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x001A) { out.opcode = Opcode::StsMacl;  out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x005A) { out.opcode = Opcode::StsFpul;  out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x006A) { out.opcode = Opcode::StsFpscr; out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x400A) { out.opcode = Opcode::LdsMach;  out.rm = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x401A) { out.opcode = Opcode::LdsMacl;  out.rm = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x405A) { out.opcode = Opcode::LdsFpul;  out.rm = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x406A) { out.opcode = Opcode::LdsFpscr; out.rm = (raw >> 8) & 0xF; return out; }

    // PR save/restore forms used in normal GCC function prologues.
    if ((raw & 0xF0FF) == 0x002A) { out.opcode = Opcode::StsPr;  out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4022) { out.opcode = Opcode::StsLPr; out.rn = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x402A) { out.opcode = Opcode::LdsPr;  out.rm = (raw >> 8) & 0xF; return out; }
    if ((raw & 0xF0FF) == 0x4026) { out.opcode = Opcode::LdsLPr; out.rm = (raw >> 8) & 0xF; return out; }

    // MOVT Rn.
    if ((raw & 0xF0FF) == 0x0029) {
        out.opcode = Opcode::Movt;
        out.rn = static_cast<std::uint8_t>((raw >> 8) & 0xF);
        return out;
    }

    return out;
}

bool is_known(const Instruction& instruction) {
    return instruction.opcode != Opcode::Unknown;
}

std::string to_string(const Instruction& i) {
    std::ostringstream ss;
    const auto rn = static_cast<int>(i.rn);
    const auto rm = static_cast<int>(i.rm);

    auto target = [&]() {
        ss << "0x" << std::hex << std::uppercase << i.target;
    };
    auto ea = [&]() {
        ss << "0x" << std::hex << std::uppercase << i.effective_address;
    };

    switch (i.opcode) {
        case Opcode::Nop: ss << "nop"; break;
        case Opcode::Rts: ss << "rts"; break;
        case Opcode::Rte: ss << "rte"; break;
        case Opcode::Bra: ss << "bra "; target(); break;
        case Opcode::Bsr: ss << "bsr "; target(); break;
        case Opcode::Bt:  ss << "bt "; target(); break;
        case Opcode::Bf:  ss << "bf "; target(); break;
        case Opcode::BtS: ss << "bt/s "; target(); break;
        case Opcode::BfS: ss << "bf/s "; target(); break;
        case Opcode::Braf: ss << "braf r" << rn; break;
        case Opcode::Bsrf: ss << "bsrf r" << rn; break;
        case Opcode::Jmp: ss << "jmp @r" << rm; break;
        case Opcode::Jsr: ss << "jsr @r" << rm; break;
        case Opcode::ClrT: ss << "clrt"; break;
        case Opcode::SetT: ss << "sett"; break;
        case Opcode::ClrS: ss << "clrs"; break;
        case Opcode::SetS: ss << "sets"; break;
        case Opcode::ClrMac: ss << "clrmac"; break;
        case Opcode::Sleep: ss << "sleep"; break;
        case Opcode::LdTlb: ss << "ldtlb"; break;
        case Opcode::Trapa: ss << "trapa #" << i.immediate; break;
        case Opcode::Pref: ss << "pref @r" << rn; break;
        case Opcode::MovcaL: ss << "movca.l r0,@r" << rn; break;
        case Opcode::Ocbi: ss << "ocbi @r" << rn; break;
        case Opcode::Ocbp: ss << "ocbp @r" << rn; break;
        case Opcode::Ocbwb: ss << "ocbwb @r" << rn; break;

        case Opcode::MovImm: ss << "mov #" << i.immediate << ",r" << rn; break;
        case Opcode::MovReg: ss << "mov r" << rm << ",r" << rn; break;
        case Opcode::AddImm: ss << "add #" << i.immediate << ",r" << rn; break;
        case Opcode::AddReg: ss << "add r" << rm << ",r" << rn; break;
        case Opcode::Addc: ss << "addc r" << rm << ",r" << rn; break;
        case Opcode::Addv: ss << "addv r" << rm << ",r" << rn; break;
        case Opcode::SubReg: ss << "sub r" << rm << ",r" << rn; break;
        case Opcode::Subc: ss << "subc r" << rm << ",r" << rn; break;
        case Opcode::Subv: ss << "subv r" << rm << ",r" << rn; break;
        case Opcode::Neg: ss << "neg r" << rm << ",r" << rn; break;
        case Opcode::Negc: ss << "negc r" << rm << ",r" << rn; break;
        case Opcode::CmpEqImm: ss << "cmp/eq #" << i.immediate << ",r0"; break;
        case Opcode::CmpEq: ss << "cmp/eq r" << rm << ",r" << rn; break;
        case Opcode::CmpStr: ss << "cmp/str r" << rm << ",r" << rn; break;
        case Opcode::CmpHs: ss << "cmp/hs r" << rm << ",r" << rn; break;
        case Opcode::CmpGe: ss << "cmp/ge r" << rm << ",r" << rn; break;
        case Opcode::CmpHi: ss << "cmp/hi r" << rm << ",r" << rn; break;
        case Opcode::CmpGt: ss << "cmp/gt r" << rm << ",r" << rn; break;
        case Opcode::CmpPz: ss << "cmp/pz r" << rn; break;
        case Opcode::CmpPl: ss << "cmp/pl r" << rn; break;
        case Opcode::TstReg: ss << "tst r" << rm << ",r" << rn; break;
        case Opcode::AndReg: ss << "and r" << rm << ",r" << rn; break;
        case Opcode::XorReg: ss << "xor r" << rm << ",r" << rn; break;
        case Opcode::OrReg: ss << "or r" << rm << ",r" << rn; break;
        case Opcode::Not: ss << "not r" << rm << ",r" << rn; break;
        case Opcode::TstImm: ss << "tst #" << i.immediate << ",r0"; break;
        case Opcode::AndImm: ss << "and #" << i.immediate << ",r0"; break;
        case Opcode::XorImm: ss << "xor #" << i.immediate << ",r0"; break;
        case Opcode::OrImm: ss << "or #" << i.immediate << ",r0"; break;
        case Opcode::Dt: ss << "dt r" << rn; break;
        case Opcode::Div0U: ss << "div0u"; break;
        case Opcode::Div0S: ss << "div0s r" << rm << ",r" << rn; break;
        case Opcode::Div1: ss << "div1 r" << rm << ",r" << rn; break;
        case Opcode::MulL: ss << "mul.l r" << rm << ",r" << rn; break;
        case Opcode::MuluW: ss << "mulu.w r" << rm << ",r" << rn; break;
        case Opcode::MulsW: ss << "muls.w r" << rm << ",r" << rn; break;
        case Opcode::DmuluL: ss << "dmulu.l r" << rm << ",r" << rn; break;
        case Opcode::DmulsL: ss << "dmuls.l r" << rm << ",r" << rn; break;
        case Opcode::SwapB: ss << "swap.b r" << rm << ",r" << rn; break;
        case Opcode::SwapW: ss << "swap.w r" << rm << ",r" << rn; break;
        case Opcode::Xtrct: ss << "xtrct r" << rm << ",r" << rn; break;

        case Opcode::Shll: ss << "shll r" << rn; break;
        case Opcode::Shlr: ss << "shlr r" << rn; break;
        case Opcode::Shal: ss << "shal r" << rn; break;
        case Opcode::Shar: ss << "shar r" << rn; break;
        case Opcode::Shll2: ss << "shll2 r" << rn; break;
        case Opcode::Shlr2: ss << "shlr2 r" << rn; break;
        case Opcode::Shll8: ss << "shll8 r" << rn; break;
        case Opcode::Shlr8: ss << "shlr8 r" << rn; break;
        case Opcode::Shll16: ss << "shll16 r" << rn; break;
        case Opcode::Shlr16: ss << "shlr16 r" << rn; break;
        case Opcode::Rotl: ss << "rotl r" << rn; break;
        case Opcode::Rotr: ss << "rotr r" << rn; break;
        case Opcode::Rotcl: ss << "rotcl r" << rn; break;
        case Opcode::Rotcr: ss << "rotcr r" << rn; break;
        case Opcode::Shld: ss << "shld r" << rm << ",r" << rn; break;
        case Opcode::Shad: ss << "shad r" << rm << ",r" << rn; break;
        case Opcode::ExtuB: ss << "extu.b r" << rm << ",r" << rn; break;
        case Opcode::ExtuW: ss << "extu.w r" << rm << ",r" << rn; break;
        case Opcode::ExtsB: ss << "exts.b r" << rm << ",r" << rn; break;
        case Opcode::ExtsW: ss << "exts.w r" << rm << ",r" << rn; break;

        case Opcode::MovBStore: ss << "mov.b r" << rm << ",@r" << rn; break;
        case Opcode::MovWStore: ss << "mov.w r" << rm << ",@r" << rn; break;
        case Opcode::MovLStore: ss << "mov.l r" << rm << ",@r" << rn; break;
        case Opcode::MovBLoad: ss << "mov.b @r" << rm << ",r" << rn; break;
        case Opcode::MovWLoad: ss << "mov.w @r" << rm << ",r" << rn; break;
        case Opcode::MovLLoad: ss << "mov.l @r" << rm << ",r" << rn; break;
        case Opcode::MovBPredec: ss << "mov.b r" << rm << ",@-r" << rn; break;
        case Opcode::MovWPredec: ss << "mov.w r" << rm << ",@-r" << rn; break;
        case Opcode::MovLPredec: ss << "mov.l r" << rm << ",@-r" << rn; break;
        case Opcode::MovBPostinc: ss << "mov.b @r" << rm << "+,r" << rn; break;
        case Opcode::MovWPostinc: ss << "mov.w @r" << rm << "+,r" << rn; break;
        case Opcode::MovLPostinc: ss << "mov.l @r" << rm << "+,r" << rn; break;
        case Opcode::MovBDispStore: ss << "mov.b r0,@(" << i.immediate << ",r" << rn << ")"; break;
        case Opcode::MovWDispStore: ss << "mov.w r0,@(" << i.immediate << ",r" << rn << ")"; break;
        case Opcode::MovBDispLoad: ss << "mov.b @(" << i.immediate << ",r" << rm << "),r0"; break;
        case Opcode::MovWDispLoad: ss << "mov.w @(" << i.immediate << ",r" << rm << "),r0"; break;
        case Opcode::MovLDispStore: ss << "mov.l r" << rm << ",@(" << (i.immediate * 4) << ",r" << rn << ")"; break;
        case Opcode::MovLDispLoad: ss << "mov.l @(" << (i.immediate * 4) << ",r" << rm << "),r" << rn; break;
        case Opcode::MovBIndexedStore: ss << "mov.b r" << rm << ",@(r0,r" << rn << ")"; break;
        case Opcode::MovWIndexedStore: ss << "mov.w r" << rm << ",@(r0,r" << rn << ")"; break;
        case Opcode::MovLIndexedStore: ss << "mov.l r" << rm << ",@(r0,r" << rn << ")"; break;
        case Opcode::MovBIndexedLoad: ss << "mov.b @(r0,r" << rm << "),r" << rn; break;
        case Opcode::MovWIndexedLoad: ss << "mov.w @(r0,r" << rm << "),r" << rn; break;
        case Opcode::MovLIndexedLoad: ss << "mov.l @(r0,r" << rm << "),r" << rn; break;
        case Opcode::MovWPcRel: ss << "mov.w @(" << (i.immediate * 2) << ",pc),r" << rn << "  ; "; ea(); break;
        case Opcode::MovLPcRel: ss << "mov.l @(" << (i.immediate * 4) << ",pc),r" << rn << "  ; "; ea(); break;
        case Opcode::Mova: ss << "mova @(" << (i.immediate * 4) << ",pc),r0  ; "; ea(); break;
        case Opcode::Movt: ss << "movt r" << rn; break;
        case Opcode::Fadd: ss << "fadd fr" << rm << ",fr" << rn; break;
        case Opcode::Fsub: ss << "fsub fr" << rm << ",fr" << rn; break;
        case Opcode::Fmul: ss << "fmul fr" << rm << ",fr" << rn; break;
        case Opcode::Fdiv: ss << "fdiv fr" << rm << ",fr" << rn; break;
        case Opcode::FcmpEq: ss << "fcmp/eq fr" << rm << ",fr" << rn; break;
        case Opcode::FcmpGt: ss << "fcmp/gt fr" << rm << ",fr" << rn; break;
        case Opcode::Fmac: ss << "fmac fr0,fr" << rm << ",fr" << rn; break;
        case Opcode::Fsts: ss << "fsts fpul,fr" << rn; break;
        case Opcode::Flds: ss << "flds fr" << rn << ",fpul"; break;
        case Opcode::Float: ss << "float fpul,fr" << rn; break;
        case Opcode::Ftrc: ss << "ftrc fr" << rn << ",fpul"; break;
        case Opcode::Fneg: ss << "fneg fr" << rn; break;
        case Opcode::Fabs: ss << "fabs fr" << rn; break;
        case Opcode::Fsqrt: ss << "fsqrt fr" << rn; break;
        case Opcode::Fsrra: ss << "fsrra fr" << rn; break;
        case Opcode::Fldi0: ss << "fldi0 fr" << rn; break;
        case Opcode::Fldi1: ss << "fldi1 fr" << rn; break;
        case Opcode::Fcnvsd: ss << "fcnvsd fpul,dr" << rn; break;
        case Opcode::Fcnvds: ss << "fcnvds dr" << rn << ",fpul"; break;
        case Opcode::Fipr: ss << "fipr fv" << rm << ",fv" << rn; break;
        case Opcode::Ftrv: ss << "ftrv xmtrx,fv" << rn; break;
        case Opcode::Fsca: ss << "fsca fpul,dr" << rn; break;
        case Opcode::Frchg: ss << "frchg"; break;
        case Opcode::Fschg: ss << "fschg"; break;
        case Opcode::FmovLoad: ss << "fmov.s @r" << rm << ",fr" << rn; break;
        case Opcode::FmovLoadPostInc: ss << "fmov.s @r" << rm << "+,fr" << rn; break;
        case Opcode::FmovStore: ss << "fmov.s fr" << rm << ",@r" << rn; break;
        case Opcode::FmovStorePreDec: ss << "fmov.s fr" << rm << ",@-r" << rn; break;
        case Opcode::FmovReg: ss << "fmov fr" << rm << ",fr" << rn; break;
        case Opcode::FmovIndexedLoad: ss << "fmov.s @(r0,r" << rm << "),fr" << rn; break;
        case Opcode::FmovIndexedStore: ss << "fmov.s fr" << rm << ",@(r0,r" << rn << ")"; break;
        case Opcode::TasB: ss << "tas.b @r" << rn; break;

        case Opcode::StsMach: ss << "sts mach,r" << rn; break;
        case Opcode::StsMacl: ss << "sts macl,r" << rn; break;
        case Opcode::StsFpul: ss << "sts fpul,r" << rn; break;
        case Opcode::StsFpscr: ss << "sts fpscr,r" << rn; break;
        case Opcode::LdsMach: ss << "lds r" << rm << ",mach"; break;
        case Opcode::LdsMacl: ss << "lds r" << rm << ",macl"; break;
        case Opcode::LdsFpul: ss << "lds r" << rm << ",fpul"; break;
        case Opcode::LdsFpscr: ss << "lds r" << rm << ",fpscr"; break;
        case Opcode::StcSr: ss << "stc sr,r" << rn; break;
        case Opcode::StcGbr: ss << "stc gbr,r" << rn; break;
        case Opcode::StcVbr: ss << "stc vbr,r" << rn; break;
        case Opcode::StcSsr: ss << "stc ssr,r" << rn; break;
        case Opcode::StcSpc: ss << "stc spc,r" << rn; break;
        case Opcode::StcSgr: ss << "stc sgr,r" << rn; break;
        case Opcode::StcDbr: ss << "stc dbr,r" << rn; break;
        case Opcode::StcBank: ss << "stc r" << i.immediate << "_bank,r" << rn; break;
        case Opcode::StcLSr: ss << "stc.l sr,@-r" << rn; break;
        case Opcode::StcLGbr: ss << "stc.l gbr,@-r" << rn; break;
        case Opcode::StcLVbr: ss << "stc.l vbr,@-r" << rn; break;
        case Opcode::StcLSsr: ss << "stc.l ssr,@-r" << rn; break;
        case Opcode::StcLSpc: ss << "stc.l spc,@-r" << rn; break;
        case Opcode::StcLSgr: ss << "stc.l sgr,@-r" << rn; break;
        case Opcode::StcLDbr: ss << "stc.l dbr,@-r" << rn; break;
        case Opcode::StcLBank: ss << "stc.l r" << i.immediate << "_bank,@-r" << rn; break;
        case Opcode::LdcSr: ss << "ldc r" << rm << ",sr"; break;
        case Opcode::LdcGbr: ss << "ldc r" << rm << ",gbr"; break;
        case Opcode::LdcVbr: ss << "ldc r" << rm << ",vbr"; break;
        case Opcode::LdcSsr: ss << "ldc r" << rm << ",ssr"; break;
        case Opcode::LdcSpc: ss << "ldc r" << rm << ",spc"; break;
        case Opcode::LdcSgr: ss << "ldc r" << rm << ",sgr"; break;
        case Opcode::LdcDbr: ss << "ldc r" << rm << ",dbr"; break;
        case Opcode::LdcBank: ss << "ldc r" << rm << ",r" << i.immediate << "_bank"; break;
        case Opcode::LdcLSr: ss << "ldc.l @r" << rm << "+,sr"; break;
        case Opcode::LdcLGbr: ss << "ldc.l @r" << rm << "+,gbr"; break;
        case Opcode::LdcLVbr: ss << "ldc.l @r" << rm << "+,vbr"; break;
        case Opcode::LdcLSsr: ss << "ldc.l @r" << rm << "+,ssr"; break;
        case Opcode::LdcLSpc: ss << "ldc.l @r" << rm << "+,spc"; break;
        case Opcode::LdcLSgr: ss << "ldc.l @r" << rm << "+,sgr"; break;
        case Opcode::LdcLDbr: ss << "ldc.l @r" << rm << "+,dbr"; break;
        case Opcode::LdcLBank: ss << "ldc.l @r" << rm << "+,r" << i.immediate << "_bank"; break;
        case Opcode::StsLMach: ss << "sts.l mach,@-r" << rn; break;
        case Opcode::StsLMacl: ss << "sts.l macl,@-r" << rn; break;
        case Opcode::StsLFpul: ss << "sts.l fpul,@-r" << rn; break;
        case Opcode::StsLFpscr: ss << "sts.l fpscr,@-r" << rn; break;
        case Opcode::LdsLMach: ss << "lds.l @r" << rm << "+,mach"; break;
        case Opcode::LdsLMacl: ss << "lds.l @r" << rm << "+,macl"; break;
        case Opcode::LdsLFpul: ss << "lds.l @r" << rm << "+,fpul"; break;
        case Opcode::LdsLFpscr: ss << "lds.l @r" << rm << "+,fpscr"; break;

        case Opcode::StsPr: ss << "sts pr,r" << rn; break;
        case Opcode::StsLPr: ss << "sts.l pr,@-r" << rn; break;
        case Opcode::LdsPr: ss << "lds r" << rm << ",pr"; break;
        case Opcode::LdsLPr: ss << "lds.l @r" << rm << "+,pr"; break;

        case Opcode::Unknown:
            ss << ".word 0x" << std::hex << std::uppercase
               << std::setw(4) << std::setfill('0') << i.raw;
            break;
    }
    return ss.str();
}

} // namespace dcrecomp::sh4
