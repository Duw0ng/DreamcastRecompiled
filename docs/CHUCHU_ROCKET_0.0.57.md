# ChuChu Rocket! acceptance — 0.0.57

## Windows checkpoint

0.0.56 crossed the previous BIOS-font memory fault and progressed until the central state dispatcher `0x8C01909C` selected `0x8C05D55A`.

## Fix

The target belongs to a six-method state family stored in a row whose following top-level entry aliases the same method array starting three entries later. 0.0.57 recognizes this overlapping-row structure generically and adds the bounded method family without resuming broad table scanning.

Recovered entries: `0x8C05D39A`, `0x8C05D460`, `0x8C05D3CA`, `0x8C05D55A`, `0x8C05D58C`, `0x8C05D56C`.

## Next observation

Run the same save/create path again. If it reaches a VMU command or another unresolved target, that becomes the next compatibility boundary.
