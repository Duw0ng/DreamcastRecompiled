# DreamcastRecomp 0.0.54 validation

## Regression

- CMake Linux build: PASS.
- CTest: **47/47 PASS**.
- Commercial raw closure from the supplied ChuChu Rocket! CDI: **2,273 functions**, **170,808 known SH-4 instructions**, **0 unknown**, **RAW_SH4=0**.
- Delta from 0.0.53: **+28 functions**, **0 lost functions**.
- Generated commercial C++: Clang 17 debug compile/link PASS in the local validation environment.

## Dispatcher validation

`0x8C026F2E` matches the compact indexed-tail shape and its literal-fed table has eight contiguous executable entries before the first non-code word. The dispatcher and its handler family are present in the generated function map.

## False-code regression

The known bad candidate `0x8C07FDC4` is printable message data and is no longer promoted by the generic callback clean-prefix fallback. The prior ~4,430 RAW_SH4 explosion does not recur.

## Runtime scope

A local headless Linux probe progressed without a new DreamcastRecomp runtime error during the bounded test interval, but the host was too slow to reach the title/START point within the practical execution window. Final post-START acceptance therefore remains a Windows run with the supplied CDI. No auto-START logic is included in the release sources.
