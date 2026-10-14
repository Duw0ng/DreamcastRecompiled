"""VMU LCD transport tests (no fabricated game output or renderer dependency)."""
from pathlib import Path
import struct
import sys
import tempfile
import time
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from vmu_lcd import FRAME_SIZE, MAGIC, bitmap_rows, lcd_path, read_frame

class LcdTransportTests(unittest.TestCase):
    def test_valid_real_frame_contract(self):
        with tempfile.TemporaryDirectory() as d:
            save=Path(d,'dreamcast_vmu_a1.bin')
            frame_path=lcd_path(save)
            now=int(time.time()*1000)
            raw=bytearray(192)
            raw[0]=0b10000001
            raw[6]=0b01000000
            frame_path.write_bytes(MAGIC + struct.pack('<Q',now) + raw)
            frame=read_frame(frame_path)
            self.assertEqual(FRAME_SIZE,208)
            self.assertEqual(frame.millis,now)
            pixels=bitmap_rows(frame.bitmap)
            self.assertTrue(pixels[0][0])
            self.assertTrue(pixels[0][7])
            self.assertTrue(pixels[1][1])
            self.assertFalse(pixels[1][0])
            self.assertGreaterEqual(frame.age_seconds(),0)
            self.assertTrue(bitmap_rows(frame.bitmap,flip_x=True)[0][47])
            self.assertTrue(bitmap_rows(frame.bitmap,flip_y=True)[31][0])
            self.assertFalse(bitmap_rows(frame.bitmap,invert=True)[0][0])
    def test_no_signal_does_not_make_up_lcd(self):
        with tempfile.TemporaryDirectory() as d:
            f=Path(d,'mem.lcd')
            self.assertIsNone(read_frame(f))
            f.write_bytes(b'BAD' + bytes(205))
            self.assertIsNone(read_frame(f))
            f.write_bytes(MAGIC + bytes(8) + bytes(192))
            self.assertIsNone(read_frame(f))
            f.write_bytes(MAGIC + struct.pack('<Q',int(time.time()*1000)) + bytes(191))
            self.assertIsNone(read_frame(f))
    def test_bmp_size_guard(self):
        with self.assertRaises(ValueError): bitmap_rows(bytes(191))

if __name__=='__main__':unittest.main()
