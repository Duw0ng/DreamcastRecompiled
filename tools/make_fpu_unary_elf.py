#!/usr/bin/env python3
"""Generate SH-4 ELF covering 0.0.19 unary/conversion/vector FPU ops."""
from __future__ import annotations
import argparse, struct
from pathlib import Path
BASE=0x8C010000; EM_SH=42; ET_EXEC=2; EV_CURRENT=1

def align(v,a): return (v+a-1)&~(a-1)

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('output', nargs='?', default='samples/sh4_fpu_unary.elf'); a=ap.parse_args()
    output=Path(a.output); output.parent.mkdir(parents=True, exist_ok=True)
    words=[]; labels={}; bfs=[]; lits=[]
    def emit(w): words.append(w)
    def label(n): labels[n]=len(words)*2
    def bf(n): bfs.append((len(words),n)); emit(0x8B00)
    def movl_pc(rn,val): idx=len(words); emit(0xD000|(rn<<8)); lits.append((idx,rn,val))
    def check_fr(fr, expect):
        emit(0xF000 | (fr<<8) | 0x3D)  # ftrc frN,fpul
        emit(0x005A)                    # sts fpul,r0
        emit(0x8800 | (expect & 0xff))  # cmp/eq #imm,r0
        bf('fail')

    # FLOAT/FNEG/FABS/FTRC: 42 -> -42 -> 42 -> integer 42.
    emit(0xE12A); emit(0x415A)          # mov #42,r1; lds r1,fpul
    emit(0xF02D); emit(0xF04D); emit(0xF05D)
    check_fr(0,42)

    # FLDI1 + FSQRT + FSRRA + FLDS/FSTS raw-bit path => still 1.0.
    emit(0xF19D); emit(0xF16D); emit(0xF17D)
    emit(0xF11D); emit(0xF20D)          # flds fr1,fpul; fsts fpul,fr2
    check_fr(2,1)

    # FCNVSD / FCNVDS with PR=1: 1.0f -> 1.0 double -> 1.0f.
    emit(0xF21D)                        # flds fr2,fpul
    movl_pc(3,0x00080000); emit(0x436A) # lds r3,fpscr
    emit(0xF4AD); emit(0xF4BD)          # fcnvsd fpul,dr4; fcnvds dr4,fpul
    emit(0xE300); emit(0x436A)          # PR=0
    emit(0xFA0D)                        # fsts fpul,fr10
    check_fr(10,1)

    # FIPR: dot([1,1,1,1],[1,1,1,1]) = 4, result in FR7.
    for fr in range(8): emit(0xF000 | (fr<<8) | 0x9D)
    emit(0xF4ED)                        # fipr fv0,fv4
    check_fr(7,4)

    # Build identity XMTRX in XF bank by toggling FR, then transform FV0.
    emit(0xFBFD)                        # frchg -> active XF
    for fr in range(16): emit(0xF000 | (fr<<8) | 0x8D) # fldi0
    for fr in (0,5,10,15): emit(0xF000 | (fr<<8) | 0x9D)
    emit(0xFBFD)                        # back to FR bank
    for fr in range(4): emit(0xF000 | (fr<<8) | 0x9D)
    emit(0xF1FD)                        # ftrv xmtrx,fv0
    check_fr(3,1)

    # FSCA angle 0 => sin=0 in FR8, cos=1 in FR9.
    emit(0xE100); emit(0x415A); emit(0xF8FD)
    check_fr(9,1)

    # FSCHG state transition exercised twice so 32-bit mode is restored.
    emit(0xF3FD); emit(0xF3FD)

    emit(0xE02A); emit(0x000B); emit(0x0009)
    label('fail'); emit(0xE0FF); emit(0x000B); emit(0x0009)

    for idx,n in bfs:
        delta=labels[n]-(idx*2+4); assert delta%2==0; d=delta//2; assert -128<=d<=127
        words[idx]=0x8B00|(d&0xff)
    code_size=len(words)*2; lit_off=align(code_size,4)
    vals=[]
    for _,_,v in lits:
        if v not in vals: vals.append(v)
    vo={v:lit_off+i*4 for i,v in enumerate(vals)}
    for idx,rn,v in lits:
        pc=BASE+idx*2; base=(pc+4)&~3; disp=(BASE+vo[v]-base)//4; assert 0<=disp<=255
        words[idx]=0xD000|(rn<<8)|disp
    code=b''.join(struct.pack('<H',w) for w in words)
    text=code+b'\0'*(lit_off-len(code))+b''.join(struct.pack('<I',v) for v in vals)

    shstr=b'\0.text\0.shstrtab\0.strtab\0.symtab\0'; names=['.text','.shstrtab','.strtab','.symtab']; sn={n:shstr.index(n.encode()) for n in names}
    strtab=b'\0_main\0'; symtab=b'\0'*16+struct.pack('<IIIBBH',strtab.index(b'_main'),BASE,code_size,0x12,0,1)
    ehsize=52; shentsize=40; text_off=0x100; shstr_off=align(text_off+len(text),4); strtab_off=shstr_off+len(shstr); symtab_off=align(strtab_off+len(strtab),4); shoff=align(symtab_off+len(symtab),4); shnum=5
    ident=bytearray(16); ident[:4]=b'\x7fELF'; ident[4]=1; ident[5]=1; ident[6]=1
    eh=struct.pack('<16sHHIIIIIHHHHHH',bytes(ident),ET_EXEC,EM_SH,EV_CURRENT,BASE,0,shoff,0,ehsize,0,0,shentsize,shnum,2)
    def sh(name,typ,flags,addr,off,size,link=0,info=0,aa=1,es=0): return struct.pack('<IIIIIIIIII',name,typ,flags,addr,off,size,link,info,aa,es)
    secs=[sh(0,0,0,0,0,0),sh(sn['.text'],1,6,BASE,text_off,len(text),aa=4),sh(sn['.shstrtab'],3,0,0,shstr_off,len(shstr)),sh(sn['.strtab'],3,0,0,strtab_off,len(strtab)),sh(sn['.symtab'],2,0,0,symtab_off,len(symtab),link=3,info=1,aa=4,es=16)]
    blob=bytearray(shoff+shnum*shentsize); blob[:len(eh)]=eh; blob[text_off:text_off+len(text)]=text; blob[shstr_off:shstr_off+len(shstr)]=shstr; blob[strtab_off:strtab_off+len(strtab)]=strtab; blob[symtab_off:symtab_off+len(symtab)]=symtab
    for i,h in enumerate(secs): blob[shoff+i*shentsize:shoff+(i+1)*shentsize]=h
    output.write_bytes(blob); print(f'Wrote {output} ({len(words)} SH-4 words)')
if __name__=='__main__': main()
