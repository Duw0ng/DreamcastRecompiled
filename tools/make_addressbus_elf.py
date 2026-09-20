#!/usr/bin/env python3
"""Generate an ELF32/SH test for DreamcastRecomp 0.0.15.

The synthetic program mirrors the control/data-flow shape of KallistiOS
memTestAddressBus(): power-of-two offsets, addressMask, indexed 32-bit loads and
stores, nested loops, early error returns, and pointer arithmetic.

_main calls _memTestAddressBus(base=0x8C100000, nBytes=64). A correct in-memory
implementation must return R0 == 0.
"""
from __future__ import annotations
import argparse
import struct
from pathlib import Path

TEXT_BASE = 0x8C010000
MAIN = TEXT_BASE
FUNC = TEXT_BASE + 0x80
TEST_BASE = 0x8C100000
EM_SH = 42
ET_EXEC = 2
EV_CURRENT = 1

def align(v, a): return (v + a - 1) & ~(a - 1)

class Asm:
    def __init__(self, base: int):
        self.base = base
        self.words: list[int] = []
        self.labels: dict[str, int] = {}
        self.fixups: list[tuple[int, str, str]] = []
        self.literal_fixups: list[tuple[int, int, int]] = []  # word index, rn, value

    @property
    def pc(self): return self.base + len(self.words) * 2
    def label(self, name): self.labels[name] = self.pc
    def w(self, value): self.words.append(value & 0xFFFF)
    def mov_imm(self, imm, rn): self.w(0xE000 | (rn << 8) | (imm & 0xFF))
    def mov(self, rm, rn): self.w(0x6003 | (rn << 8) | (rm << 4))
    def add_imm(self, imm, rn): self.w(0x7000 | (rn << 8) | (imm & 0xFF))
    def add(self, rm, rn): self.w(0x300C | (rn << 8) | (rm << 4))
    def shll(self, rn): self.w(0x4000 | (rn << 8))
    def shll2(self, rn): self.w(0x4008 | (rn << 8))
    def shlr2(self, rn): self.w(0x4009 | (rn << 8))
    def tst(self, rm, rn): self.w(0x2008 | (rn << 8) | (rm << 4))
    def cmp_eq(self, rm, rn): self.w(0x3000 | (rn << 8) | (rm << 4))
    def store_l(self, rm, rn): self.w(0x2002 | (rn << 8) | (rm << 4))
    def load_l(self, rm, rn): self.w(0x6002 | (rn << 8) | (rm << 4))
    def store_l_idx(self, rm, base_rn): self.w(0x0006 | (base_rn << 8) | (rm << 4))
    def load_l_idx(self, base_rm, dst_rn): self.w(0x000E | (dst_rn << 8) | (base_rm << 4))
    def push_pr(self): self.w(0x4F22)
    def pop_pr(self): self.w(0x4F26)
    def rts(self): self.w(0x000B); self.w(0x0009)
    def bf(self, label):
        idx = len(self.words); self.w(0x8B00); self.fixups.append((idx, label, 'bf'))
    def bt(self, label):
        idx = len(self.words); self.w(0x8900); self.fixups.append((idx, label, 'bt'))
    def bra(self, label):
        idx = len(self.words); self.w(0xA000); self.fixups.append((idx, label, 'bra')); self.w(0x0009)
    def bsr(self, label):
        idx = len(self.words); self.w(0xB000); self.fixups.append((idx, label, 'bsr')); self.w(0x0009)
    def mov_l_literal(self, rn, value):
        idx = len(self.words); self.w(0xD000 | (rn << 8)); self.literal_fixups.append((idx, rn, value))

    def finish(self):
        # Append unique 32-bit literals aligned to 4 bytes.
        if self.pc & 3: self.w(0x0009)
        literal_addresses: dict[int, int] = {}
        for _, _, value in self.literal_fixups:
            if value not in literal_addresses:
                literal_addresses[value] = self.pc
                self.words.extend([value & 0xFFFF, (value >> 16) & 0xFFFF])

        for idx, label, kind in self.fixups:
            addr = self.base + idx * 2
            target = self.labels[label]
            disp_words = (target - (addr + 4)) // 2
            if kind in ('bf', 'bt'):
                if not -128 <= disp_words <= 127: raise ValueError((kind, label, disp_words))
                base = 0x8B00 if kind == 'bf' else 0x8900
                self.words[idx] = base | (disp_words & 0xFF)
            else:
                if not -2048 <= disp_words <= 2047: raise ValueError((kind, label, disp_words))
                base = 0xA000 if kind == 'bra' else 0xB000
                self.words[idx] = base | (disp_words & 0x0FFF)

        for idx, rn, value in self.literal_fixups:
            addr = self.base + idx * 2
            target = literal_addresses[value]
            pcbase = (addr + 4) & ~3
            disp = (target - pcbase) // 4
            if not 0 <= disp <= 255: raise ValueError(('literal', hex(addr), hex(target), disp))
            self.words[idx] = 0xD000 | (rn << 8) | disp
        return b''.join(struct.pack('<H', w) for w in self.words)

def make_main():
    a = Asm(MAIN)
    a.mov_l_literal(4, TEST_BASE)
    a.mov_imm(64, 5)
    a.push_pr()
    bsr_idx = len(a.words); a.w(0xB000); a.w(0x0009)  # bsr FUNC + delay
    a.pop_pr()
    a.rts()
    # This assembler only knows local labels; patch BSR manually after finish.
    code = bytearray(a.finish())
    # Patch BSR target after literal-pool placement.
    addr = MAIN + bsr_idx*2; disp = (FUNC - (addr+4))//2
    struct.pack_into('<H', code, bsr_idx*2, 0xB000 | (disp & 0x0FFF))
    return bytes(code)

