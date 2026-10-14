# Validation — cpp36b1f

- Windows amd64 launcher cross-build: PASS (`PE32+ GUI`).
- `go vet` with `GOOS=windows GOARCH=amd64`: PASS.
- Auto portable sync fixture: PASS.
- Synced portable launcher SHA-256 matches source portable launcher: PASS.
- Portable ZIP integrity (`unzip -t`): PASS.
- SAFE Guest ABI1 hash preserved: PASS.
- Runtime build ID preserved as cpp36b1e: PASS (launcher-only release).
- Static visual mapping markers for D-pad, face buttons, START, analog stick and L/R triggers: PASS.
