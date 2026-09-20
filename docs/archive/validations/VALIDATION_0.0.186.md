# DreamcastRecomp 0.0.191 validation

SPG_STATUS is guest-cycle-driven, frame cadence derives from guest SPG timing registers, and timing changes re-synchronize the raster. CT2 wait157 telemetry remains enabled to test whether the 12-VBlank stalls collapse toward one VBlank.
