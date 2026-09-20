# Probar KallistiOS `hello.elf` en DreamcastRecomp 0.0.22

Coloca el archivo real en:

```text
samples/hello.elf
```

Para inspeccionarlo sin ejecutar:

```powershell
.\build\Release\dc_analyzer.exe .\samples\hello.elf --analyze-function _main
.\build\Release\dc_analyzer.exe .\samples\hello.elf --cfg _main
.\build\Release\dc_analyzer.exe .\samples\hello.elf --emit-ir _main
```

Para generar C++:

```powershell
.\build\Release\dc_recomp.exe .\samples\hello.elf --function _main --output .\generated\hello_native
```

Para ejecutar todo automaticamente:

```text
run_native_hello.bat
```

El runner debe obtener la direccion de `_printf` desde la tabla de simbolos del
ELF, cargar los datos de `.rodata` en la RAM del runtime y leer el format string
usando el valor que el codigo recompilado deja en `R4`.

Por tanto el mensaje de consola procede de `hello.elf`; no esta escrito como una
constante de texto en `generated_runner.cpp`.
