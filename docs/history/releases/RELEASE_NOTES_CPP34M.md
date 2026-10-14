# cpp34m — V.42 idle-mark bootstrap

- Based on cpp34l; guest ABI/emitter unchanged.
- Fixes a real-hardware V.42 startup deadlock found in ChuChu Rocket logs.
- While synchronous V.42 is in None/Detection and no response bytes are pending, the remote side now supplies continuous mark/idle octets (0xFF) to RBUFFER at the negotiated serial pace.
- This gives the guest V.42 detector receive carrier activity before it emits ODP.
- Existing ODP/ADP, HDLC/LAPM and PPP bridge remain unchanged.
- No guest recompilation is required; use TEST_CPP34M_V42_IDLE_ONLINE.bat.
