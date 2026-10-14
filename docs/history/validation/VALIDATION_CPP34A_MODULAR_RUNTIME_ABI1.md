# Validation — cpp34a MODULAR_RUNTIME_ABI1

Date: 2026-10-03

## Core regression suite

`ctest --output-on-failure -j 4`: **54/54 PASS**, 0 failures.

## Modular fixture

A generated SH-4 fixture was transformed with `tools/modular/modularize_generated.py` and built as:

- `dcr_game_module` — generated guest SH-4
- `DreamcastRuntime` — shared runtime
- `dcr_network` — external Network plugin
- `dreamcast_program` — thin runner

The runner loaded the plugin through ABI v1 and printed the expected plugin descriptor.

## Network-only rebuild

After touching only `runtime/plugins/dcr_network.cpp`, rebuilding `dreamcast_program` produced exactly two build steps: network plugin compilation and plugin shared-library link. Generated guest compilation count: **0**.

## Runtime-only / cross-release rebuild

The same persistent game build was reconfigured from the original runtime source path to a byte-identical runtime copied to a different directory. Results:

- runtime source compile count: 5
- generated guest compile count: **0**
- generated runner compile count: **0**
- game module: relink only

This verifies that changing ZIP/extraction paths no longer invalidates the expensive SH-4 objects.

## Persistent Dreamcast data

The modular runner honors `DCR_SYSTEM_DATA_DIR`. Flash and VMU paths are redirected to that directory before guest attachment/plugin startup, allowing the virtual console identity and VMU to survive release-folder changes.
