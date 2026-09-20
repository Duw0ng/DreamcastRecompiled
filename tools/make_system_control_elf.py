#!/usr/bin/env python3
"""Generate SH-4 ELF samples for DreamcastRecomp 0.0.20 system/control tests."""
from __future__ import annotations
import argparse, struct
from pathlib import Path
BASE=0x8C010000
RTE_ADDR=0x8C010080
DATA=0x8C020000
EM_SH=42; ET_EXEC=2; EV_CURRENT=1

def align(v,a): return (v+a-1)&~(a-1)

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('output', nargs='?', default='samples/sh4_system_control.elf'); args=ap.parse_args()
    output=Path(args.output); output.parent.mkdir(parents=True, exist_ok=True)
    words=[
        0xE12A,       # mov #42,r1
        0x411E,       # ldc r1,gbr
        0x0012,       # stc gbr,r0  => 42
        0x0058,       # sets
        0x0048,       # clrs
        0x412E,       # ldc r1,vbr
        0x0222,       # stc vbr,r2
        0x4F13,       # stc.l gbr,@-r15
        0xE100,       # mov #0,r1
        0x411E,       # ldc r1,gbr
        0x4F17,       # ldc.l @r15+,gbr
        0x0312,       # stc gbr,r3
        0x400A,       # lds r0,mach
        0x4F02,       # sts.l mach,@-r15
        0x0028,       # clrmac
        0x4F06,       # lds.l @r15+,mach
        0x040A,       # sts mach,r4
        0xD500,       # mov.l literal,r5 (patched)
        0x0583,       # pref @r5 (normal RAM => cache hint/no-op)
        0x05C3,       # movca.l r0,@r5
        0x6652,       # mov.l @r5,r6
        0x6063,       # mov r6,r0 => must still be 42
        0x000B,       # rts
        0x0009,       # delay slot
    ]
    main_size=len(words)*2
    literal_off=align(main_size,4)
    # mov.l PC-relative: EA = ((address+4)&~3) + disp*4
    insn_addr=BASE+17*2
    pcbase=(insn_addr+4)&~3
    literal_addr=BASE+literal_off
    disp=(literal_addr-pcbase)//4
    assert 0 <= disp <= 0xFF
    words[17]=0xD500 | disp
    main_code=b''.join(struct.pack('<H',w) for w in words)
    text=bytearray(main_code + b'\0'*(literal_off-len(main_code)) + struct.pack('<I',DATA))
    rte_off=RTE_ADDR-BASE
    if len(text)>rte_off: raise RuntimeError('main overlaps rte probe')
    text.extend(b'\0'*(rte_off-len(text)))
    rte_code=struct.pack('<HH',0x002B,0x0029) # rte ; movt r0 delay slot
    text.extend(rte_code)
    data=b'\0'*4

    shstr=b'\0.text\0.data\0.shstrtab\0.strtab\0.symtab\0'
    names=['.text','.data','.shstrtab','.strtab','.symtab']; sh_name={n:shstr.index(n.encode()) for n in names}
    strtab=b'\0_main\0_rte_probe\0_system_word\0'
    syms=[b'\0'*16]
    syms.append(struct.pack('<IIIBBH',strtab.index(b'_main'),BASE,main_size,0x12,0,1))
    syms.append(struct.pack('<IIIBBH',strtab.index(b'_rte_probe'),RTE_ADDR,len(rte_code),0x12,0,1))
    syms.append(struct.pack('<IIIBBH',strtab.index(b'_system_word'),DATA,len(data),0x11,0,2))
    symtab=b''.join(syms)
    ehsize=52; shentsize=40; text_off=0x100; data_off=align(text_off+len(text),4); shstr_off=align(data_off+len(data),4); strtab_off=shstr_off+len(shstr); symtab_off=align(strtab_off+len(strtab),4); shoff=align(symtab_off+len(symtab),4); shnum=6; shstrndx=3
    ident=bytearray(16); ident[:4]=b'\x7fELF'; ident[4]=1; ident[5]=1; ident[6]=1
    ehdr=struct.pack('<16sHHIIIIIHHHHHH',bytes(ident),ET_EXEC,EM_SH,EV_CURRENT,BASE,0,shoff,0,ehsize,0,0,shentsize,shnum,shstrndx)
    def sh(name,typ,flags,addr,off,size,link=0,info=0,addralign=1,entsize=0): return struct.pack('<IIIIIIIIII',name,typ,flags,addr,off,size,link,info,addralign,entsize)
    sections=[sh(0,0,0,0,0,0),sh(sh_name['.text'],1,0x6,BASE,text_off,len(text),addralign=4),sh(sh_name['.data'],1,0x3,DATA,data_off,len(data),addralign=4),sh(sh_name['.shstrtab'],3,0,0,shstr_off,len(shstr)),sh(sh_name['.strtab'],3,0,0,strtab_off,len(strtab)),sh(sh_name['.symtab'],2,0,0,symtab_off,len(symtab),link=4,info=1,addralign=4,entsize=16)]
    blob=bytearray(shoff+shnum*shentsize); blob[:len(ehdr)]=ehdr
    blob[text_off:text_off+len(text)]=text; blob[data_off:data_off+len(data)]=data; blob[shstr_off:shstr_off+len(shstr)]=shstr; blob[strtab_off:strtab_off+len(strtab)]=strtab; blob[symtab_off:symtab_off+len(symtab)]=symtab
    for i,h in enumerate(sections): blob[shoff+i*shentsize:shoff+(i+1)*shentsize]=h
    output.write_bytes(blob)
    print(f'Wrote {output} ({len(blob)} bytes)')
if __name__=='__main__': main()
