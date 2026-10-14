# DreamcastRecomp v1.2 cpp34g — PRESERVE_RELOC_IPBIN_ABI1

cpp34f llegó correctamente a Homepage y confirmó una reentrada real por 0x8C0083A8.
El primer restore de IP.BIN funcionó, pero la limpieza eliminó también relocations
legítimas dentro de IP.BIN: 0x8C00F400 -> 0x8C1409A8 y 0x8C00FA00 -> 0x8C14094C.

cpp34g conserva esas relocations al restaurar IP.BIN. Solo vacía el dispatch cache
y limpia las páginas dirty de literales. El log añade `reloc-aliases-preserved=N`.