def make_func():
    a = Asm(FUNC)
    # r4=baseAddress, r5=nBytes
    # r6=addressMask=(nBytes>>2)-1; r9=pattern; r10=antipattern.
    a.mov(5,6); a.shlr2(6); a.add_imm(-1,6)
    a.mov_l_literal(9, 0xAAAAAAAA)
    a.mov_l_literal(10,0x55555555)

    # Write pattern at each power-of-two offset.
    a.mov_imm(1,7)
    a.label('write_loop')
    a.mov(7,0); a.shll2(0); a.store_l_idx(9,4)
    a.shll(7); a.mov(7,0); a.tst(6,0); a.bf('write_loop')

    # Check for address lines stuck high.
    a.store_l(10,4)
    a.mov_imm(1,7)
    a.label('check_high')
    a.mov(7,0); a.shll2(0); a.load_l_idx(4,1)
    a.cmp_eq(9,1); a.bf('fail_offset')
    a.shll(7); a.mov(7,0); a.tst(6,0); a.bf('check_high')
    a.store_l(9,4)

    # Check stuck-low/shorted address lines with nested loops.
    a.mov_imm(1,8)
    a.label('outer')
    a.mov(8,0); a.shll2(0); a.store_l_idx(10,4)
    a.load_l(4,1); a.cmp_eq(9,1); a.bf('fail_test')
    a.mov_imm(1,7)
    a.label('inner')
    a.mov(7,0); a.shll2(0); a.load_l_idx(4,1)
    a.cmp_eq(9,1); a.bt('inner_continue')
    a.cmp_eq(8,7); a.bf('fail_test')
    a.label('inner_continue')
    a.shll(7); a.mov(7,0); a.tst(6,0); a.bf('inner')

    a.mov(8,0); a.shll2(0); a.store_l_idx(9,4)
    a.shll(8); a.mov(8,0); a.tst(6,0); a.bf('outer')

    a.mov_imm(0,0); a.rts()

    a.label('fail_offset')
    a.mov(7,0); a.shll2(0); a.add(4,0); a.rts()

    a.label('fail_test')
    a.mov(8,0); a.shll2(0); a.add(4,0); a.rts()
    return a.finish()

def main():
    ap = argparse.ArgumentParser(); ap.add_argument('output', nargs='?', default='samples/sh4_addressbus.elf'); args=ap.parse_args()
    output=Path(args.output); output.parent.mkdir(parents=True, exist_ok=True)

    main_code = bytearray(make_main())
    func = make_func()
    text = bytearray(0x80 + len(func))
    text[:len(main_code)] = main_code
    for off in range(len(main_code), 0x80, 2): text[off:off+2] = struct.pack('<H',0x0009)
    text[0x80:0x80+len(func)] = func

    shstr=b'\0.text\0.shstrtab\0.strtab\0.symtab\0'
    names=['.text','.shstrtab','.strtab','.symtab']; sh_name={n:shstr.index(n.encode()) for n in names}
    strtab=b'\0_main\0_memTestAddressBus\0'
    sym_null=b'\0'*16
    sym_main=struct.pack('<IIIBBH',strtab.index(b'_main'),MAIN,len(main_code),0x12,0,1)
    sym_func=struct.pack('<IIIBBH',strtab.index(b'_memTestAddressBus'),FUNC,len(func),0x12,0,1)
    symtab=sym_null+sym_main+sym_func

    ehsize=52; shentsize=40; text_off=0x100
    shstr_off=align(text_off+len(text),4); strtab_off=shstr_off+len(shstr); symtab_off=align(strtab_off+len(strtab),4); shoff=align(symtab_off+len(symtab),4)
    shnum=5; shstrndx=2
    ident=bytearray(16); ident[:4]=b'\x7fELF'; ident[4]=1; ident[5]=1; ident[6]=1
    ehdr=struct.pack('<16sHHIIIIIHHHHHH',bytes(ident),ET_EXEC,EM_SH,EV_CURRENT,MAIN,0,shoff,0,ehsize,0,0,shentsize,shnum,shstrndx)
    def sh(name,typ,flags,addr,off,size,link=0,info=0,addralign=1,entsize=0): return struct.pack('<IIIIIIIIII',name,typ,flags,addr,off,size,link,info,addralign,entsize)
    sections=[
        sh(0,0,0,0,0,0),
        sh(sh_name['.text'],1,0x6,TEXT_BASE,text_off,len(text),addralign=2),
        sh(sh_name['.shstrtab'],3,0,0,shstr_off,len(shstr)),
        sh(sh_name['.strtab'],3,0,0,strtab_off,len(strtab)),
        sh(sh_name['.symtab'],2,0,0,symtab_off,len(symtab),link=3,info=1,addralign=4,entsize=16),
    ]
    blob=bytearray(shoff+shnum*shentsize); blob[:len(ehdr)]=ehdr; blob[text_off:text_off+len(text)]=text
    blob[shstr_off:shstr_off+len(shstr)]=shstr; blob[strtab_off:strtab_off+len(strtab)]=strtab; blob[symtab_off:symtab_off+len(symtab)]=symtab
    for i,h in enumerate(sections): blob[shoff+i*shentsize:shoff+(i+1)*shentsize]=h
    output.write_bytes(blob)
    print(f'Wrote {output} ({len(blob)} bytes)')
    print('Expected native result: R0=0; nested address-bus loops complete without alias errors')

if __name__=='__main__': main()
