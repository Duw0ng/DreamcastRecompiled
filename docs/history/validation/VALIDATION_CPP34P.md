# Validation cpp34p

- Runtime/plugin CMake build: PASS.
- `runtime/include/dc_runtime.hpp` SHA-256 remains `ca75e79316dc78ae74021a0853312f4c3f378a0f0524be0371c0c3db263e9739` (ABI1 unchanged).
- V.42 raw-flag/LAPM/PPP real-trace smoke: PASS (`state=6 conn=5`, PPP/LCP active).
- TCP sequence regression smoke: PASS (normal send, duplicate suppression, overlap trimming, gap ACK, host RX).
- IPv4-tail regression smoke reproducing the cpp34o failure: PASS. A 46-byte backing frame with IPv4 Total Length 40 is clamped (`tail=6`), advances no TCP sequence, sends no host payload, and the following 60-byte packet is transmitted 60/60 from byte 0.
- No guest ABI/layout change. Existing ChuChu generated cache can be reused.

The full core tree is unchanged from cpp34o except runtime build identification and the network plugin; cpp34o had already passed the core suite. cpp34p validation therefore targeted the modified network path plus ABI identity.
