#!/usr/bin/env python3
"""Generate an ELF32/SH test for DreamcastRecomp 0.0.15.

The synthetic program exercises structures and mixed-width data:

    struct Record {
        uint8_t flags;       // 0xA5
        int8_t  delta;       // -3
        uint16_t value;      // 0x1234
        uint32_t payload;    // 0x01020304
        uint8_t bytes[4];    // 0x10,0x20,0x30,0x40
    };

_process_record() uses byte/word displacement loads, long displacement loads,
extension, logical operations, shifts/rotates, indexed loads/stores, and mixed-width
stores. The final value loaded back from the output structure is R0 == 42.
"""
from __future__ import annotations
import argparse, struct
from pathlib import Path

TEXT_BASE = 0x8C010000
MAIN = TEXT_BASE
PROCESS = TEXT_BASE + 0x80
DATA_BASE = 0x8C020000
RECORD = DATA_BASE
BSS_BASE = 0x8C030000
OUT = BSS_BASE
EM_SH = 42
ET_EXEC = 2
EV_CURRENT = 1

def align(v,a): return (v+a-1)&~(a-1)

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('output', nargs='?', default='samples/sh4_structs.elf'); args=ap.parse_args()
    output=Path(args.output); output.parent.mkdir(parents=True, exist_ok=True)

    # _main: R4=&record, R5=&out, call _process_record, return its R0.
    main_words=[
        0xD405, # +00 mov.l @(20,pc),r4 -> literal +18
        0xD506, # +02 mov.l @(24,pc),r5 -> literal +1c
        0x4F22, # +04 sts.l pr,@-r15
        0xB03B, # +06 bsr _process_record @ +80
        0x0009, # +08 delay
        0x4F26, # +0a lds.l @r15+,pr
        0x000B, # +0c rts
        0x0009, # +0e delay
        0x0009,0x0009,0x0009,0x0009, # +10..+16 padding
    ]
    main_code=b''.join(struct.pack('<H',w) for w in main_words)+struct.pack('<I',RECORD)+struct.pack('<I',OUT)
    assert len(main_code)==0x20

    # _process_record @ +0x80.
    # Result path: flags low nibble 5 + value high byte 18 + payload byte 2 + delta -3 = 22;
    # xor 8 => 30; or 0x20 => 62; and 0x2A => 42.
    w=[
        0x8440, # mov.b @(0,r4),r0
        0x620C, # extu.b r0,r2
        0x6023, # mov r2,r0
        0xC90F, # and #0x0f,r0
        0x6203, # mov r0,r2  (5)

        0x8441, # mov.b @(1,r4),r0  (-3, sign extended)
        0x660E, # exts.b r0,r6

        0x8541, # mov.w @(2,r4),r0
        0x630D, # extu.w r0,r3
        0x6733, # mov r3,r7
        0x4719, # shlr8 r7 -> 0x12

        0x5841, # mov.l @(4,r4),r8
        0x4829, # shlr16 r8 -> 0x0102
        0x6083, # mov r8,r0
        0xC9FF, # and #0xff,r0 -> 2
        0x6803, # mov r0,r8

        0x302C, # add r2,r0 -> 7
        0x307C, # add r7,r0 -> 25
        0x306C, # add r6,r0 -> 22
        0xCA08, # xor #8,r0 -> 30
        0xCB20, # or #0x20,r0 -> 62
        0xC92A, # and #0x2a,r0 -> 42

        0x8050, # mov.b r0,@(0,r5)
        0x8151, # mov.w r0,@(2,r5)
        0x1501, # mov.l r0,@(4,r5)

        0x6903, # mov r0,r9 save 42
        0x6B43, # mov r4,r11
        0x7B08, # add #8,r11 -> bytes[] base
        0xE003, # mov #3,r0 index
        0xEC7F, # mov #127,r12
        0x0BC4, # mov.b r12,@(r0,r11) indexed store (bytes[3]=127)
        0x0DBC, # mov.b @(r0,r11),r13 indexed load
        0x6DDC, # extu.b r13,r13
        0xE002, # mov #2,r0
        0x0ABC, # mov.b @(r0,r11),r10 -> bytes[2]=0x30
        0x6AAC, # extu.b r10,r10
        0x4A08, # shll2 r10
        0x4A09, # shlr2 r10
        0x4A18, # shll8 r10
        0x4A19, # shlr8 r10
        0x4A04, # rotl r10
        0x4A05, # rotr r10
        0x2DA9, # and r10,r13
        0x2DAA, # xor r10,r13
        0x2DAB, # or r10,r13

        0x6093, # mov r9,r0 restore 42
        0x8450, # mov.b @(0,r5),r0
        0x600C, # extu.b r0,r0
        0x8551, # mov.w @(2,r5),r0
        0x600D, # extu.w r0,r0
        0x5051, # mov.l @(4,r5),r0
        0x000B, # rts
        0x0009, # delay
    ]
    proc=b''.join(struct.pack('<H',x) for x in w)

    text=bytearray(0x80+len(proc)); text[:len(main_code)]=main_code
    for off in range(len(main_code),0x80,2): text[off:off+2]=struct.pack('<H',0x0009)
    text[0x80:0x80+len(proc)]=proc

    record=struct.pack('<BbHI4B',0xA5,-3,0x1234,0x01020304,0x10,0x20,0x30,0x40)
    assert len(record)==12
    bss_size=8

    shstr=b'\0.text\0.data\0.bss\0.shstrtab\0.strtab\0.symtab\0'
    names=['.text','.data','.bss','.shstrtab','.strtab','.symtab']; sh_name={n:shstr.index(n.encode()) for n in names}
    strtab=b'\0_main\0_process_record\0_record\0_out\0'
    sym_null=b'\0'*16
    sym_main=struct.pack('<IIIBBH',strtab.index(b'_main'),MAIN,len(main_code),0x12,0,1)
    sym_proc=struct.pack('<IIIBBH',strtab.index(b'_process_record'),PROCESS,len(proc),0x12,0,1)
    sym_record=struct.pack('<IIIBBH',strtab.index(b'_record'),RECORD,len(record),0x11,0,2)
    sym_out=struct.pack('<IIIBBH',strtab.index(b'_out'),OUT,bss_size,0x11,0,3)
    symtab=sym_null+sym_main+sym_proc+sym_record+sym_out

    ehsize=52; shentsize=40; text_off=0x100; data_off=align(text_off+len(text),4); bss_off=data_off+len(record)
    shstr_off=bss_off; strtab_off=shstr_off+len(shstr); symtab_off=align(strtab_off+len(strtab),4); shoff=align(symtab_off+len(symtab),4)
    shnum=7; shstrndx=4
    ident=bytearray(16); ident[:4]=b'\x7fELF'; ident[4]=1; ident[5]=1; ident[6]=1
    ehdr=struct.pack('<16sHHIIIIIHHHHHH',bytes(ident),ET_EXEC,EM_SH,EV_CURRENT,MAIN,0,shoff,0,ehsize,0,0,shentsize,shnum,shstrndx)
    def sh(name,typ,flags,addr,off,size,link=0,info=0,addralign=1,entsize=0):
        return struct.pack('<IIIIIIIIII',name,typ,flags,addr,off,size,link,info,addralign,entsize)
    sections=[
        sh(0,0,0,0,0,0),
        sh(sh_name['.text'],1,0x6,TEXT_BASE,text_off,len(text),addralign=2),
        sh(sh_name['.data'],1,0x3,DATA_BASE,data_off,len(record),addralign=4),
        sh(sh_name['.bss'],8,0x3,BSS_BASE,bss_off,bss_size,addralign=4),
        sh(sh_name['.shstrtab'],3,0,0,shstr_off,len(shstr)),
        sh(sh_name['.strtab'],3,0,0,strtab_off,len(strtab)),
        sh(sh_name['.symtab'],2,0,0,symtab_off,len(symtab),link=5,info=1,addralign=4,entsize=16),
    ]
    blob=bytearray(shoff+shnum*shentsize); blob[:len(ehdr)]=ehdr; blob[text_off:text_off+len(text)]=text; blob[data_off:data_off+len(record)]=record
    blob[shstr_off:shstr_off+len(shstr)]=shstr; blob[strtab_off:strtab_off+len(strtab)]=strtab; blob[symtab_off:symtab_off+len(symtab)]=symtab
    for i,h in enumerate(sections): blob[shoff+i*shentsize:shoff+(i+1)*shentsize]=h
    output.write_bytes(blob)
    print(f'Wrote {output} ({len(blob)} bytes)')
    print('Expected native result: R0=42; _out byte/word/long fields contain 42')

if __name__=='__main__': main()
