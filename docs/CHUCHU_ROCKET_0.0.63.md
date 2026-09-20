# ChuChu Rocket! checkpoint - 0.0.63

The accepted 0.0.61/0.0.62 path reaches the real main menu and Mode Select using a persistent A1 VMU. Four subsequent observed failures are addressed together in 0.0.63: Puzzle Edit `0x8C02DC50`, Homepage `0x8C04FFC0`, first 4P Battle state `0x8C0278CC`, and Options helper `0x8C0E23E0`.

The first three belong to the two-level state table selected by `0x8C01909C`. Recovery stays bounded to short gaps between independently known rows and exact duplicate row aliases. Options is a separate indexed-JSR table whose base literal occurs 18 bytes before the indexed load, outside the previous 16-byte search window.

No title-address seeds are added. GPU rendering is intentionally deferred.
