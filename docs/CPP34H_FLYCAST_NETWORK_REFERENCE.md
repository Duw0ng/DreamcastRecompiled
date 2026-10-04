# cpp34h — Flycast behavioral reference for Dreamcast modem/PPP

This note records **behavioral observations** from Flycast used to guide an
independent DreamcastRecompiled implementation. Flycast is GPL; no Flycast
source code is copied into DreamcastRecompiled.

## Immediate cpp34h fix: System/Boot ROM writes

The cpp34g ChuChu Rocket! Homepage run reaches guest PC `0x8C0ECC10` and
performs a 32-bit write to physical `0x00000000`. DreamcastRecompiled used to
classify that as an unmapped-memory fault.

Flycast's Area-0 memory handler treats the system ROM as read-only: writes to
the ROM aperture are logged/ignored rather than crashing execution.

cpp34h therefore treats the complete Dreamcast system-ROM aperture
`0x00000000-0x001FFFFF` as read-only for 8/16/32-bit guest writes. Reads keep
the existing BIOS-less HLE backing. Truly unmapped addresses remain fatal.

Reference:
- https://github.com/flyinghead/flycast/blob/master/core/hw/holly/sb_mem.cpp

## What Flycast does for real modem networking

Flycast keeps the original Dreamcast software stack alive instead of replacing
guest socket calls with host calls.

The chain is approximately:

```
Dreamcast game / DreamKey / PlanetWeb
        |
        v
Dreamcast modem MMIO (0x00600000-0x006007FF)
        |
        v
modem state machine
DIALING -> RINGING -> HANDSHAKING -> PRE_CONNECTED -> CONNECTED
        |
        v
serial byte stream
        |
        v
PPP endpoint
        |
        v
IPv4 / DNS / UDP / TCP proxy
        |
        v
host sockets / Internet
```

Flycast models modem status/control registers, FIFO/status bits and connection
state. Once the negotiated modem state is accepted it starts the network
service and moves payload bytes between the modem's TX/RX stream and the PPP
backend.

References:
- https://github.com/flyinghead/flycast/blob/master/core/hw/modem/modem.cpp
- https://github.com/flyinghead/flycast/blob/master/core/network/netservice.cpp
- https://github.com/flyinghead/flycast/blob/master/core/network/picoppp.cpp

## PPP topology worth reproducing independently

Flycast's PicoTCP backend creates a point-to-point PPP device with a private
Dreamcast peer and host-side gateway, supplies DNS via IPCP, then proxies
TCP/UDP traffic to host sockets. The exact implementation/library does not need
to be copied; DreamcastRecompiled can provide equivalent behavior behind its
existing network plugin ABI.

A particularly important detail is that PPP stays passive until the game first
sends serial data. This avoids sending negotiation bytes before some Dreamcast
software is ready.

## Flash/ISP data

Flycast ensures the Dreamcast user flash contains browser/ISP records, two ISP
profiles, a phone number, username/password placeholders, and a valid console
ID. It explicitly notes that the console ID is used by some network games,
including ChuChu Rocket!, and fixes its checksum/copy.

DreamcastRecompiled already has a full ISP-map generator, but cpp34h+ should
compare its byte layout/checksums against the documented behavior rather than
only checking that blocks are present.

Reference:
- https://github.com/flyinghead/flycast/blob/master/core/hw/flashrom/nvmem.cpp

## ChuChu Rocket! specific transport

Flycast recognizes ChuChu Rocket! product IDs `MK-51049`, `HDR-0039` and
`MK-5104950` and exposes UDP port **9789** for inbound game traffic.

This should become a per-title network-profile hint in DreamcastRecompiled,
not a hard-coded SH-4 address patch.

## DreamcastRecompiled implementation direction

1. Keep cpp34g's safe IP.BIN re-entry and relocation preservation.
2. Apply cpp34h read-only system-ROM write semantics.
3. Instrument the first access to modem MMIO `0x00600000-0x006007FF`.
4. Replace open-bus-only modem behavior with a small register/state front-end.
5. Start the existing network plugin only when the virtual modem reaches
   CONNECTED.
6. Feed modem TX/RX bytes to an independent PPP endpoint.
7. Proxy IPv4/DNS/TCP/UDP through host sockets.
8. Add a ChuChu Rocket! profile for UDP 9789 and optional UPnP/NAT mapping.
9. Validate ISP profile and console-ID checksums before browser launch.

The important architectural conclusion is that **native host networking is the
back-end**, while the Dreamcast-visible front-end still needs to look like a
real modem. Bypassing the modem layer entirely risks never reaching the game's
own PPP/IP stack.
