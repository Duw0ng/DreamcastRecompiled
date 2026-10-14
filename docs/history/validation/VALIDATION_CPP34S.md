# Validation - cpp34s V42/LAPM RELIABILITY

- Runtime + dcr_network Release build: PASS
- V42 real-trace ODP -> ADP -> raw HDLC -> SABME/UA -> PPP LCP: PASS
- LAPM RR cumulative ACK: PASS
- LAPM RNR pause / RR resume: PASS
- LAPM REJ Go-Back-N retransmission: PASS
- XID tx max/window parse: PASS (128 B / window 7 smoke)
- TX window full blocks and ACK reopens: PASS
- TCP overlap/duplicate/gap bridge smoke: PASS
- IPv4 Total Length 6-byte tail regression: PASS
- Host MMIO fastpath benchmark: 2,000,000 reads ~36 ms in this container
- Core tests excluding generated_program_build: 53/53 PASS
- generated_program_build: PASS separately
- Total unique core tests: 54/54 PASS
- dc_runtime.hpp SHA-256 unchanged: ca75e79316dc78ae74021a0853312f4c3f378a0f0524be0371c0c3db263e9739
- cpp_emitter.cpp byte-identical to cpp34r
