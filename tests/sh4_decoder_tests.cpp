#include "dcrecomp/sh4_decoder.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

using dcrecomp::sh4::Opcode;

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

void require_text(std::uint16_t raw, std::uint32_t pc, const std::string& expected) {
    const auto i = dcrecomp::sh4::decode(raw, pc);
    const auto text = dcrecomp::sh4::to_string(i);
    if (text != expected) {
        std::cerr << "FAIL text: raw=0x" << std::hex << raw
                  << " expected='" << expected << "' got='" << text << "'\n";
        std::exit(1);
    }
}

int main() {
    {
        const auto i = dcrecomp::sh4::decode(0xE1FF, 0x8C010000); // mov #-1,r1
        require(i.opcode == Opcode::MovImm, "MOV immediate opcode");
        require(i.rn == 1, "MOV immediate Rn");
        require(i.immediate == -1, "MOV immediate sign extension");
    }
    {
        const auto i = dcrecomp::sh4::decode(0x720A, 0x8C010000); // add #10,r2
        require(i.opcode == Opcode::AddImm, "ADD immediate opcode");
        require(i.rn == 2 && i.immediate == 10, "ADD immediate operands");
    }
    {
        const auto i = dcrecomp::sh4::decode(0x312C, 0x8C010000); // add r2,r1
        require(i.opcode == Opcode::AddReg, "ADD register opcode");
        require(i.rn == 1 && i.rm == 2, "ADD register operands");
    }
    {
        const auto i = dcrecomp::sh4::decode(0xA000, 0x8C010000); // bra +0
        require(i.opcode == Opcode::Bra, "BRA opcode");
        require(i.target == 0x8C010004, "BRA target PC+4");
        require(i.has_delay_slot, "BRA delay slot");
    }
    {
        const auto i = dcrecomp::sh4::decode(0x8BFE, 0x8C010010); // bf -2 => PC+4-4 = PC
        require(i.opcode == Opcode::Bf, "BF opcode");
        require(i.target == 0x8C010010, "BF signed target");
        require(!i.has_delay_slot, "BF no delay slot");
    }
    {
        const auto i = dcrecomp::sh4::decode(0x8FFC, 0x8C010010); // bf/s -4
        require(i.opcode == Opcode::BfS, "BF/S opcode");
        require(i.has_delay_slot, "BF/S delay slot");
    }
    {
        const auto i = dcrecomp::sh4::decode(0xD103, 0x8C010002);
        require(i.opcode == Opcode::MovLPcRel, "MOV.L PC relative opcode");
        require(i.rn == 1, "MOV.L PC relative Rn");
        require(i.effective_address == 0x8C010010, "MOV.L PC relative alignment/scaling");
    }
    {
        const auto i = dcrecomp::sh4::decode(0x2FE6, 0x8C010000); // mov.l r14,@-r15
        require(i.opcode == Opcode::MovLPredec, "MOV.L predecrement opcode");
        require(i.rn == 15 && i.rm == 14, "MOV.L predecrement registers");
        require_text(0x2FE6, 0x8C010000, "mov.l r14,@-r15");
    }
    {
        const auto i = dcrecomp::sh4::decode(0x6246, 0x8C010000); // mov.l @r4+,r2
        require(i.opcode == Opcode::MovLPostinc, "MOV.L postincrement opcode");
        require(i.rn == 2 && i.rm == 4, "MOV.L postincrement registers");
        require_text(0x6246, 0x8C010000, "mov.l @r4+,r2");
    }
    {
        const auto i = dcrecomp::sh4::decode(0x2302, 0x8C010000); // mov.l r0,@r3
        require(i.opcode == Opcode::MovLStore, "MOV.L store opcode");
        require(i.rn == 3 && i.rm == 0, "MOV.L store registers");
        require_text(0x2302, 0x8C010000, "mov.l r0,@r3");
    }
    {
        const auto i = dcrecomp::sh4::decode(0x6032, 0x8C010000); // mov.l @r3,r0
        require(i.opcode == Opcode::MovLLoad, "MOV.L load opcode");
        require(i.rn == 0 && i.rm == 3, "MOV.L load registers");
        require_text(0x6032, 0x8C010000, "mov.l @r3,r0");
    }
    {
        const auto store = dcrecomp::sh4::decode(0x1323, 0x8C010000); // mov.l r2,@(12,r3)
        const auto load = dcrecomp::sh4::decode(0x5233, 0x8C010000);  // mov.l @(12,r3),r2
        require(store.opcode == Opcode::MovLDispStore && store.immediate == 3, "MOV.L disp store");
        require(load.opcode == Opcode::MovLDispLoad && load.immediate == 3, "MOV.L disp load");
        require_text(0x1323, 0x8C010000, "mov.l r2,@(12,r3)");
        require_text(0x5233, 0x8C010000, "mov.l @(12,r3),r2");
    }
    {
        const auto bload = dcrecomp::sh4::decode(0x8441, 0x8C010000); // mov.b @(1,r4),r0
        const auto wload = dcrecomp::sh4::decode(0x8541, 0x8C010000); // mov.w @(2,r4),r0
        const auto bstore = dcrecomp::sh4::decode(0x8050, 0x8C010000); // mov.b r0,@(0,r5)
        const auto wstore = dcrecomp::sh4::decode(0x8151, 0x8C010000); // mov.w r0,@(2,r5)
        require(bload.opcode == Opcode::MovBDispLoad && bload.rm == 4 && bload.immediate == 1, "MOV.B disp load");
        require(wload.opcode == Opcode::MovWDispLoad && wload.rm == 4 && wload.immediate == 2, "MOV.W disp load");
        require(bstore.opcode == Opcode::MovBDispStore && bstore.rn == 5 && bstore.immediate == 0, "MOV.B disp store");
        require(wstore.opcode == Opcode::MovWDispStore && wstore.rn == 5 && wstore.immediate == 2, "MOV.W disp store");
        require_text(0x8441, 0x8C010000, "mov.b @(1,r4),r0");
        require_text(0x8541, 0x8C010000, "mov.w @(2,r4),r0");
    }
    {
        const auto load = dcrecomp::sh4::decode(0x0ABC, 0x8C010000);
        const auto store = dcrecomp::sh4::decode(0x0BC4, 0x8C010000);
        require(load.opcode == Opcode::MovBIndexedLoad && load.rn == 10 && load.rm == 11, "MOV.B indexed load");
        require(store.opcode == Opcode::MovBIndexedStore && store.rn == 11 && store.rm == 12, "MOV.B indexed store");
        require_text(0x0ABC, 0x8C010000, "mov.b @(r0,r11),r10");
        require_text(0x0BC4, 0x8C010000, "mov.b r12,@(r0,r11)");
    }
    {
        const auto i = dcrecomp::sh4::decode(0x4F22, 0x8C010000); // sts.l pr,@-r15
        require(i.opcode == Opcode::StsLPr, "STS.L PR opcode");
        require(i.rn == 15, "STS.L PR register");
        require_text(0x4F22, 0x8C010000, "sts.l pr,@-r15");
    }
    {
        const auto i = dcrecomp::sh4::decode(0x4F26, 0x8C010000); // lds.l @r15+,pr
        require(i.opcode == Opcode::LdsLPr, "LDS.L PR opcode");
        require(i.rm == 15, "LDS.L PR register");
        require_text(0x4F26, 0x8C010000, "lds.l @r15+,pr");
    }
    {
        const auto i = dcrecomp::sh4::decode(0x430B, 0x8C010000); // jsr @r3
        require(i.opcode == Opcode::Jsr, "JSR opcode");
        require(i.rm == 3 && i.has_delay_slot, "JSR operand/delay slot");
    }
    {
        const auto i = dcrecomp::sh4::decode(0x000B, 0x8C010000);
        require(i.opcode == Opcode::Rts && i.has_delay_slot, "RTS delay slot");
    }
    {
        const auto i = dcrecomp::sh4::decode(0xFFFF, 0x8C010000);
        require(i.opcode == Opcode::Unknown, "Unknown remains unknown");
        require(!dcrecomp::sh4::is_known(i), "Unknown coverage classification");
    }

    std::cout << "SH-4 decoder tests: PASS\n";
    return 0;
}
