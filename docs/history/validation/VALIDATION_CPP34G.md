# Validation cpp34g

- Runtime C++: compila correctamente con CMake Release.
- dcr_network: compila correctamente con CMake Release.
- `TEST_CPP34G_PRESERVE_RELOC.bat`: 34 CRLF reales, sin `\\n` literales.
- Build ID: `v1.2-cpp34g-preserve-reloc-ipbin`.
- `dispatch_cache` se vacía al restaurar IP.BIN.
- `relocated_code_aliases` ya no se borran durante el restore.
- El log de restore muestra `reloc-aliases-preserved=N`.

Hipótesis validada contra el log cpp34f: la primera reentrada real ocurrió en
0x8C0083A8; después del restore cpp34f perdió los aliases F400/FA00 y el segundo
paso a 0x8C00FA00 terminó sin target. cpp34g conserva dichos aliases para que el
dispatch vuelva al template ya descubierto.
