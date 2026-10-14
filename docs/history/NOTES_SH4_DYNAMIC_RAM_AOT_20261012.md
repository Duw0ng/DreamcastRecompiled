# SH-4 AOT incremental para overlays y código de RAM (experimental)

Base: `Dreamcast_Recompiled_v1.2_AQUA_PYTHON_PORTABLE_BOOTSTRAP`. Se conserva **la interfaz Aqua Python original** y los plugins existentes; no se sustituye por el launcher Win32 simplificado.

## Activación

Abrir el launcher visual Aqua → **Debug / Diagnóstico**:

- **AOT dinámico por snapshots de RAM** (`dynamic_aot`): OFF de fábrica. Añade `--dynamic-aot` a `run_game.bat`. El intérprete sigue activo por defecto y, al encontrar código que no coincide con BOOTSTRAP.BIN, captura una ventana de 4096 bytes desde la RAM. En el siguiente inicio, las capturas pendientes pueden convertirse en DLLs nativas ABI1 independientes. **No se reescribe ni se recompila el guest base.**
- **Diagnóstico de saltos SH-4 inválidos** (`bad_target_trace`): OFF de fábrica. Añade `--bad-target-trace`; ante un salto impar registra PC, PR, SR, SP, GBR, VBR, R0-R15 y las palabras SH-4 alrededor del punto de salto (máximo 16 reportes).

El modo también funciona por línea de comandos:

```bat
run_game.bat "C:\Juegos\wr.cdi" --reuse --dynamic-aot --bad-target-trace --interp-trace --no-pause
```

Las opciones AOT principales e intérprete mantienen sus propios controles. Para compilar snapshots es necesario el `tools/native/dcr_native_tools.exe` **actualizado** incluido; el respaldo Python anterior solo compila seeds estáticas y mostrará un aviso si falta la utilidad nativa.

## Nuevos archivos de caché por juego

- `incremental/dynamic/snapshot_<ENTRY>_<BASE>_<FNV64>.bin`: RAM capturada, exactamente 4096 bytes, validada por nombre/hash.
- `incremental/native/dcr_sh4_patch_<ID>.dll`: parche nativo independiente.
- `incremental/native/dcr_sh4_patch_<ID>.json`: huella SHA-256 y contexto del parche.
- `incremental/failed/<ID>.json`: cuarentena si una función no se puede compilar; evita recompilar repetidamente el mismo snapshot.

El runner calcula de nuevo el hash del boot inicial. El generador también incluye los hashes del emisor, generador incremental y Guest ABI1 en el ID del parche dinámico.

**Protección:** una DLL dinámica nunca sobreescribe un target AOT ya registrado. Si el overlay todavía no existe al iniciar, se deja en espera y solo se registra cuando el juego llama a su entrada y los **4096 bytes completos** coinciden. Se comprueban de nuevo en cada dispatch nativo dinámico: si la memoria cambia, el intérprete se hace cargo. Esto es conservador: páginas con datos variables pueden producir más fallbacks y algunos bloques grandes no se podrán compilar con una captura de 4 KiB.

## Wacky Races

La dirección `0x8CFD2000` aparece en una región que el juego carga después de arrancar, por lo que ahora es candidata a captura y compilación dinámica. Sin embargo, **el crash `0xE2010009` NO es una seed válida**: continúa siendo un destino impar y el sistema lo rechaza. Activar «Diagnóstico de saltos SH-4 inválidos» permitirá rastrear registros e instrucciones en torno a `0x8C012FB6`. No hay garantía de que el juego pase de ese fallo con este parche.

## Validaciones realizadas

- Runtime C++ compilado en Linux y tests de intérprete/variantes de snapshot: OK.
- Prueba de extremo a extremo Linux con módulo `.so` de prueba: el módulo se difiere antes de cargar el overlay, se registra cuando aparece código coincidente y vuelve al intérprete al alterar el snapshot: OK.
- `tools/native` tests Go y compilación cruzada a Windows x64: OK.
- Tests de navegación Aqua (Xvfb): 7/7 OK; nuevos flags del launcher verificados.
- **Pendiente:** compilación MSVC de un parche dinámico real y pruebas con Wacky Races en Windows.

No borres tu versión estable ni las cachés. Probá el ZIP en una carpeta independiente y compará `[DCR SH4-DYNAMIC]` de los logs con la ejecución anterior.
