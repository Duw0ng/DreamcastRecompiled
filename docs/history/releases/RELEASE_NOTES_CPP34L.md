# cpp34l — V42/LAPM PPP bridge

Base: cpp34k / guest ABI1 unchanged.

## Why
Online-direct reaches modem `m6/c5` (Normal/Connected) but heartbeat remains `ppp0,0 / ip0 / udp0`. Flycast's current modem path uses V.42/LAPM when modem register 08 has ASYN=0. cpp34k incorrectly fed synchronous modem-wire bytes directly to the PPP parser.

## Changes
- Adds an independently implemented minimal V.42/LAPM termination layer in the network plugin.
- V.14-style ODP detection and ADP `EC` response.
- Bit-level HDLC flag detection, bit stuffing/de-stuffing, CRC-16.
- SABME/UA establishment, XID response with V.42bis explicitly disabled, I-frame payload transport, RR/REJ.
- PPP replies are wrapped back into LAPM I-frames before reaching the Dreamcast modem RX FIFO.
- ASYN=1 keeps the old direct PPP path.
- Adds first-64-byte synchronous wire probe to logs for diagnosis.
- No DCRuntime layout change and no guest SH-4 regeneration required.

## Expected progression
`m6/c5` -> `[V42] ... LAPM connected` -> `ppp>0` -> LCP/PAP-or-CHAP/IPCP -> `ip>0` -> DNS/UDP 9789.
