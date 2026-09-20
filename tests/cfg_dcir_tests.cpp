#include "dcrecomp/cfg.hpp"
#include "dcrecomp/dcir.hpp"
#include "dcrecomp/elf32.hpp"
#include "dcrecomp/function_analysis.hpp"
#include "dcrecomp/sh4_context.hpp"

#include <cstdlib>
#include <iostream>

namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: cfg_dcir_tests <literal_pool.elf>\n";
        return 2;
    }

    dcrecomp::SH4Context context{};
    require(context.r[0] == 0 && context.pc == 0 && context.pr == 0, "SH4Context starts zeroed");

    const auto elf = dcrecomp::load_elf32(argv[1]);
    const auto analysis = dcrecomp::analyze_function(elf, "_main");
    const auto cfg = dcrecomp::build_cfg(analysis);

    require(cfg.blocks.size() == 2, "_main splits into two basic blocks around JSR return site");
    require(cfg.blocks[0].start_address == 0x8C010000, "first block start");
    require(cfg.blocks[0].end_address == 0x8C01000A, "first block contains JSR delay slot");
    require(cfg.blocks[1].start_address == 0x8C01000A, "second block begins after call delay slot");
    require(cfg.edges.size() == 1, "one intra-function CFG edge");
    require(cfg.edges[0].kind == dcrecomp::CFGEdgeKind::CallReturn, "edge is call-return");
    require(cfg.edges[0].to == 0x8C01000A, "call returns to second block");

    const auto ir = dcrecomp::lower_to_dcir(elf, analysis, cfg);
    require(ir.blocks.size() == 2, "DCIR preserves CFG blocks");
    require(ir.blocks[0].instructions.size() == 5, "first DCIR block has load/push/load/dynamic-target/call");
    require(ir.blocks[0].instructions[0].op == dcrecomp::DCIROp::LoadLiteral32, "IR load _printf literal");
    require(ir.blocks[0].instructions[0].symbol == "_printf", "IR retains _printf symbol");
    require(ir.blocks[0].instructions[1].op == dcrecomp::DCIROp::PushPR, "IR push PR");
    require(ir.blocks[0].instructions[2].op == dcrecomp::DCIROp::LoadLiteral32, "IR load string literal");
    require(ir.blocks[0].instructions[3].op == dcrecomp::DCIROp::SaveDynamicAbsoluteTarget, "IR snapshots JSR register target before the delay slot");
    require(ir.blocks[0].instructions[4].op == dcrecomp::DCIROp::DynamicCall, "JSR remains runtime-dynamic even when analysis observed one target");

    require(ir.blocks[1].instructions.size() == 3, "second block has mov/pop/return (delay NOP removed)");
    require(ir.blocks[1].instructions[0].op == dcrecomp::DCIROp::MovImm, "return value lowered");
    require(ir.blocks[1].instructions[1].op == dcrecomp::DCIROp::PopPR, "IR pop PR");
    require(ir.blocks[1].instructions[2].op == dcrecomp::DCIROp::Return, "IR return");

    // A non-NOP delay slot must be lowered before the control transfer.
    dcrecomp::FunctionAnalysis delay_analysis;
    delay_analysis.name = "delay_slot_probe";
    delay_analysis.start_address = 0x1000;
    delay_analysis.end_address = 0x1008;

    dcrecomp::sh4::Instruction load;
    load.opcode = dcrecomp::sh4::Opcode::MovImm;
    load.address = 0x1000;
    load.rn = 1;
    load.immediate = 7;

    dcrecomp::sh4::Instruction jsr;
    jsr.opcode = dcrecomp::sh4::Opcode::Jsr;
    jsr.address = 0x1002;
    jsr.rm = 0;
    jsr.has_delay_slot = true;

    dcrecomp::sh4::Instruction slot;
    slot.opcode = dcrecomp::sh4::Opcode::MovImm;
    slot.address = 0x1004;
    slot.rn = 2;
    slot.immediate = 9;

    delay_analysis.instructions = {load, jsr, slot};
    delay_analysis.calls.push_back({0x1002, false, true, 0x2000, "target", ".text"});

    dcrecomp::ControlFlowGraph delay_cfg;
    delay_cfg.function_name = delay_analysis.name;
    delay_cfg.entry = 0x1000;
    delay_cfg.blocks.push_back({0x1000, 0x1006, delay_analysis.instructions});

    const auto delay_ir = dcrecomp::lower_to_dcir(elf, delay_analysis, delay_cfg);
    require(delay_ir.blocks.size() == 1, "delay-slot probe one block");
    require(delay_ir.blocks[0].instructions.size() == 4, "delay-slot probe four IR ops");
    require(delay_ir.blocks[0].instructions[0].op == dcrecomp::DCIROp::MovImm, "pre-call op preserved");
    require(delay_ir.blocks[0].instructions[1].op == dcrecomp::DCIROp::SaveDynamicAbsoluteTarget,
            "JSR target is sampled before the delay slot");
    require(delay_ir.blocks[0].instructions[2].op == dcrecomp::DCIROp::MovImm &&
            delay_ir.blocks[0].instructions[2].source_address == 0x1004,
            "delay slot lowered after target snapshot");
    require(delay_ir.blocks[0].instructions[3].op == dcrecomp::DCIROp::DynamicCall, "dynamic call emitted after delay slot");

    // MOVA must remain distinct from an ordinary PC-relative literal load so
    // copied/relocated code can recompute the address from its execution site.
    dcrecomp::FunctionAnalysis mova_analysis;
    mova_analysis.name = "mova_probe";
    mova_analysis.start_address = 0x1800;
    mova_analysis.end_address = 0x1804;
    dcrecomp::sh4::Instruction mova;
    mova.opcode = dcrecomp::sh4::Opcode::Mova;
    mova.address = 0x1800;
    mova.rn = 0;
    mova.effective_address = 0x1840;
    dcrecomp::sh4::Instruction mova_nop;
    mova_nop.opcode = dcrecomp::sh4::Opcode::Nop;
    mova_nop.address = 0x1802;
    mova_analysis.instructions = {mova, mova_nop};
    dcrecomp::ControlFlowGraph mova_cfg;
    mova_cfg.function_name = mova_analysis.name;
    mova_cfg.entry = 0x1800;
    mova_cfg.blocks.push_back({0x1800, 0x1804, mova_analysis.instructions});
    const auto mova_ir = dcrecomp::lower_to_dcir(elf, mova_analysis, mova_cfg);
    require(!mova_ir.blocks.empty() && !mova_ir.blocks[0].instructions.empty(), "MOVA probe lowers");
    require(mova_ir.blocks[0].instructions[0].op == dcrecomp::DCIROp::Mova, "MOVA has relocation-aware DCIR opcode");
    require(mova_ir.blocks[0].instructions[0].value == 0x1840u, "MOVA preserves effective PC-relative address");

    // Conditional branch produces separate taken/not-taken CFG edges.
    dcrecomp::FunctionAnalysis branch_analysis;
    branch_analysis.name = "branch_probe";
    branch_analysis.start_address = 0x2000;
    branch_analysis.end_address = 0x200E;

    dcrecomp::sh4::Instruction bt;
    bt.opcode = dcrecomp::sh4::Opcode::Bt;
    bt.address = 0x2000;
    bt.target = 0x2008;
    dcrecomp::sh4::Instruction fmov;
    fmov.opcode = dcrecomp::sh4::Opcode::MovImm;
    fmov.address = 0x2002;
    dcrecomp::sh4::Instruction frts;
    frts.opcode = dcrecomp::sh4::Opcode::Rts;
    frts.address = 0x2004;
    frts.has_delay_slot = true;
    dcrecomp::sh4::Instruction fnop;
    fnop.opcode = dcrecomp::sh4::Opcode::Nop;
    fnop.address = 0x2006;
    dcrecomp::sh4::Instruction tmov = fmov;
    tmov.address = 0x2008;
    dcrecomp::sh4::Instruction trts = frts;
    trts.address = 0x200A;
    dcrecomp::sh4::Instruction tnop = fnop;
    tnop.address = 0x200C;
    branch_analysis.instructions = {bt, fmov, frts, fnop, tmov, trts, tnop};

    const auto branch_cfg = dcrecomp::build_cfg(branch_analysis);
    require(branch_cfg.blocks.size() == 3, "conditional branch creates three blocks");
    require(branch_cfg.edges.size() == 2, "conditional branch creates two edges");
    bool saw_taken = false;
    bool saw_not_taken = false;
    for (const auto& edge : branch_cfg.edges) {
        saw_taken |= edge.kind == dcrecomp::CFGEdgeKind::ConditionalTaken && edge.to == 0x2008;
        saw_not_taken |= edge.kind == dcrecomp::CFGEdgeKind::ConditionalNotTaken && edge.to == 0x2002;
    }
    require(saw_taken, "taken edge target");
    require(saw_not_taken, "not-taken edge target");

    // BT/S and BF/S sample T before their delay slot. The slot is allowed to
    // modify T, so DCIR must preserve the pre-slot value explicitly.
    dcrecomp::FunctionAnalysis delayed_t_analysis;
    delayed_t_analysis.name = "delayed_t_probe";
    delayed_t_analysis.start_address = 0x3000;
    delayed_t_analysis.end_address = 0x3006;

    dcrecomp::sh4::Instruction cmp_before;
    cmp_before.opcode = dcrecomp::sh4::Opcode::CmpEq;
    cmp_before.address = 0x3000;
    cmp_before.rn = 2;
    cmp_before.rm = 1;

    dcrecomp::sh4::Instruction bts;
    bts.opcode = dcrecomp::sh4::Opcode::BtS;
    bts.address = 0x3002;
    bts.target = 0x3010;
    bts.has_delay_slot = true;

    dcrecomp::sh4::Instruction cmp_slot;
    cmp_slot.opcode = dcrecomp::sh4::Opcode::CmpEq;
    cmp_slot.address = 0x3004;
    cmp_slot.rn = 4;
    cmp_slot.rm = 3;

    delayed_t_analysis.instructions = {cmp_before, bts, cmp_slot};
    dcrecomp::ControlFlowGraph delayed_t_cfg;
    delayed_t_cfg.function_name = delayed_t_analysis.name;
    delayed_t_cfg.entry = 0x3000;
    delayed_t_cfg.blocks.push_back({0x3000, 0x3006, delayed_t_analysis.instructions});

    const auto delayed_t_ir = dcrecomp::lower_to_dcir(elf, delayed_t_analysis, delayed_t_cfg);
    require(delayed_t_ir.blocks.size() == 1, "delayed T probe one block");
    require(delayed_t_ir.blocks[0].instructions.size() == 4, "delayed T probe four IR ops");
    require(delayed_t_ir.blocks[0].instructions[0].op == dcrecomp::DCIROp::CmpEq, "pre-branch compare preserved");
    require(delayed_t_ir.blocks[0].instructions[1].op == dcrecomp::DCIROp::SaveT, "BT/S saves T before slot");
    require(delayed_t_ir.blocks[0].instructions[2].op == dcrecomp::DCIROp::CmpEq &&
            delayed_t_ir.blocks[0].instructions[2].source_address == 0x3004,
            "T-changing delay slot executes after T snapshot");
    require(delayed_t_ir.blocks[0].instructions[3].op == dcrecomp::DCIROp::BranchIfSavedTrue,
            "BT/S branches using saved T");


    // A delay slot may also be a separate CFG entry. The fall-through JMP @Rn
    // still has to snapshot Rn and execute that shared slot before branching.
    dcrecomp::FunctionAnalysis split_delay_analysis;
    split_delay_analysis.name = "split_delay_probe";
    split_delay_analysis.start_address = 0x4000;
    split_delay_analysis.end_address = 0x4006;

    dcrecomp::sh4::Instruction split_jmp;
    split_jmp.opcode = dcrecomp::sh4::Opcode::Jmp;
    split_jmp.address = 0x4000;
    split_jmp.rm = 0;
    split_jmp.has_delay_slot = true;

    dcrecomp::sh4::Instruction split_slot;
    split_slot.opcode = dcrecomp::sh4::Opcode::MovImm;
    split_slot.address = 0x4002;
    split_slot.rn = 3;
    split_slot.immediate = 11;

    dcrecomp::sh4::Instruction split_tail;
    split_tail.opcode = dcrecomp::sh4::Opcode::Nop;
    split_tail.address = 0x4004;

    split_delay_analysis.instructions = {split_jmp, split_slot, split_tail};
    dcrecomp::ControlFlowGraph split_delay_cfg;
    split_delay_cfg.function_name = split_delay_analysis.name;
    split_delay_cfg.entry = 0x4000;
    split_delay_cfg.blocks.push_back({0x4000, 0x4002, {split_jmp}});
    split_delay_cfg.blocks.push_back({0x4002, 0x4006, {split_slot, split_tail}});

    const auto split_delay_ir = dcrecomp::lower_to_dcir(elf, split_delay_analysis, split_delay_cfg);
    require(split_delay_ir.blocks.size() == 2, "split delay probe preserves two CFG blocks");
    require(split_delay_ir.blocks[0].instructions.size() == 3, "split JMP block snapshots target, executes shared slot, then branches");
    require(split_delay_ir.blocks[0].instructions[0].op == dcrecomp::DCIROp::SaveDynamicAbsoluteTarget &&
            split_delay_ir.blocks[0].instructions[0].src == 0,
            "split JMP snapshots R0 before its cross-block delay slot");
    require(split_delay_ir.blocks[0].instructions[1].op == dcrecomp::DCIROp::MovImm &&
            split_delay_ir.blocks[0].instructions[1].source_address == 0x4002,
            "split JMP executes shared delay-slot instruction locally");
    require(split_delay_ir.blocks[0].instructions[2].op == dcrecomp::DCIROp::DynamicBranch,
            "split JMP emits dynamic branch after shared delay slot");
    require(split_delay_ir.blocks[1].instructions[0].source_address == 0x4002,
            "shared delay-slot remains independently executable as CFG entry");

    std::cout << "CFG/DCIR tests: PASS\n";
    return 0;
}
