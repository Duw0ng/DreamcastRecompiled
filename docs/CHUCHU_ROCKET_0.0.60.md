# ChuChu Rocket! checkpoint - 0.0.60

0.0.60 fixes the Maple DMA descriptor parser that kept A1 invisible and left
ChuChu Rocket in the black backup transition after confirming the save menu.

Katana Maple lists mix START descriptors with one-word control descriptors.
0.0.59 treated every descriptor as START, so NOP (`pattern=7`) words were read
as receive pointers and request packets. The parser then drifted into unrelated
RAM, inflating DMA/non-controller/fixup counters and preventing reliable
sub-device enumeration.

0.0.60 parses the three-bit descriptor pattern first:

- START (`0`) consumes receive pointer + request + payload and is the only
  descriptor counted as a Maple transfer frame.
- NOP (`7`) consumes one word only.
- RESET/occupy/cancel control patterns consume one word only and do not touch a
  receive buffer.

Acceptance with the ChuChu Rocket CDI on the portable runner:

- commercial closure: 2,294 functions / 171,568 known SH-4 / 0 unknown /
  `RAW_SH4=0`;
- A1 VMU reaches `DEVINFO -> GETMINFO -> block read` during normal startup;
- heartbeat reaches `vmu=A1/1/1/1/0/0` instead of staying at all zeroes;
- early `maple-recv-fix` falls to zero because NOP words are no longer mistaken
  for receive addresses;
- 47/47 CTest regressions pass.

The persistent A1 VMU introduced in 0.0.58/0.0.59 is retained unchanged.
PVR/AICA timing and rasterization are unchanged in this checkpoint.
