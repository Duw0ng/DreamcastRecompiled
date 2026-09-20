DreamcastRecomp 0.0.176

Main CT2 test folder:
  experimental_ct2_0.0.176

1. Copy your Crazy Taxi 2 image into that folder as ct2.cdi.
2. Run build_windows.bat.
3. Run run_crazy_taxi_2.bat.

Controls:
  Arrow keys = D-Pad
  J / Space / Z = A
  K / X = B
  Enter = START

This release fixes the runtime registration regression that caused 0.0.174/0.0.175 to fail at 0x0C14AD06 and includes the newly discovered clean AOT entry 0x8C0383B2.

The audit BAT is optional and injects controls automatically. Use the normal BAT for your real gameplay test.
