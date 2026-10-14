"""Read the *actual* VMU LCD framebuffer written by Dreamcast Maple (not save data).

Wire format: DCRLCD01 | uint64 little-endian unix_ms | 48*32/8 bytes.
The runtime atomically replaces <VMU save stem>.lcd on successful LCD block writes.
No app-side fake frame is produced; no data is sent to the game.
"""
from __future__ import annotations
from dataclasses import dataclass
from pathlib import Path
import struct
import time

MAGIC = b'DCRLCD01'
WIDTH, HEIGHT = 48, 32
BITMAP_SIZE = WIDTH * HEIGHT // 8
FRAME_SIZE = 16 + BITMAP_SIZE

@dataclass(frozen=True)
class Frame:
    millis: int
    bitmap: bytes
    def age_seconds(self):
        return max(0.0, time.time() - self.millis / 1000.0)


def lcd_path(vmu_image: str | Path) -> Path:
    return Path(vmu_image).with_suffix('.lcd')


def read_frame(path: str | Path) -> Frame | None:
    """Returns None for missing, incomplete or invalid frame; never reads VMU flash."""
    try:
        data = Path(path).read_bytes()
    except (OSError, ValueError):
        return None
    if len(data) != FRAME_SIZE or data[:8] != MAGIC:
        return None
    millis = struct.unpack_from('<Q', data, 8)[0]
    if millis == 0 or millis > int(time.time() * 1000) + 120_000:
        return None
    return Frame(millis, data[16:])


def bitmap_rows(bitmap: bytes, *, flip_x=False, flip_y=False, invert=False):
    """Decode the 1-bit 48x32 LCD, 6 bytes per row, MSB first.

    Additional flips are UI-only, useful when a title's orientation differs.
    """
    if len(bitmap) != BITMAP_SIZE:
        raise ValueError('Expected 192 bytes for VMU LCD image')
    rows = []
    for y in range(HEIGHT):
        row = []
        for x in range(WIDTH):
            bit = (bitmap[y*6 + x//8] >> (7 - x%8)) & 1
            row.append(bool(bit) ^ invert)
        if flip_x:
            row.reverse()
        rows.append(row)
    if flip_y:
        rows.reverse()
    return rows
