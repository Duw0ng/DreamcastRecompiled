# Real KallistiOS validation — 0.0.22

The following tests were run against the supplied KallistiOS ELF corpus, without redistributing those binaries.

## Executed natively through generated x64 code

| ELF / function | Result |
|---|---|
| `hello.elf::_main` | prints `Hello world!`, `R0=0`, host sentinel return |
| `memtest32.elf::_memTestDataBus` | `R0=0` |
| `memtest32.elf::_memTestAddressBus` | `R0=0` |
| `memtest32.elf::_memTestDevice` | `R0=0` |
| `fpu_exc.elf::___ieee754_sqrtf`, `FR5=9.0` | `FR0=3` |
| `hello.elf::_irq_inside_int` with controlled `_inside_int=42` | `R0=42` |
| `hello.elf::_arch_tls_init -> _thd_get_current` with mock thread/TLS | `GBR=0x8C123456` |

`_arch_tls_init` is the main 0.0.22 system/control semantic test: the original KOS SH-4 function loads the current thread, reads its TLS field and executes `LDC ...,GBR`.

## CPU-clean but intentionally hardware-blocked/deferred

| Function | Decoder coverage | Runtime boundary |
|---|---:|---|
| `_irq_init` | 118 / 118 | interrupts/MMU/MMIO not modeled |
| `_irq_shutdown` | 33 / 33 | interrupts/MMU/MMIO not modeled |
| `_irq_get_priority` | 13 / 13 | accesses Dreamcast MMIO (`0xFFD...`) |
| `_sq_cpy` | 104 / 104 | store-queue/locking/hardware path deferred |

A CPU-clean result is therefore not presented as a full Dreamcast-hardware implementation.
