#!/usr/bin/env python3
import math
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SAMPLES = ROOT / "samples"
SAMPLES.mkdir(exist_ok=True)

rate = 22050
count = int(rate * 0.25)
pcm = bytearray()
for i in range(count):
    value = int(math.sin(2.0 * math.pi * 440.0 * i / rate) * 12000.0)
    pcm += struct.pack("<h", value)
(SAMPLES / "aica_tone_pcm16.bin").write_bytes(pcm)

# aica_cmd_t (8 dwords) + aica_channel_t (16 dwords)
words = [24, 2, 0, 0, 0, 0, 0, 0]
words += [
    1,          # AICA_CH_CMD_START
    0x30000,    # sample base in AICA RAM
    0,          # AICA_SM_16BIT
    count,      # samples
    0,          # no loop
    0,
    count,
    rate,
    255,        # volume
    128,        # center pan
    0,
    0, 0, 0, 0, 0,
]
(SAMPLES / "aica_chan_start_packet.bin").write_bytes(struct.pack("<24I", *words))
print(f"Generated {count} PCM16 samples and one 24-dword AICA channel command")
