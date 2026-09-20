#include "dcrecomp/elf32.hpp"
#include "dcrecomp/program_analysis.hpp"
#include "dcrecomp/sh4_decoder.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
namespace {
void require(bool c,const char* m){if(!c) throw std::runtime_error(m);} 
void dec(std::uint16_t raw,dcrecomp::sh4::Opcode op,const char* m){require(dcrecomp::sh4::decode(raw,0x8C010000).opcode==op,m);} 
bool has(const dcrecomp::ProgramFunction& f,dcrecomp::DCIROp op){for(const auto& b:f.ir.blocks)for(const auto&i:b.instructions)if(i.op==op)return true;return false;}
}
int main(int argc,char**argv){try{
 if(argc!=2){std::cerr<<"usage: cpu_control_tests <sh4_cpu_control.elf>\n";return 2;}
 using O=dcrecomp::sh4::Opcode;
 dec(0x0019,O::Div0U,"DIV0U decode"); dec(0x2567,O::Div0S,"DIV0S decode"); dec(0x3124,O::Div1,"DIV1 decode");
 dec(0x256C,O::CmpStr,"CMP/STR decode"); dec(0x0423,O::Braf,"BRAF decode"); dec(0x0403,O::Bsrf,"BSRF decode");
 const auto elf=dcrecomp::load_elf32(argv[1]); const auto p=dcrecomp::analyze_reachable_program(elf,"_main");
 require(p.functions.size()==2,"expected _main + _helper");
 const dcrecomp::ProgramFunction* h=nullptr; for(const auto&f:p.functions)if(f.analysis.name=="_helper")h=&f; require(h,"helper missing");
 require(h->analysis.unknown==0,"unknown opcode in helper"); require(has(*h,dcrecomp::DCIROp::Div0U)&&has(*h,dcrecomp::DCIROp::Div0S)&&has(*h,dcrecomp::DCIROp::Div1),"division lowering missing");
 require(has(*h,dcrecomp::DCIROp::CmpStr),"CMP/STR lowering missing"); require(has(*h,dcrecomp::DCIROp::SaveDynamicRelativeTarget)&&has(*h,dcrecomp::DCIROp::DynamicBranch),"BRAF lowering missing");
 const auto& m=*std::find_if(p.functions.begin(),p.functions.end(),[](const auto&f){return f.analysis.name=="_main";}); require(has(m,dcrecomp::DCIROp::DynamicCall),"BSRF lowering missing");
 for(const auto&f:p.functions)for(const auto&b:f.ir.blocks)for(const auto&i:b.instructions)require(i.op!=dcrecomp::DCIROp::RawSH4,"unexpected RAW_SH4");
 std::cout<<"CPU control/DCIR tests: PASS\n"; return 0;
 }catch(const std::exception&e){std::cerr<<"CPU control/DCIR tests: FAIL: "<<e.what()<<"\n";return 1;}}
