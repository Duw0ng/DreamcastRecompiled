# cpp34p — IPv4 LENGTH / TCP PAYLOAD FIX

Based on cpp34o. Guest ABI remains 1 and `runtime/include/dc_runtime.hpp` is unchanged.

## Why
The real ChuChu server at 146.185.135.179:9000 answered cpp34o with the 76-byte `DreamCast Port Map. Copyright SEGA Enterprises. 1999` banner. Immediately afterwards, the PPP/IP path exposed bytes beyond the IPv4 Total Length to the TCP bridge. cpp34o could therefore interpret frame tail/padding as TCP payload, advance `guest_next`, and then trim the beginning of the real application packet as a retransmission overlap.

## Fix
- IPv4 Total Length is now authoritative at the PPP/IP boundary.
- Trailing PPP/LAPM bytes beyond the IPv4 datagram are clamped before UDP/TCP parsing.
- TCP payload length is independently bounded by IPv4 Total Length instead of `vector.size()`.
- Adds `[IPV4 LENGTH] clamp ...` diagnostics to prove when a frame carried a tail.
- Keeps cpp34o duplicate/overlap/gap TCP sequence handling.
- No DCRuntime ABI/layout change and no guest recompilation required.

## Expected ChuChu trace
The former bogus six-byte packet `02 04 05 B4 63 68` should either disappear entirely or be explicitly identified as outside the IPv4 datagram. The following 60-byte application packet must then be forwarded in full, from its first byte, without `overlap trimmed=6`.
