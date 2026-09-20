# ChuChu Rocket! checkpoint - 0.0.61

The 0.0.60 Windows run proved the VMU path is active: `vmu=A1/1/1/28/20/5` means A1 DEVINFO and media-info succeeded, 28 block reads occurred, 20 phased writes were accepted, and five sync commands completed. The next runtime boundary was a clean dynamic call from `0x8C0190A4` to `0x8C022F24`.

`0x8C022F24` is the first method of row 5 in the nested state table rooted at `0x8C0B373C`. Rows 0-3 were already recovered and a later row is independently known. 0.0.61 bridges only such short verified gaps, requiring three strict callable methods in every missing row. No ChuChu-specific address is seeded.

Final supplied-CDI closure: 2,349 functions / 174,530 known SH-4 instructions / 0 unknown / RAW_SH4=0.
