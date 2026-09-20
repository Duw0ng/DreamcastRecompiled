#!/usr/bin/env python3
"""Build a minimal 1x1 8bpp PCX-like buffer accepted by KOS 2ndMix load_pcx()."""
from pathlib import Path
import argparse, struct

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('output', nargs='?', default='samples/pcx_1x1_8bpp.bin'); args=ap.parse_args()
    out=Path(args.output); out.parent.mkdir(parents=True, exist_ok=True)
    h=bytearray(128)
    h[0]=0x0A       # PCX manufacturer marker
    h[1]=5          # version
    h[2]=1          # RLE encoding
    h[3]=8          # Bpp -- the field 2ndMix validates
    struct.pack_into('<HHHH', h, 4, 0,0,0,0) # Xmin,Ymin,Xmax,Ymax => 1x1
    struct.pack_into('<HH', h, 12, 1,1)
    h[65]=1         # color planes
    struct.pack_into('<H', h, 66, 1) # bytes/line
    pixel=bytes([0x2A])
    marker=bytes([0x0C])
    # recognizable palette: byte i = (0x11 + i) & 0xff
    palette=bytes(((0x11+i)&0xFF) for i in range(768))
    data=bytes(h)+pixel+marker+palette
    out.write_bytes(data)
    print(f'Wrote {out} ({len(data)} bytes)')
if __name__=='__main__': main()
