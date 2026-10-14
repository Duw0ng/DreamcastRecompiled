# cpp34o — TCP BRIDGE SEQ/DIAG

Rebased directly from cpp34n. No guest ABI, emitter, SH-4 discovery, V.42/LAPM,
PPP, flash, VMU, PVR or AICA behavior is changed.

The real ChuChu trace now reaches IPCP, DNS and an outbound TCP connection to the
login/lobby service on port 9000. cpp34o focuses only on the user-mode TCP bridge.

Changes:
- Logs exact remote IPv4/port for every outbound TCP flow.
- Logs DC TCP flags/seq/ack/payload preview, actual host socket writes, host socket
  reads, and packets synthesized back to the Dreamcast.
- Logs host FIN/RST-like socket failures and connect/send/recv errors.
- Fixes strict `seq == expected` handling: duplicate retransmissions are ACKed,
  overlapping retransmissions have the already-seen prefix trimmed, and forward
  gaps are not falsely acknowledged as received.
- Keeps partial host writes queued correctly.
- Diagnostic output is capped per flow to avoid runaway logs.

Expected success path:
V42/LAPM -> PPP/IPCP -> DNS -> TCP OPEN -> HOST-TX -> HOST-RX -> DC-RX.

The test BAT uses `--reuse --rebuild-runtime`; it must not recompile ChuChu.
