"""Read-only Dreamcast VMU (128 KiB, 256 x 512-byte blocks) inspection.

Never auto-creates, formats or modifies a VMU. Designed to inspect the stable
runtime's persistent A1 image and conventional Dreamcast FAT/directory layout.
"""
from __future__ import annotations
from dataclasses import dataclass
from pathlib import Path

SIZE=128*1024
BLOCK=512
ROOT_BLOCK=255
FAT_BLOCK=254
FAT_FREE=0xfffc
FAT_END=0xfffa

@dataclass(frozen=True)
class SaveEntry:
    filename: str
    kind: str
    first_block: int
    blocks: int
    bytes_allocated: int
    copy_protected: bool
    chain_status: str

@dataclass(frozen=True)
class VMUReport:
    filename: str
    size: int
    formatted: bool
    capacity_blocks: int
    used_blocks: int
    free_blocks: int
    entries: tuple[SaveEntry,...]
    warnings: tuple[str,...]


def read_vmu(path: str | Path) -> VMUReport:
    p=Path(path)
    data=p.read_bytes()
    if len(data)!=SIZE:
        raise ValueError(f'Imagen VMU inválida: {len(data)} bytes (esperados {SIZE})')
    def u16(off):return int.from_bytes(data[off:off+2],'little')
    root=ROOT_BLOCK*BLOCK
    fat_start=FAT_BLOCK*BLOCK
    fat=[u16(fat_start+i*2) for i in range(256)]
    warnings=[]
    # The stable emulator generates a 0x55 superblock and 200 data blocks.
    # Do not trust the root's ranges without sanity checking them.
    formatted=data[root:root+16]==b'\x55'*16
    capacity=u16(root+0x40+16) if formatted else 200
    if not 1<=capacity<=200:
        warnings.append('Capacidad declarada no válida; se usan 200 bloques.')
        capacity=200
    if not formatted:
        warnings.append('Cabecera de formato no reconocida: inspección provisional.')
    free=sum(fat[i]==FAT_FREE for i in range(capacity))
    used=capacity-free
    dir_start=u16(root+0x40+10) if formatted else 253
    dir_blocks=u16(root+0x40+12) if formatted else 13
    if not 1<=dir_blocks<=13 or not 200<=dir_start<=253 or dir_start-dir_blocks+1<200:
        warnings.append('Directorio inválido; se usará el rango estándar 253..241.')
        dir_start=253;dir_blocks=13
    entries=[]
    for block in range(dir_start,dir_start-dir_blocks,-1):
        for slot in range(16):
            start=block*BLOCK+slot*32
            rec=data[start:start+32]
            typ=rec[0]
            if typ not in (0x33,0xcc):
                continue
            first=int.from_bytes(rec[2:4],'little')
            name=rec[4:16].split(b'\0',1)[0].decode('shift_jis','replace').strip()
            blocks=int.from_bytes(rec[24:26],'little')
            # Audit, don't dereference unbounded or malformed allocation chains.
            status='OK'
            seen=set()
            nxt=first
            for i in range(min(blocks,capacity+1)):
                if nxt>=capacity or nxt in seen:
                    status='Cadena FAT inválida/cíclica';break
                seen.add(nxt)
                next_block=fat[nxt]
                if i==blocks-1:
                    if next_block!=FAT_END:status='Fin de cadena FAT inesperado'
                elif next_block in (FAT_END,FAT_FREE,0xffff):
                    status='Cadena FAT truncada';break
                nxt=next_block
            if blocks==0 or blocks>capacity:
                status='Tamaño de archivo inválido'
            entries.append(SaveEntry(name or f'SIN_NOMBRE_{block}_{slot}',
                                     'Juego' if typ==0x33 else 'Datos',first,blocks,
                                     blocks*BLOCK,rec[1]!=0,status))
            if status!='OK':warnings.append(f'{name or "Archivo desconocido"}: {status}')
    return VMUReport(p.name,len(data),formatted,capacity,used,free,tuple(entries),tuple(warnings))


def find_vmus(system_dir: Path, games_dir: Path | None = None) -> list[Path]:
    """Bounded discovery; don't recursively traverse the full game cache."""
    roots=[system_dir]
    if games_dir and games_dir.is_dir():
        roots+= [p for p in list(games_dir.iterdir())[:150] if p.is_dir()]
    result=[]
    for root in roots:
        if not root.is_dir():continue
        for p in list(root.iterdir())[:400]:
            if p.is_file() and p.suffix.lower() in ('.bin','.vmu') and 'vmu' in p.name.lower():
                try:
                    if p.stat().st_size==SIZE:result.append(p)
                except OSError:pass
    return sorted(result, key=lambda p:str(p).casefold())
