"""Regression checks for read-only VMU inspector in tester candidate."""
import tempfile
import unittest
from pathlib import Path
from vmu_inspector import read_vmu, find_vmus


def fixture():
    b=bytearray(128*1024)
    b[255*512:255*512+16]=bytes([0x55])*16
    base=255*512+0x40
    for i,value in enumerate((255,0,255,254,1,253,13)):
        b[base+i*2:base+i*2+2]=value.to_bytes(2,'little')
    for i,value in enumerate((200,31,0)):
        off=base+16+i*2
        b[off:off+2]=value.to_bytes(2,'little')
    for i in range(256):
        b[254*512+i*2:254*512+i*2+2]=(0xfffc if i<200 else 0xfffa).to_bytes(2,'little')
    return b

class VMUInspectorTests(unittest.TestCase):
    def test_formatted_empty(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'dreamcast_vmu_a1.bin';p.write_bytes(fixture());original=p.read_bytes()
            r=read_vmu(p)
            self.assertTrue(r.formatted)
            self.assertEqual((r.capacity_blocks,r.free_blocks,len(r.entries)),(200,200,0))
            self.assertEqual(p.read_bytes(),original)
            self.assertEqual(find_vmus(Path(d)),[p])

    def test_one_save_and_bad_chain(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'dreamcast_vmu_a1.bin';b=fixture()
            off=253*512; b[off]=0x33; b[off+4:off+11]=b'SAVE001'
            b[off+24:off+26]=(1).to_bytes(2,'little')
            b[254*512:254*512+2]=(0xfffa).to_bytes(2,'little')
            p.write_bytes(b);r=read_vmu(p)
            self.assertEqual(r.used_blocks,1)
            self.assertEqual(r.entries[0].filename,'SAVE001')
            self.assertEqual(r.entries[0].chain_status,'OK')
            b[254*512:254*512+2]=(0xffff).to_bytes(2,'little');p.write_bytes(b)
            self.assertNotEqual(read_vmu(p).entries[0].chain_status,'OK')

    def test_reject_bad_size(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'tiny_vmu.bin';p.write_bytes(b'123')
            with self.assertRaises(ValueError):read_vmu(p)
            self.assertEqual(find_vmus(Path(d)),[])

if __name__=='__main__':unittest.main()
